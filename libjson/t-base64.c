#include <assert.h>

#include "json.h"
#include "t-assert.h"

/**
 * Asserts that #json_base64_from_bytes() will encode
 * a plaintext into its BASE-64-encoded form.
 * Also assert that it correctly detect boundary errors in buffer sizes.
 *
 * @a param ptext    Plaintext string literal, may contain <code>'\0'</code>
 * @a param etext    BASE-64 encoded text
 */
#define assert_encode_works(ptext, etext)				\
	do {								\
		char _json[1024];					\
		int jlen = strlen(etext) + 2, plen = sizeof(ptext)-1;	\
									\
		/* Passing a too-small dstsz is an error */		\
		assert_errno(json_base64_from_bytes(ptext, plen,	\
			_json, 0) == -1, ENOMEM);			\
		assert_errno(json_base64_from_bytes(ptext, plen,	\
			_json, 1) == -1, ENOMEM);			\
		assert_errno(json_base64_from_bytes(ptext, plen,	\
			_json, 2) == -1, ENOMEM);			\
									\
		/* Passing a small-by-one dst is an error */		\
		assert_errno(json_base64_from_bytes(ptext, plen,	\
			_json, jlen) == -1, ENOMEM);			\
									\
		/* Passing a just-right dstz converts correctly */	\
		memset(_json, '#', sizeof _json);			\
		assert_inteq(json_base64_from_bytes(ptext, plen,	\
			_json, jlen + 1), jlen);			\
		assert_chareq(_json[jlen], '\0');			\
		assert_streq(_json, "\"" etext "\"");			\
									\
		/* Passing an oversized dstsz converts correctly */	\
		memset(_json, '#', sizeof _json);			\
		assert_inteq(json_base64_from_bytes(ptext, plen,	\
			_json, sizeof _json), jlen);			\
		assert_chareq(_json[jlen], '\0');			\
		assert_streq(_json, "\"" etext "\"");			\
	} while (0)

/**
 * Asserts that #json_as_base64() will decode
 * a JSON string into it binary form.
 * Also assert that it correctly detect boundary errors in buffer sizes.
 *
 * @a param ptext    Plaintext string literal, may contain <code>'\0'</code>
 * @a param etext    BASE-64 encoded text (unquoted!)
 */
#define assert_decode_works(ptext, etext)				\
	do {								\
		char _plain[1024];					\
		int plen = sizeof(ptext)-1;				\
									\
		/* Passing an undersizeddstsz converts correctly */	\
		memset(_plain, '#', sizeof _plain);			\
		assert_inteq(json_as_base64("\"" etext "\"",		\
			_plain, sizeof _plain), plen);			\
		assert_memeq(ptext, _plain, plen);			\
	} while (0)

#define assert_encode_and_decode_work(ptext, etext)			\
	do {                                                            \
		assert_encode_works(ptext, etext);                      \
		assert_decode_works(ptext, etext);                      \
	} while (0)
