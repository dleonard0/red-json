#include <string.h>

#include "libjson_private.h"
#include "utf8.h"

/** Delimiters are bytes that terminate unquoted words.
 *  Delimiters are the set: U+0..U+1F [ ] { } : , " ' */
int
is_delimiter(__JSON char ch)
{
	return ch <= ' ' || strchr("[]{}:,\"\'", ch);
}

/** Words are tokens starting with any char except delimiters */
int
is_word_start(__JSON char ch)
{
	return !is_delimiter(ch);
}

/** Words are tokens containing quotes or non-delimiter characters */
int
is_word_char(__JSON char ch)
{
	return ch == '\'' || is_word_start(ch);
}

/**
 * Compares a JSON word with a C UTF-8 string.
 *
 * @param json  (optional) JSON word (no whitespace)
 * @param str   a UTF-8 string
 * @param strsz length of the string
 *
 * @retval -1 @a json is invalid, or sorts earlier than @a str.
 * @retval  0 @a json is identical to @a str.
 * @retval  0 @a json is invalid and @a str is the empty string.
 * @retval +1 @a json sorts later than @a str.
 */
int
word_strcmpn(const __JSON char *json, const char *str, size_t strsz)
{
	if (!json || !is_word_start(*json))
		return strsz ? -1 : 0;
	if (strsz)
		do {
			if (*json != *str)
				return *json < *str ? -1 : 1;
			strsz--;
			str++;
			json++;
		} while (is_word_char(*json) && strsz);

	if (strsz)
		return -1;
	else if (is_word_char(*json))
		return 1;
	else
		return 0;
}

/**
 * Compares a JSON word with a C UTF-8 string.
 *
 * @param json  (optional) JSON word (no whitespace)
 * @param str  a NUL-terminated UTF-8 C string
 *
 * @retval -1 @a json is invalid, or sorts earlier than @a str.
 * @retval  0 @a json is identical to @a str.
 * @retval  0 @a json is invalid and @a str is the empty string.
 * @retval +1 @a json sorts later than @a str.
 */
int
word_strcmp(const __JSON char *json, const char *str)
{
	if (!json || !is_word_start(*json))
		return *str ? -1 : 0;

	do {
		if (*json != *str)
			return *json < *str ? -1 : 1;
		str++;
		json++;
	} while (is_word_char(*json) && *str);

	if (*str)
		return -1;
	else if (is_word_char(*json))
		return 1;
	else
		return 0;
}
