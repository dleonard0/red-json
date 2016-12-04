#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>

#include "libjson_private.h"
#include "utf8.h"

/** Convert a hexadecimal digit into its integer value */
static unsigned
xdigit_as_int(char ch)
{
	/* assert(isxdigit(ch)); */
	return (ch & 0xf) + (ch & 0x10 ? 0 : 9);
}

/**
 * Scan a four-digit hexadecimal number.
 *
 * @param json_ptr pointer to (optional) JSON input
 * @param u_return storage for decoded hexadecimal number
 * @return 0 unless four hexadecimal digits were decoded into
 *         u_return and the json_ptr was advanced
 */
static int
four_xdigits(const __JSON char **json_ptr, unicode_t *u_return)
{
	const __JSON char *json;
	unicode_t result;
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
 * Decode a UTF-8 code point or escape sequence from a quoted JSON string.
 *
 * Invalid escapes are treated as beginning with "invalid" code U+DC5C.
 *
 * @param json_ptr  pointer-to-pointer to UTF-8, always advanced
 * @return codepoint (never an error)
 */
__SANITIZED unicode_t
get_escaped_sanitized(const __JSON char **json_ptr)
{
	__SANITIZED unicode_t u;
	const __JSON char *after_backslash;

	assert(**json_ptr != '\0');
	u = get_utf8_sanitized(json_ptr);
	if (u != '\\')
		return u; /* Not an escape */

	after_backslash = *json_ptr;
	switch (*(*json_ptr)++) {
	    case 'u':
	    {
		unicode_t code;
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
			unicode_t hi = code;
			unicode_t lo;
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

	/* Escape sequence was unrecognised; sanitize the \ */
	*json_ptr = after_backslash;
	return 0xdc5c;
}


/** Puts \uNNNN */
static int
put_uescape(unicode_t u, void *buf, size_t bufsz)
{
	char *out = buf;
	unsigned shift;

	if (bufsz) bufsz--, *out++ = '\\';
	if (bufsz) bufsz--, *out++ = 'u';

	shift = 16;
	do {
		shift -= 4;
		if (bufsz) bufsz--,
		           *out++ = "0123456789abcdef"[0xf & (u >> shift)];
	} while (shift);
	return 6;
}

/** Puts a unicode codepoint, escaped for a JSON string.
 * @param u     sanitized code point
 * @param buf   output buffer
 * @param bufsz sizeof of the output buffer
 * @return number of bytes written to buf if it were big enough
 */
int
put_sanitized_escaped(__SANITIZED unicode_t u, void *buf, int bufsz)
{
	char *out = buf;
	char ch;


	switch (u) {
	case 0x0008: ch = 'b'; break;
	case 0x0009: ch = 't'; break;
	case 0x000a: ch = 'n'; break;
	case 0x000c: ch = 'f'; break;
	case 0x000d: ch = 'r'; break;
	case 0x005c: ch = '\\'; break;
	case 0x0022: ch = '"'; break;
	default:
		if (u < 0x20)
			return put_uescape(u, buf, bufsz);
		return put_sanitized_utf8(u, buf, bufsz);
	}

	if (bufsz) bufsz--, *out++ = '\\';
	if (bufsz) bufsz--, *out++ = ch;
	return 2;
}


#define SAFE 1

/**
 * Convert a JSON value into a NUL-terminated C string.
 * @param json  (optional) JSON text
 * @param buf   (optional) output buffer, always NUL-terminated
 * @param bufsz (optional) size of output buffer, or 0 for no output
 * @param flags <ul>
 *              <li>#SAFE: fail if the output would contain invalid UTF-8
 *              </ul>
 * @return number of bytes modified in the output buffer, or would have
 *         been had there been enough room; or
 *         0 if the #SAFE flag was set and bad UTF-8 was present (#EINVAL)
 */
size_t
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
		__SANITIZED unicode_t u;
		if (quote)
			u = get_escaped_sanitized(&json);
		else
			u = get_utf8_sanitized(&json);
		assert(u != 0);
		if ((flags & SAFE) && !IS_UTF8_SAFE(u)) {
			goto invalid;
		}
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
 * Convert a JSON value to an allocated C string.
 * @param json (optional) JSON text
 * @param flags @see #as_str()
 * @return NULL #EINVAL if #SAFE and the output would contain invalid UTF-8; or
 *         NULL #ENOMEM if storage could not be allocated; or
 *         pointer to a NUL-terminated UTF-8 string
 */
char *
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

int
string_cmp(const __JSON char *json, const char *str, const char *str_end)
{
	char quote;

	quote = *json++;

	while (str < str_end) {
		__SANITIZED unicode_t ju;
		unicode_t su;
		size_t n;

		if (!*json || *json == quote)
			return -1; /* json string is short */

		n = get_utf8_raw_bounded(str, str_end, &su);
		if (n == 0)
			return 1; /* Broken str sorts low */
		str += n;

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

/**
 * Compares a JSON string against a C string constant.
 * @param json (optional) JSON text
 * @param str  UTF-8
 * @param strsz length of @a str in bytes
 * @return -1 if @a json < @a str, or
 *          0 if @a json = @a str, or
 *          1 if @a json > @a str.
 */
__PUBLIC
int
json_strcmpn(const __JSON char *json, const char *str, size_t strsz)
{
	const char *str_end = str + strsz;

	skip_white(&json);
	if (!json || !*json)
		return -1;
	if (*json == '\'' || *json == '"')
		return string_cmp(json, str, str_end);
	if (is_delimiter(*json) || word_strcmp(json, json_null) == 0)
		return -1;
	return word_strcmpn(json, str, strsz);
}

__PUBLIC
int
json_strcmp(const __JSON char *json, const char *str)
{
	if (str)
		return json_strcmpn(json, str, strlen(str));

	/* Comparing against str NULL */
	if (!json)
		return 0;
	skip_white(&json);
	if (*json == '"' || *json == '\'')
		return 1;
	if (!*json || is_delimiter(*json))
		return 0;
	if (word_strcmp(json, json_null) == 0)
		return 0;
	return 1;
}

