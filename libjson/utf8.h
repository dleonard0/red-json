
#ifndef H_LIBJSON_UTF8
#define H_LIBJSON_UTF8

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

#define get_utf8_raw_bounded	__libjson_get_utf8_raw_bounded
#define put_utf8_raw		__libjson_put_utf8_raw
#define get_utf8_sanitized	__libjson_get_utf8_sanitized
#define put_sanitized_utf8	__libjson_put_sanitized_utf8

size_t get_utf8_raw_bounded(const char *p, const char *p_end,
				unicode_t *u_return);
int    put_utf8_raw(unicode_t u, void *buf, size_t bufsz);
__SANITIZED unicode_t
       get_utf8_sanitized(const char **p_ptr);
int    put_sanitized_utf8(__SANITIZED unicode_t u, void *buf, size_t bufsz);

#endif /* H_LIBJSON_UTF8 */
