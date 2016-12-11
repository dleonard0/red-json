#include <errno.h>

#include "libjson_private.h"
#include "utf8.h"

/**
 * Decodes a shortest-form UTF-8 sequence into a code point.
 *
 * This function rejects invalid UTF-8 encodings including overlong encoding.
 * It does not interpret the code points (eg surrogates).
 *
 * This function is safe for NUL-terminated strings, because if a NUL is
 * encountered, data beyond it will not be accessed. At best, the NUL
 * will be decoded as U+0 and the function will return 1.
 *
 * @param p        pointer to UTF-8 encoded data (NUL is not special)
 * @param u_return storage for resulting decoded code point
 *
 * @returns the number of bytes used from @a p to fill in @a u_return
 * @retval 0 The input was not UTF-8 encoded.
 * @retval 0 The input was an overlong UTF-8 encoding.
 */
static size_t
get_utf8_raw(const char *p, unicode_t *u_return)
{
	if ((p[0] & 0x80) == 0x00) {
		*u_return = p[0];		/* U+0000 .. U+007F */
		return 1;
	}
	if ((p[0] & 0xe0) == 0xc0 &&
	    (p[1] & 0xc0) == 0x80)
	{
		*u_return = (p[1] & 0x3f) |	/* U+0080 .. U+07FF */
			    (p[0] & 0x1f) << 6;
		if (*u_return < 0x80) return 0;
		return 2;
	}
	if ((p[0] & 0xf0) == 0xe0 &&
	    (p[1] & 0xc0) == 0x80 &&
	    (p[2] & 0xc0) == 0x80)
	{
		*u_return = (p[2] & 0x3f) |	/* U+0800 .. U+FFFF */
			    (p[1] & 0x3f) << 6 |
			    (p[0] & 0x0f) << 12;
		if (*u_return < 0x800) return 0;
		return 3;
	}
	if ((p[0] & 0xf8) == 0xf0 &&
	    (p[1] & 0xc0) == 0x80 &&
	    (p[2] & 0xc0) == 0x80 &&
	    (p[3] & 0xc0) == 0x80)
	{
		*u_return = (p[3] & 0x3f) |	/* U+10000 .. U+1FFFFF */
			    (p[2] & 0x3f) << 6 |
			    (p[1] & 0x3f) << 12|
			    (p[0] & 0x07) << 18;
		if (*u_return < 0x10000) return 0;
		return 4;
	}
	return 0;
}

/**
 * Decodes a shortest-form UTF-8 sequence into a code point,
 * with boundary checks.
 *
 * This function rejects invalid UTF-8 encodings including overlong encoding.
 * It does not interpret the code points (eg surrogates).
 *
 * @param p        pointer to UTF-8 encoded data (NUL is not special)
 * @param p_end    end of encoded data. Data at this location will not
 *                 be accessed
 * @param u_return storage for resulting decoded code point
 *
 * @returns the number of bytes consumed from @p to fill in @a u_return
 * @retval 0 The input was not UTF-8 encoded.
 * @retval 0 The input was an overlong UTF-8 encoding.
 * @retval 0 The input was a truncated UTF-8 encoding.
 */
size_t
get_utf8_raw_bounded(const char *p, const char *p_end, unicode_t *u_return)
{
	size_t expected;

	if (p_end == p)
		return 0;
	if ((p[0] & 0x80) == 0x00)
		expected = 1;
	else if ((p[0] & 0xe0) == 0xc0)
		expected = 2;
	else if ((p[0] & 0xf0) == 0xe0)
		expected = 3;
	else if ((p[0] & 0xf8) == 0xf0)
		expected = 4;
	else
		return 0;
	if (p + expected <= p_end)
		return get_utf8_raw(p, u_return);
	return 0;
}

/**
 * Encodes a unicode codepoint directly into shortest-form UTF-8.
 *
 * This function does not treat any code points specially.
 *
 * @param u     a unicode codepoint (U+0..U+10FFFF)
 * @param buf   (optional) output buffer
 * @param bufsz (optional) size of the output buffer
 *
 * @returns number of bytes that were stored in buf, or would have been
 *          stored had there been enough space
 * @retval -1 [EINVAL] The code point @a u was too large
 */
int
put_utf8_raw(unicode_t u, void *buf, size_t bufsz)
{
	unsigned char *out = buf;
	size_t outlen = 0;

#	define OUT(ch)	do {						\
				if (outlen < bufsz)			\
					*out++ = (ch);			\
				outlen++;				\
			} while (0)

	if (u < 0x80) {				/* U+0000 .. U+007F */
		OUT(u);
	} else if (u < 0x800) {			/* U+0080 .. U+07FF */
		OUT(0xc0 | ((u >>  6) & 0x1f));
		OUT(0x80 | ((u >>  0) & 0x3f));
	} else if (u < 0x10000) {		/* U+0800 .. U+FFFF */
		OUT(0xe0 | ((u >> 12) & 0x0f));
		OUT(0x80 | ((u >>  6) & 0x3f));
		OUT(0x80 | ((u >>  0) & 0x3f));
	} else if (u < 0x200000) {		/* U+10000 .. U+1FFFFF */
		OUT(0xf0 | ((u >> 18) & 0x07));
		OUT(0x80 | ((u >> 12) & 0x3f));
		OUT(0x80 | ((u >>  6) & 0x3f));
		OUT(0x80 | ((u >>  0) & 0x3f));
	} else {
		errno = EINVAL;
		return -1;
	}
	return outlen;
#	undef OUT
}

/**
 * Consumes a UTF-8 unicode character, sanitizing invalid UTF-8,
 * and otherwise valid encodings from {U+0,U+D800.U+DFFF}.
 *
 * This function either consumes and returns the next UTF-8 character
 * unaltered, or it consumes the next byte, mapping it into U+DC00..U+DCFF.
 *
 * This function is safe to use with NUL-terminated inputs, as it
 * will not access memory beyond a NUL byte. However, if called with
 * a NUL byte at the pointer, this function will return U+DC00 and advance
 * the pointer over the NUL.
 *
 * @param p_ptr pointer to read and advance. It is always advanced at
 *              least one byte.
 *
 * @returns a sanitized unicode character, never U+0
 */
__SANITIZED unicode_t
get_utf8_sanitized(const char **p_ptr)
{
	size_t n;
	unicode_t u;

	n = get_utf8_raw(*p_ptr, &u);
	if (n == 0 || u == 0 || u > 0x10FFFF || IS_SURROGATE(u)) {
		u = 0xDC00 | (**p_ptr & 0xff);
		n = 1;
	}
	*p_ptr += n;
	return u;
}

/**
 * Stores a sanitized code point into a dirty UTF-8 output buffer.
 *
 * If the code point is in U+DC00..U+DCFF then it will be unwrapped back
 * into an "invalid" byte and stored in the output buffer.
 * Otherwise, the shortest-form UTF-8 encoding of @a u is stored in
 * the buffer.
 *
 * @param u     sanitized code point
 * @param buf   output buffer
 * @param bufsz output buffer size
 *
 * @returns the number of bytes stored in buf, or would have been stored
 *          had there been enough space.
 */
int
put_sanitized_utf8(__SANITIZED unicode_t u, void *buf, size_t bufsz)
{
	if (IS_DCxx(u)) {
		unsigned char *out = buf;
		if (bufsz)
			*out = u & 0xff;
		return 1;
	}
	return put_utf8_raw(u, buf, bufsz);
}
