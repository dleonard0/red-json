#include <assert.h>
#include <errno.h>
#include <string.h>

#include "private.h"
#include "utf8.h"

#define SAFE 1

/**
 * Stores a 6-byte \uXXXX sequence in the buffer.
 *
 * @param u     code point in \u0000..\uFFFF
 * @param buf   output buffer
 * @param bufsz output buffer size
 * @returns 6, which is the number of bytes that were stored
 *          or would have been stored had there been room.
 */
static size_t
put_uescape(ucode u, void *buf, int bufsz)
{
	char *out = buf;
	static const char HEX[] = "0123456789abcdef";

	assert(u <= 0xffff);

	if (bufsz > 0) bufsz--, *out++ = '\\';
	if (bufsz > 0) bufsz--, *out++ = 'u';
	if (bufsz > 0) bufsz--, *out++ = HEX[(u >> 12) & 0xf];
	if (bufsz > 0) bufsz--, *out++ = HEX[(u >>  8) & 0xf];
	if (bufsz > 0) bufsz--, *out++ = HEX[(u >>  4) & 0xf];
	if (bufsz > 0) bufsz--, *out++ = HEX[(u >>  0) & 0xf];
	return 6;
}

/**
 * Puts a unicode ucode, suitably string escaped, into the buffer.
 *
 * Only adds bytes of codepoints that are permitted by the RFC
 * to be part of JSON strings are added as clean UTF-8 to the buffer.
 * (But does not check for surrogate pairs).
 *
 * @param u     sanitized code point
 * @param buf   output buffer
 * @param bufsz output buffer size
 *
 * @returns number of bytes that were stored, or would have been stored
 *          had there been enough space
 */
static size_t
put_sanitized_str_escaped(__SANITIZED ucode u, void *buf, int bufsz)
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
		if (u >= 0x20)
			return put_sanitized_utf8(u, buf, bufsz);
		return put_uescape(u, buf, bufsz);
	}

	if (bufsz > 0) bufsz--, *out++ = '\\';
	if (bufsz > 0) bufsz--, *out++ = ch;
	return 2;
}

/**
 * Converts a UTF-8 string into a quoted JSON string.
 *
 * @param src     UTF-8 input
 * @param srclen  UTF-8 input size in bytes
 * @param dst     output buffer for JSON string. On error, the first byte
 *                in this buffer will be set to NUL.
 * @param dstsz   size of output buffer, or 0 for a size request
 * @param flags   Conversion flags: <ul>
 *                <li>@c SAFE - raise an error if could produce
 *                              non-strict UTF-8 output
 *		  </ul>
 * @return number of bytes stored in output buffer (including the trailing
 *         NUL), or the number that would have been stored because
 *         @a dstsz was zero.
 * @retval 0 [EINVAL] The input contains U+DC00..U+DCFF and the
 *                    SAFE flag is set.
 * @retval 0 [EINVAL] The input is malformed UTF-8.
 * @retval 0 [ENOMEM] The output buffer is too small.
 */
static
size_t
string_from_strn(const char *src, int srclen,
    __JSON char *dst, size_t dstsz, int flags)
{
	__JSON char *out = dst;
	__JSON char *out_end = dst + dstsz;
	const char *src_end = src + srclen;
	int outlen = 0;
	ucode lookbehind[2] = { 0, 0 };

	assert(src);

#define OUT(ch) do {						\
		if (out < out_end)				\
			*out++ = (ch);				\
		outlen++;					\
	} while (0)

	OUT('"');
	while (src < src_end) {
		ucode u;
		size_t n;

		n = get_utf8_raw_bounded(src, src_end, &u);
		if (n == 0) {
			errno = EINVAL; /* Bad UTF-8 in source string */
			return 0;
		}
		src += n;

		if ((flags & SAFE) && !IS_UTF8_SAFE(u)) {
			errno = EINVAL;
			return 0;
		}

		/* Avoid some character formations to protect those users
		 * who are generating HTML/XML. (Inspired by OWASP's
		 * JSON sanitizer) */
		if (u == '/' && lookbehind[1] == '<') {
			/* "</" -> "<\/" */
			OUT('\\');
			OUT('/');
		} else if (u == '>' && lookbehind[1] == ']'
				    && lookbehind[0] == ']')
		{
			/* "]]>" -> "]]\u003e" */
			n = put_uescape(u, out, out_end - out);
			outlen += n;
			out += n;
		} else {
			/* It is OK to call put_sanitized_str_escaped() here
			 * even in SAFE mode because the IS_UTF8_SAFE()
			 * above will have caught any DCxx code points. */
			n = put_sanitized_str_escaped(u, out, out_end - out);
			outlen += n;
			out += n;
		}

		lookbehind[0] = lookbehind[1];
		lookbehind[1] = u;
	}
	OUT('"');
	OUT('\0');

	if (dstsz && outlen > dstsz) {
		errno = ENOMEM;
		*dst = '\0';
		return 0;
	}
	return outlen;
}

__PUBLIC
size_t
json_string_from_str(const char *src, __JSON char *dst, size_t dstsz)
{
	if (!src) {
		errno = EINVAL;
		return 0;
	}
	return string_from_strn(src, strlen(src), dst, dstsz, SAFE);
}

__PUBLIC
size_t
json_string_from_strn(const char *src, int srclen,
    __JSON char *dst, size_t dstsz)
{
	return string_from_strn(src, srclen, dst, dstsz, SAFE);
}

__PUBLIC
size_t
json_string_from_unsafe_str(const char *src, __JSON char *dst, size_t dstsz)
{
	if (!src) {
		errno = EINVAL;
		return 0;
	}
	return string_from_strn(src, strlen(src), dst, dstsz, 0);
}

__PUBLIC
size_t
json_string_from_unsafe_strn(const char *src, int srclen,
    __JSON char *dst, size_t dstsz)
{
	return string_from_strn(src, srclen, dst, dstsz, 0);
}
