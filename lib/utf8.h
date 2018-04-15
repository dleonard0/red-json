#ifndef REDJSON_UTF8_H
#define REDJSON_UTF8_H

#include <stdlib.h>

/** ucode is a Unicode code point within the range U+0 .. U+10FFFF */
typedef unsigned int ucode;

/**
 * A sanitized Unicode code point is one restricted to the subset
 * {U+1..U+D7FF,U+DC00..U+DCFF,U+E000..U+10FFFF}.
 *
 * In code, this type is designated <code>__SANITIZED ucode</code>.
 *
 * On U+0: Note that __SANITIZED ucode excludes 0, which
 * means that a UTF-8 encoding of sanitized ucodes (yielding "sanitized
 * UTF-8") can always become a valid C string. Also, a function
 * returning a __SANITIZED ucode can use 0 to indicate exhaustion or
 * an error.
 *
 * The sanitized range is <em>similar but different</em> to the "Unicode
 * Scalar Value" (USV) as used in RFC 3629's UTF-8 and defined by
 * http://www.unicode.org/glossary/#unicode_scalar_value as the range
 * USV = {U+0..U+D7FF,U+E000..U+10FFFF}.
 * @see #IS_UTF8_SAFE() for a standard USV check.
 *
 * The __SANITIZED values U+DC00..U+DCFF are reserved for
 * holding "dirty" bytes from an unsanitary UTF-8 source.
 * Dirty bytes are those forming non-canonical UTF-8, or
 * forming canonical UTF-8 that encodes outside of USV-{U+0}.
 * Each dirty byte is individually mapped into {U+DC00..U+DCFF}.
 * A reverse process recovers the dirty UTF-8 input exactly.
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
__SANITIZED ucode get_utf8_sanitized(const char **p_ptr);
size_t put_sanitized_utf8(__SANITIZED ucode u, void *buf, int bufsz);

#endif /* REDJSON_UTF8_H */
