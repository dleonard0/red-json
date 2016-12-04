#include <assert.h>
#include <errno.h>
#include <string.h>

#include "libjson_private.h"

#define WHITESPACE " \t\n\r"
#define MAX_NEST 32768

/**
 * Skip the pointer over insignificant whitespace.
 * @param json_ptr  pointer to (optional) JSON text
 **/
void
skip_white(const __JSON char **json_ptr)
{
	if (!*json_ptr)
		return;
	while (**json_ptr && strchr(WHITESPACE, **json_ptr))
		++*json_ptr;
}

/**
 * Skip the pointer over an expeectd character and its following whitespace.
 * But it has no effect if the character is not there.
 *
 * @param json_ptr  pointer to (optional) JSON text
 * @param ch        expected character to skip
 * @return 0 iff no skipping occurred (@a ch was not the next character).
 */
int
can_skip_delim(const __JSON char **json_ptr, char ch)
{
	if (*json_ptr && **json_ptr == ch) {
		++*json_ptr;
		skip_white(json_ptr);
		return 1;
	}
	return 0;
}

/**
 * Skip an unquoted word or a quoted JSON string,
 * and its trailing whitespace.
 * That means it will skip booleans, numbers, strings, and null.
 * However, it will <em>not</em> skip structural characters [] {} : ,
 *
 * @param json_ptr  pointer to (optional) JSON text
 * @return 0 iff no skipping occurred.
 */
int
skip_word_or_string(const __JSON char **json_ptr)
{
	const __JSON char *json;

	json = *json_ptr;
	if (!json) {
		return 0;
	}

	if (*json == '"' || *json == '\'') {
		__JSON char quote = *json++;
		__JSON char ch;
		while ((ch = *json++)) {
			if (ch == quote)
				break;
			if (ch == '\\')
				json++;
		}
	}
	else if (is_word_start(*json)) {
		do { json++; } while (is_word_char(*json));
	}
	else {
		return 0;
	}

	skip_white(&json);
	*json_ptr = json;
	return 1;
}

/**
 * Skips over a JSON value and its trailing whitespace.
 * This skips numbers, strings, arrays, objects, words, quoted strings, etc.
 *
 * This function is non-recursive so that it can handle nested arrays and
 * objects up to a depth of 32768.
 *
 * @param json_ptr  pointer to (optional) JSON text (not whitespace!)
 * @returns 0 if nothing was skipped,
 *          1 if a value was skipped, or
 *          0 if a depth limit was reached (#ENOMEM)
 */
int
skip_value(const __JSON char **json_ptr)
{
	/* Avoid recursion by using a nesting stack that remembers
	 * whether an array or object was entered (1=array) */
	typedef unsigned long word_t;
	word_t nest[MAX_NEST / (8 * sizeof (word_t))];
	struct {
		size_t offset;  /* index into nest[] */
		word_t bit;     /* bitmask into nest[offset] */
	} depth = { 0, 0 };	/* bit=0 means toplevel */
	word_t const max_bit = (word_t)1 << ((8 * sizeof (word_t)) - 1);
	size_t const max_offset = sizeof nest / sizeof nest[0];
	const __JSON char *json = *json_ptr;

	if (!json)
		return 0;
	while (*json) {
		assert(!strchr(WHITESPACE, *json));
		/* Handle popping the nest stack */
		while (*json == ']' || *json == '}') {
			if (!depth.bit) /* stack underflow */
				goto done;
			if (nest[depth.offset] & depth.bit) {
				if (*json == '}')
					goto done; /* wrong close */
			} else {
				if (*json == ']')
					goto done; /* wrong close */
			}
			json++;
			skip_white(&json);

			if (depth.bit == 1 && depth.offset == 0) {
				goto done; /* completed */
			}
			if (depth.bit == 1) {
				depth.bit = max_bit;
				depth.offset--;
			} else {
				depth.bit >>= 1;
			}
		}

		if (depth.bit && !(nest[depth.offset] & depth.bit)) {
			/* We're in an object; expect a key and colon */
			(void) skip_word_or_string(&json);
			(void) can_skip_delim(&json, ':');
		}

		if (*json == '[' || *json == '{') {
			/* enter new nesting level */
			__JSON char open = *json;
			if (depth.bit == 0) {
				/* outermost structure */
				depth.bit = 1;
				depth.offset = 0;
			} else if (depth.bit == max_bit) {
				/* push to next word in nesting stack */
				depth.bit = 1;
				depth.offset++;
				if (depth.offset >= max_offset) {
					errno = ENOMEM;
					return 0; /* stack overflow */
				}
			} else
				depth.bit <<= 1;
			if (open == '[')
				nest[depth.offset] |= depth.bit;
			else
				nest[depth.offset] &= ~depth.bit;
			json++;
			skip_white(&json);
		} else {
			skip_word_or_string(&json);
			if (!depth.bit)
				goto done; /* completed */
		}

		/* Skip commas if they are there */
		(void) can_skip_delim(&json, ',');
	}
	/* EOS */
done:
	if (json == *json_ptr)
		return 0;
	*json_ptr = json;
	return 1;
}