int
main()
{

	char plain[1024];

	/* The JSON_BASE64_DSTSZ() macro computes the right size.
	 * Using the examples from RFC 3548 section 7 and adding
	 * 3 bytes for the two quotes and NUL: */
	assert_inteq(JSON_BASE64_DSTSZ(6), 8 + 3);
	assert_inteq(JSON_BASE64_DSTSZ(5), 8 + 3);
	assert_inteq(JSON_BASE64_DSTSZ(4), 8 + 3);
	assert_inteq(JSON_BASE64_DSTSZ(3), 4 + 3);
	assert_inteq(JSON_BASE64_DSTSZ(2), 4 + 3);
	assert_inteq(JSON_BASE64_DSTSZ(1), 4 + 3);
	assert_inteq(JSON_BASE64_DSTSZ(0), 0 + 3);

	/* Valid examples from RFC 3548 s7 */
	assert_encode_and_decode_work("\x14\xfb\x9c\x03\xd9\x7e", "FPucA9l+");
	assert_encode_and_decode_work("\x14\xfb\x9c\x03\xd9",     "FPucA9k=");
	assert_encode_and_decode_work("\x14\xfb\x9c\x03",         "FPucAw==");

	/* More valid BASE-64 test vectors */
	assert_encode_and_decode_work("", "");
	assert_encode_and_decode_work("hello", "aGVsbG8=");
	assert_encode_and_decode_work("a", "YQ==");
	assert_encode_and_decode_work("ab", "YWI=");
	assert_encode_and_decode_work("abc", "YWJj");
	assert_encode_and_decode_work("\xff\xff\xff", "////");

	/* Every BASE-64 symbol can be used */
	assert_encode_and_decode_work(
	    "\x00\x10\x83\x10\x51\x87\x20\x92\x8b\x30\xd3\x8f\x41\x14\x93\x51"
	    "\x55\x97\x61\x96\x9b\x71\xd7\x9f\x82\x18\xa3\x92\x59\xa7\xa2\x9a"
	    "\xab\xb2\xdb\xaf\xc3\x1c\xb3\xd3\x5d\xb7\xe3\x9e\xbb\xf3\xdf\xbf",
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
	);
	assert_encode_and_decode_work(
	    "\xff\xef\x7c\xef\xae\x78\xdf\x6d\x74\xcf\x2c\x70\xbe\xeb\x6c\xae"
	    "\xaa\x68\x9e\x69\x64\x8e\x28\x60\x7d\xe7\x5c\x6d\xa6\x58\x5d\x65"
	    "\x54\x4d\x24\x50\x3c\xe3\x4c\x2c\xa2\x48\x1c\x61\x44\x0c\x20\x40",
	    "/+9876543210zyxwvutsrqponmlkjihgfedcbaZYXWVUTSRQPONMLKJIHGFEDCBA"
	);

	/* Decoding BASE-64 strings with extra whitespace is allowed */
	assert_decode_works("hello", " a G \\n V s b G 8 \\n = ");
	assert_decode_works("", " \\n ");

	/* String escapes in JSON strings make no difference */
	assert_decode_works("\xff\xff\xff", "\\/\\/\\/\\/");
	assert_decode_works("a", "\\u0059\\u0051\\u003d\\u003d");

	/* Decoding NULL JSON pointers returns EINVAL */
	assert_errno(json_as_base64(NULL, NULL, 0) == -1, EINVAL);
	assert_errno(json_as_base64(NULL, plain, 0) == -1, EINVAL);
	assert_errno(json_as_base64(NULL, plain, sizeof plain) == -1, EINVAL);

	/* Decoding invalid JSON returns EINVAL */
	assert_errno(json_as_base64("", NULL, 0) == -1, EINVAL);
	assert_errno(json_as_base64("", plain, 0) == -1, EINVAL);
	assert_errno(json_as_base64("", plain, sizeof plain) == -1, EINVAL);

	/* Decoding invalid BASE-64 strings returns EINVAL */
	#define assert_decodes_as_invalid(etext) \
		assert_errno(json_as_base64("\"" etext "\"", \
			plain, sizeof plain) == -1, EINVAL)
	assert_decodes_as_invalid("_");
	assert_decodes_as_invalid("!aGVsbG8=");
	assert_decodes_as_invalid("=xxx");
	assert_decodes_as_invalid("xx=x");         /* bad padding */
	assert_decodes_as_invalid("=");
	assert_decodes_as_invalid("x=");
	assert_decodes_as_invalid("==");
	assert_decodes_as_invalid("==x");
	assert_decodes_as_invalid("==xx");
	assert_decodes_as_invalid("x==");
	assert_decodes_as_invalid("x==x");
	assert_decodes_as_invalid("aGVsbG8=aGVs");
	assert_decodes_as_invalid("aGVsbG8=x");
	assert_decodes_as_invalid("aGVsbG8=!");
	assert_decodes_as_invalid("\x80");
	assert_decodes_as_invalid("\xc0\x80");
	assert_decodes_as_invalid("\xf0\x9f\x80\x9c");

	return 0;
}
