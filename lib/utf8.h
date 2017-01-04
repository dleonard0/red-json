#ifndef REDJSON_UTF8_H
#define REDJSON_UTF8_H

#include <stdlib.h>

/** ucode is a Unicode code point within the range U+0 .. U+10FFFF */
typedef unsigned int ucode;

/**
 * A sanitized Unicode code point is a restriction into the subset
 * {U+1..U+D7FF,U+DC00..U+DCFF,U+E000..U+10FFFF}.
 * The codepoints U+DC00..U+DCFF are used to represent "invalid" bytes.
 * U+0 is excluded for use as a string sentinel.
 *
 * In code, this is designated with the type
 * <code>__SANITIZED ucode</code>.
 *
 * This range restriction is slightly different to RFC 3629's UTF-8
 * (and http://www.unicode.org/glossary/#unicode_scalar_value) which is
 * {U+0..U+D7FF,U+E000..U+10FFFF}.
 * @see #IS_UTF8_SAFE() for a standard USV check.
 *
 * Sanitized codepoints transcode losslessly with "dirty UTF-8",
 * where codepoints from the U+DC00..U+DCFF range are mapped directly
 * to invalid bytes in the encoding. In particular, UTF-8 0x00 encodes
 * the sanitized value U+DC00.
 *
 * The lossless or invalid-byte-preserving translation technique is due to
 * Marcus Kuhn ("Substituting malformed UTF-8 sequences in a decoder", 2000).
 */
#define __SANITIZED

#define IS_SURROGATE(u)     (((u) & ~0x7ff) == 0xd800)  /* U+D800 .. U+DFFF */
#define IS_SURROGATE_HI(u)  (((u) & ~0x3ff) == 0xd800)  /* U+D800 .. U+DBFF */
#define IS_SURROGATE_LO(u)  (((u) & ~0x3ff) == 0xdc00)  /* U+DC00 .. U+DFFF */
#define IS_DCxx(u)          (((u) & ~0x0ff) == 0xdc00)  /* U+DC00 .. U+DCFF */
#define IS_UTF8_SAFE(u)     ((u) <= 0x10ffff && !IS_SURROGATE(u))

#define get_utf8_raw_bounded	_redjson_get_utf8_raw_bounded
#define put_utf8_raw		_redjson_put_utf8_raw
#define get_utf8_sanitized	_redjson_get_utf8_sanitized
#define put_sanitized_utf8	_redjson_put_sanitized_utf8

size_t get_utf8_raw_bounded(const char *p, const char *p_end,
				ucode *u_return);
size_t put_utf8_raw(ucode u, void *buf, int bufsz);
__SANITIZED ucode
       get_utf8_sanitized(const char **p_ptr);
size_t put_sanitized_utf8(__SANITIZED ucode u, void *buf, int bufsz);

#endif /* REDJSON_UTF8_H */
