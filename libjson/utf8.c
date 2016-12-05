#include <errno.h>

#include "libjson_private.h"
#include "utf8.h"

/**
 * Decode a shortest-form UTF-8 sequence (RFC 3629).
 * Checks only for valid encoding and overlong.
 * Does not interpret code points (eg surrogates)
 *
 * @param      p        pointer to NUL-terminated UTF-8 encoded data
 * @param[out] u_return storage for decoded code point
 * @return the number of bytes consumed to store value in @a u_return, or
 *         0 on decoding error (invalid UTF-8 encoding)
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

size_t
get_utf8_raw_bounded(const char *p, const char *p_end, unicode_t *u_return)
{
	size_t expected;

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
 * Encode a unicode codepoint into shortest-form UTF-8 (RFC 3629).
 * Does not treat any code points specially.
 *
 * @param u     a unicode codepoint from U+0000 to U+10FFFF
 * @param buf   (optional) output buffer; required if @a bufsz > 0
 * @param bufsz (optional) size of the output buffer
 * @returns number of bytes that would be stored, or
 *          -1 if the @a u was too large a code point (#EINVAL)
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
 * Reads a UTF-8 character and maps invalid bytes to U+DC00..U+DCFF.
 * @param p_ptr[inout] pointer to read and advance
 * @returns a "sanitized" unicode code point
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
 * Puts a sanitized code point into a dirty UTF-8 output.
 * If the code point is in U+DC00..U+DCFF it is unwrapped back
 * into an "invalid" byte.
 * @param u     Sanitized code point
 * @param buf   Output buffer
 * @param bufsz size of the buffer. No more than this will be stored.
 * @returns  number of bytes that would have been written to buf
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

