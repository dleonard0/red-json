#include <assert.h>

#include "redjson.h"
#include "t-assert.h"

/**
 * @file
 *
 * Tests for generating quoted JSON strings from UTF-8 C strings.
 *
 *     #json_string_from_str()
 *     #json_string_from_strn()
 *     #json_string_from_unsafe_str()
 *     #json_string_from_unsafe_strn()
 */

char buf[1024];

#define U_DC5C "\xed\xb1\x9c"
#define U_1F01C "\xf0\x9f\x80\x9c"

/* Asserts that FN(src,,buf,) generates the expected json without error */
#define _assert_ok(FN, src, json) do {					\
	    memset(buf, '#', sizeof buf);				\
	    assert_inteq(FN(src, sizeof src - 1, NULL, 0), sizeof json);\
	    assert(buf[0] == '#');					\
	    assert_errno(!FN(src, sizeof src - 1, buf, 1), ENOMEM);	\
	    assert(buf[0] == '\0');					\
	    assert(buf[1] == '#');					\
	    buf[0] = '#';						\
	    assert_inteq(FN(src, sizeof src - 1,			\
		    buf, sizeof buf), sizeof json);			\
	    assert_streqx(buf, json, " from " #FN "(" #src ", ...)");	\
	} while (0)

/* Asserts that FN(src) will return 0/EINVAL */
#define _assert_bad(FN, src) do {					\
	    assert_errno(!FN(src, sizeof src - 1, NULL, 0), EINVAL);	\
	    assert_errno(!FN(src, sizeof src - 1, buf, sizeof buf), EINVAL); \
	} while (0)

#define Q(json) "\"" json "\"" /* simple quoting of a C string literal */

/* The following three macros assert that
 *   json_string_from_unsafe_strn()
 *   json_string_from_strn()
 * correctly classify the src data as safe/unsafe/bad.
 */
#define assert_unsafe(src, json) do {					\
	    _assert_ok(json_string_from_unsafe_strn, src, Q(json));	\
	    _assert_bad(json_string_from_strn, src);			\
	} while (0)

#define assert_safe(src, json) do {					\
	    _assert_ok(json_string_from_unsafe_strn, src, Q(json));	\
	    _assert_ok(json_string_from_strn, src, Q(json));		\
	} while (0)

#define assert_bad(src) do {						\
	    _assert_bad(json_string_from_unsafe_strn, src);		\
	    _assert_bad(json_string_from_strn, src);			\
	} while (0)

int
main()
{
	/* Smoke tests */

	/* We can generate a JSON string from the first 5 chars of some data */
	memset(buf, '#', sizeof buf);
	assert_inteq(json_string_from_strn("helloxx", 5, buf, sizeof buf),
	    sizeof "\"hello\"");
	assert_streq(buf, "\"hello\"");

	memset(buf, '#', sizeof buf);
	assert_inteq(json_string_from_unsafe_strn("helloxx", 5,
	    buf, sizeof buf), sizeof "\"hello\"");
	assert_streq(buf, "\"hello\"");

	/* The _str() variants works just the same way */
	memset(buf, '#', sizeof buf);
	assert_inteq(json_string_from_str("hello", buf, sizeof buf),
	    sizeof "\"hello\"");
	assert_streq(buf, "\"hello\"");
	memset(buf, '#', sizeof buf);
	assert_inteq(json_string_from_unsafe_str("hello", buf, sizeof buf),
	    sizeof "\"hello\"");
	assert_streq(buf, "\"hello\"");

	/* The _str functions don't crash with NULL inputs */
	assert_errno(!json_string_from_str(NULL, buf, sizeof buf), EINVAL);
	assert_errno(!json_string_from_unsafe_str(NULL, buf, sizeof buf),
	    EINVAL);

	/*
	 * NOTE:
	 *    Because the _str() variants are simple wrappers around
	 *    their _strn() partners, the rest of this test only uses
	 *    the _strn() forms.
	 */

	/* Simple strings convert simply */
	assert_safe("i", "i");
	assert_safe("hi", "hi");
	assert_safe("", "");

	/* Whitespace is correctly converted to string escapes \t\r\n */
	assert_safe("\t\r\n \b/\"", "\\t\\r\\n \\b/\\\"");
	/* Low bytes are correctly converted to \uNNNN, incl \0 */
	assert_safe("\0\1\x1f", "\\u0000\\u0001\\u001f");
	/* Backslash input is doubled for a JSON string output */
	assert_safe("\\", "\\\\");
	/* Complex UTF-8 characters are preserved and not escaped */
	assert_safe(U_1F01C, U_1F01C);

	/* Undecodable UTF-8 input sequences are never acceptable */
	assert_bad("\xf0\x9f\x80");
	assert_bad("\xf0\x9f");
	assert_bad("\xf0");
	assert_bad("\x80");
	assert_bad("\xc0\x80");

	/* A decodable but unsafe U+DC5C input converts into a lone backslash */
	assert_unsafe(U_DC5C, "\\");
	assert_unsafe("a"U_DC5C"b", "a\\b");

	return 0;
}
