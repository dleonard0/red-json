#include <assert.h>

#include "json.h"
#include "t-assert.h"

#define assert_both_base64(ptext, etext)				\
	do {								\
		char _json[1024], _plain[1024];				\
		int jlen = strlen(etext) + 2, plen = sizeof(ptext)-1;	\
		memset(_json, '#', sizeof _json);			\
		assert_inteq(json_base64_from_bytes(ptext, plen,	\
			_json, sizeof _json), jlen);			\
		assert_chareq(_json[jlen], '\0');			\
		assert_streq(_json, "\"" etext "\"");			\
									\
		memset(_plain, '#', sizeof _plain);			\
		assert_inteq(json_as_base64("\"" etext "\"",		\
			_plain, sizeof _plain), plen);			\
		assert_memeq(ptext, _plain, plen);			\
	} while (0)

int
main()
{

	char plain[1024];
	char json[1024];

	assert_inteq(JSON_BASE64_DSTSZ(5), 11);
	assert_errno(json_base64_from_bytes("hello", 5, NULL, 0) == -1, ENOMEM);
	assert_errno(json_base64_from_bytes("hello", 5, json, 10)==-1, ENOMEM);
	assert_inteq(json_base64_from_bytes("hello", 5, json, 11), 10);

	assert_errno(json_as_base64(NULL, NULL, 0) == -1, EINVAL);
	assert_errno(json_as_base64(NULL, plain, sizeof plain) == -1, EINVAL);
	assert_errno(json_as_base64("", NULL, 0) == -1, EINVAL);
	assert_errno(json_as_base64("", plain, sizeof plain) == -1, EINVAL);

	assert_both_base64("", "");
	assert_both_base64("hello", "aGVsbG8=");
	assert_both_base64("a", "YQ==");
	assert_both_base64("ab", "YWI=");
	assert_both_base64("abc", "YWJj");
	assert_both_base64("\xff\xff\xff", "////");

	assert_both_base64(
	    "\x00\x10\x83\x10\x51\x87\x20\x92\x8b\x30\xd3\x8f\x41\x14\x93\x51"
	    "\x55\x97\x61\x96\x9b\x71\xd7\x9f\x82\x18\xa3\x92\x59\xa7\xa2\x9a"
	    "\xab\xb2\xdb\xaf\xc3\x1c\xb3\xd3\x5d\xb7\xe3\x9e\xbb\xf3\xdf\xbf",
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
	);
	assert_both_base64(
	    "\xff\xef\x7c\xef\xae\x78\xdf\x6d\x74\xcf\x2c\x70\xbe\xeb\x6c\xae"
	    "\xaa\x68\x9e\x69\x64\x8e\x28\x60\x7d\xe7\x5c\x6d\xa6\x58\x5d\x65"
	    "\x54\x4d\x24\x50\x3c\xe3\x4c\x2c\xa2\x48\x1c\x61\x44\x0c\x20\x40",
	    "/+9876543210zyxwvutsrqponmlkjihgfedcbaZYXWVUTSRQPONMLKJIHGFEDCBA"
	);

	return 0;
}
