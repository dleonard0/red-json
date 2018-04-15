#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>

#include "private.h"
#include "utf8.h"

#define SAFE 1

/** Converts a hexadecimal ASCII digit into its integer value. */
static unsigned
xdigit_as_int(char ch)
{
	/*
	 *   0x30 .. 0x39   ->    0 ..  9
	 *   0x41 .. 0x46   ->   10 .. 15
	 *   0x61 .. 0x66   ->   10 .. 15
	 */
	return (ch & 0xf) + (ch & 0x10 ? 0 : 9);
}

/**
 * Scans a four-digit hexadecimal number.
 *
 * @param json_ptr pointer to (optional) JSON input.
 *                 The pointer will be advanced past the digits on success.
 * @param u_return storage for the decoded hexadecimal number
 *
 * @retval 1 The scan was successful, and the pointer was advanced.
 * @retval 0 The input is not four hexadecimal digits.
 */
static int
four_xdigits(const __JSON char **json_ptr, ucode *u_return)
{
	const __JSON char *json;
	ucode result;
	unsigned i;

	json = *json_ptr;
	if (!json)
		return 0;
	for (result = 0, i = 0; i < 4 && *json; i++) {
		int ch = *json++;
		if (!isxdigit(ch))
			return 0;
		result = (result << 4) | xdigit_as_int(ch);
	}
	*json_ptr = json;
	*u_return = result;
	return 1;
}

/**
 * Decodes a single UTF-8 character or escape sequence from a quoted string.
 *
 * Invalid escapes are treated as though the initial backslash was an
 * invalid byte. That is it returns U+DC5C.
 *
 * Invalid bytes are always mapped into U+DC00..U+DCFF.
 *
 * @param json_ptr  pointer to UTF-8 string position, always advanced
 *
 * @returns a sanitized unicode ucode
 */
__SANITIZED ucode
get_escaped_sanitized(const __JSON char **json_ptr)
{
	__SANITIZED ucode u;
	const __JSON char *after_backslash;

	/* Although this function could return U+DC00 for NULs, if it
	 * is ever called that way, then something is wrong. We would
	 * be advancing over the end of a NUL-terminated JSON input string. */
	assert(**json_ptr != '\0');

	u = get_utf8_sanitized(json_ptr);
	if (u != '\\')
		return u; /* Not an escape */

	after_backslash = *json_ptr;
	switch (*(*json_ptr)++) {
	    case 'u':
	    {
		ucode code;
		if (!four_xdigits(json_ptr, &code))
			break; /* \u???? */
		if (!code)
			break; /* \u0000 */
		if (IS_SURROGATE_LO(code))
			break;
		if (IS_SURROGATE_HI(code)) {
			/*
			 * RFC7159 specifies handling of \u-escaped
			 * surrogate pairs.
			 */
			ucode hi = code;
			ucode lo;
			if (*(*json_ptr)++ != '\\')
				break;
			if (*(*json_ptr)++ != 'u')
				break;
			if (!four_xdigits(json_ptr, &lo))
				break;
			if (!IS_SURROGATE_LO(lo))
				break;
			lo &= 0x3ff;
			hi &= 0x3ff;
			return ((hi << 10) | lo) + 0x10000;
						/* U+10000..U+10FFFF */
		}
		return code;
	    }
	    case '"': return 0x0022;
	    case '\'':return 0x0027; /* non-standard */
	    case '\\':return 0x005c;
	    case '/': return 0x002f;
	    case 'b': return 0x0008;
	    case 'f': return 0x000c;
	    case 'n': return 0x000a;
	    case 'r': return 0x000d;
	    case 't': return 0x0009;
	}

	/* The escape sequence was unrecognised; sanitize the \ */
	*json_ptr = after_backslash;
	return 0xdc5c;
}

/**
 * Converts a JSON value into a NUL-terminated C string.
 *
 * Bare words are copied without change. Quoted strings have their
 * escaped sequences decoded.
 *
 * @param json  (optional) JSON text
 * @param buf   (optional) output buffer. This will always be NUL-terminated.
 *              If an error occurs, and there is space, the first byte
 *              of the buffer will be set to NUL.
 * @param bufsz (optional) size of the output buffer
 * @param flags <ul>
 *              <li>#SAFE: don't allow the output to contain invalid UTF-8
 *              </ul>
 *
 * @returns the number of bytes stored in the output buffer (including
 *          the trailing NUL) or would have been stored had there been
 *          enough space
 * @retval 0 [EINVAL] the JSON text contains invalid UTF-8, and
 *                    the SAFE flag is set.
 * @retval 0 [EINVAL] The input is neither a word nor a quoted string.
 */
