#include <errno.h>

#include "libjson_private.h"
#include "utf8.h"

#define BAD 0xffu  /* Not a BASE-64 symbol */
#define PAD 0xfeu  /* padding '=' */
#define SPC 0xfdu  /* whitespace */

static const
char base64_to_char[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			  "abcdefghijklmnopqrstuvwxyz"
			  "0123456789+/";

static const
unsigned char char_to_base64[256] = {
  /* 00 */ BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD, BAD,SPC,SPC,BAD,SPC,SPC,BAD,BAD,
  /* 10 */ BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,
  /* 20 */ SPC,BAD,BAD,BAD,BAD,BAD,BAD,BAD, BAD,BAD,BAD, 62,BAD,BAD,BAD, 63,
  /* 30 */  52, 53, 54, 55, 56, 57, 58, 59,  60, 61,BAD,BAD,BAD,PAD,BAD,BAD,
  /* 40 */ BAD,  0,  1,  2,  3,  4,  5,  6,   7,  8,  9, 10, 11, 12, 13, 14,
  /* 50 */  15, 16, 17, 18, 19, 20, 21, 22,  23, 24, 25,BAD,BAD,BAD,BAD,BAD,
  /* 60 */ BAD, 26, 27, 28, 29, 30, 31, 32,  33, 34, 35, 36, 37, 38, 39, 40,
  /* 70 */  41, 42, 43, 44, 45, 46, 47, 48,  49, 50, 51,BAD,BAD,BAD,BAD,BAD,
  /* 80 */ BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,
  /* 90 */ BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,
  /* a0 */ BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,
  /* b0 */ BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,
  /* c0 */ BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,
  /* d0 */ BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,
  /* e0 */ BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,
  /* f0 */ BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,
};

int
shift_hex(__JSON char ch, char *out)
{
	char value;

	if (ch >= '0' && ch <= '9')
		value = ch - '0';
	else if (ch >= 'A' && ch <= 'F')
		value = ch - 'A' + 10;
	else if (ch >= 'a' && ch <= 'f')
		value = ch - 'a' + 10;
	else
		return 0;
	*out = (*out << 4) | value;
	return 1;
}

__PUBLIC
int
json_as_base64(const __JSON char *json, void *buf, size_t bufsz)
{
	unsigned char *d = buf, *dend = d + bufsz;

	skip_white(&json);
	if (!json || *json != '"')
		goto invalid;
	json++;

#	define OUT(c) do {					\
		if (bufsz) {					\
			if (d < dend) *d++ = (c);		\
			else goto nomem;			\
		}						\
	} while(0)

	while (*json && *json != '"') {
		unsigned i;
		unsigned char v[4];

		for (i = 0; i < 4; i++) {
			do {
				__JSON char ch = *json++;
				if (ch == '\\') {
					ch = *json++;
					switch (ch) {
					case 'n': ch = '\n'; break;
					case 't': ch = '\t'; break;
					case 'f': ch = '\f'; break;
					case '/': break;
					case 'u': {
					    if (*json++ != '0') goto invalid;
					    if (*json++ != '0') goto invalid;
					    if (!shift_hex(*json++, &ch))
							goto invalid;
					    if (!shift_hex(*json++, &ch))
							goto invalid;
					}
					default:
					    goto invalid;
					}
				}
				v[i] = char_to_base64[(unsigned char)ch];
			} while (v[i] == SPC);
			if (v[i] == BAD)
				goto invalid;
		}
		if (v[0] == PAD || v[1] == PAD)
			goto invalid;
		OUT((v[0] << 2) | (v[1] >> 4));
		if (v[2] == PAD)
			break;
		OUT((v[1] << 4) | (v[2] >> 2));
		if (v[3] == PAD)
			break;
		OUT(v[2] << 6 | v[3]);
	}
	if (*json != '"')
		goto invalid;
	return d - (unsigned char *)buf;
#	undef OUT

invalid:
	errno = EINVAL;
	return -1;
nomem:
	errno = ENOMEM;
	return -1;
}

__PUBLIC
int
json_base64_from_bytes(const void *src, size_t srcsz,
		       __JSON char *dst, size_t dstsz)
{
	const unsigned char *s = src;
	__JSON char *out = dst;

	if (dstsz < JSON_BASE64_DSTSZ(srcsz)) {
		errno = ENOMEM;
		return -1;
	}

#	define OUT(c) *out++ = (c)

	OUT('"');
	while (srcsz >= 3) {
		/* 00000000 11111111 22222222 */
		/* aaaaaabb bbbbcccc ccdddddd */
		unsigned char a;
		unsigned char b;
		unsigned char c;
		unsigned char d;

		a = (s[0] >> 2);
		OUT(base64_to_char[a]);
		b = (s[0] << 4) | (s[1] >> 4);
		OUT(base64_to_char[b & 63]);
		c = (s[1] << 2) | (s[2] >> 6);
		OUT(base64_to_char[c & 63]);
		d = s[2];
		OUT(base64_to_char[d & 63]);
		s += 3;
		srcsz -= 3;
	}
	if (srcsz) {
		unsigned char a;
		unsigned char b;
		unsigned char c;

		a = (s[0] >> 2);
		OUT(base64_to_char[a]);
		if (srcsz == 1) {
			/* 000000 00---- ------ ------ */
			/* aaaaaa bbbbbb ====== ====== */
			b = (s[0] << 4);
			OUT(base64_to_char[b & 63]);
			OUT('=');
			OUT('=');
		} else {
			/* 000000 001111 1111-- ------ */
			/* aaaaaa bbbbbb cccccc ====== */
			b = (s[0] << 4) | (s[1] >> 4);
			OUT(base64_to_char[b & 63]);
			c = s[1] << 2;
			OUT(base64_to_char[c & 63]);
			OUT('=');
		}
	}
	OUT('"');
	*out = '\0';
	return out - dst;
#	undef OUT
}
