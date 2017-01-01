#ifndef LIBJSON_UTF8_H
#define LIBJSON_UTF8_H

#include <stdlib.h>

/** unicode_t is a codepoint within the range U+0 .. U+10FFFF */
typedef unsigned unicode_t;

/**
 * A sanitized unicode character excludes characters from
 * {U+0,U+D800..U+DBFF,U+DD00..U+DFFF,U+110000..} and uses
 * U+DC00..U+DCFF to transport "invalid" bytes.
 *
 * In code, this is designated with the type
 * <code>__SANITIZED unicode_t</code>.
 *
 * This encoding is slightly incompatible with RFC 3629's UTF-8 which
 * excludes the invalid range U+DC00..U+DCFF, and includes U+0.
 * @see #IS_UTF8_SAFE().
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

#define get_utf8_raw_bounded	_red_json_get_utf8_raw_bounded
#define put_utf8_raw		_red_json_put_utf8_raw
#define get_utf8_sanitized	_red_json_get_utf8_sanitized
#define put_sanitized_utf8	_red_json_put_sanitized_utf8

size_t get_utf8_raw_bounded(const char *p, const char *p_end,
				unicode_t *u_return);
size_t put_utf8_raw(unicode_t u, void *buf, int bufsz);
__SANITIZED unicode_t
       get_utf8_sanitized(const char **p_ptr);
size_t put_sanitized_utf8(__SANITIZED unicode_t u, void *buf, int bufsz);

#endif /* LIBJSON_UTF8_H */