static size_t
as_str(const __JSON char *json, void *buf, size_t bufsz, int flags)
{
	char *out = buf;
	size_t n = 0;
	char quote;

	if (!json)
		goto invalid; /* NULL text becomes "" */

	skip_white(&json);
	if (*json == '"' || *json == '\'') {
		/* Single and double-quoted strings */
		quote = *json++;
	} else if (is_word_start(*json)) {
		/* Bare words */
		quote = 0;
	} else {
		/* Anything else gets an empty string */
		goto invalid;
	}

	while (*json && (quote ? (*json != quote) : is_word_char(*json))) {
		__SANITIZED ucode u;
		if (quote)
			u = get_escaped_sanitized(&json);
		else
			u = get_utf8_sanitized(&json);
		assert(u != 0);
		if ((flags & SAFE) && !IS_UTF8_SAFE(u)) {
			goto invalid;
		}

		assert(u < 0x200000); /* for put_utf8_raw() */
		n += put_utf8_raw(u, out + n, bufsz > n ? bufsz-n : 0);
	}
	if (quote && *json != quote)
		goto invalid;

	if (n < bufsz)
		out[n] = '\0';
	n++;
	/* Not sure this is the best style of interface,
	 * since it requires the caller to remember that bufsz==0
	 * suppresses ENOMEM and returns non-zero buffer space */
	if (bufsz && n > bufsz) {
		errno = ENOMEM;
		goto fail;
	}
	return n;
invalid:
	errno = EINVAL;
fail:
	out = buf;
	if (bufsz) *out = '\0';
	return 0;
}

/**
 * Converts a JSON value to a heap-allocated C string.
 *
 * @param json (optional) JSON text
 * @param flags @see #as_str()
 *
 * @returns pointer to a heap-allocated, NUL-terminated UTF-8 string
 * @retval NULL [EINVAL] The output would contain invalid UTF-8 and
 *                       the #SAFE flag is set.
 * @retval NULL [ENOMEM] Heap storage could not be allocated.
 */
static char *
as_str_alloc(const __JSON char *json, int flags)
{
	size_t sz = as_str(json, NULL, 0, flags);
	char *buf = sz ? malloc(sz) : NULL;
	if (buf)
		(void) as_str(json, buf, sz, flags);
	return buf;
}

__PUBLIC
size_t
json_as_unsafe_str(const __JSON char *json, void *buf, size_t bufsz)
{
	return as_str(json, buf, bufsz, 0);
}

__PUBLIC
size_t
json_as_str(const __JSON char *json, void *buf, size_t bufsz)
{
	return as_str(json, buf, bufsz, SAFE);
}

__PUBLIC
char *
json_as_unsafe_strdup(const __JSON char *json)
{
	return as_str_alloc(json, 0);
}

__PUBLIC
char *
json_as_strdup(const __JSON char *json)
{
	return as_str_alloc(json, SAFE);
}

/**
 * Compares content of a quoted JSON string with a UTF-8 C string segment.
 *
 * @param json     JSON string value
 * @param cstr     C string segment. The segment need not be NUL-terminated.
 *                 It may contain UTF-8 encodings of sanitized code points
 *                 U+DC00..U+DCFF.
 * @param cstr_end the end of the C string segment. Memory at this location
 *                 will not be accessed.
 *
 * @retval -1 The JSON string is invalid, or its content sorts before @a cstr.
 * @retval  0 The JSON string's content is equal to @a cstr.
 * @retval +1 The JSON string's content sorts after @a cstr.
 */
static int
string_cmp(const __JSON char *json, const char *cstr, const char *cstr_end)
{
	char quote;

	assert(*json == '\'' || *json == '"');

	quote = *json++;
	while (cstr < cstr_end) {
		__SANITIZED ucode ju;
		ucode su;
		size_t n;

		if (!*json || *json == quote)
			return -1; /* json string is short */

		n = get_utf8_raw_bounded(cstr, cstr_end, &su);
		if (n == 0)
			return 1; /* Broken cstr sorts low */
		cstr += n;

		ju = get_escaped_sanitized(&json);
		if (ju != su)
			return ju < su ? -1 : 1;
	}
	if (!*json)
		return -1; /* broken strings sort lower */
	else if (*json == quote)
		return 0;
	else
		return 1;
}

__PUBLIC
int
json_strcmpn(const __JSON char *json, const char *cstr, size_t cstrsz)
{
	const char *cstr_end = cstr + cstrsz;

	skip_white(&json);
	if (json && (*json == '\'' || *json == '"'))
		return string_cmp(json, cstr, cstr_end);
	errno = EINVAL;
	if (!json || is_delimiter(*json))
		return cstrsz ? -1 : 0;
	return word_strcmpn(json, cstr, cstrsz);
}

__PUBLIC
int
json_strcmp(const __JSON char *json, const char *cstr)
{
	return json_strcmpn(json, cstr, strlen(cstr));
}
