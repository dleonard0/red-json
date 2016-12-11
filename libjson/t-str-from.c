#include <assert.h>

#include "json.h"
#include "t-assert.h"

char buf[1024];

#define U_DC5C "\xed\xb1\x9c"
#define U_1F01C "\xf0\x9f\x80\x9c"

#define _assert_ok(fn, src, json) do {					\
	    memset(buf, '#', sizeof buf);				\
	    assert_inteq(fn(src, sizeof src - 1, NULL, 0), sizeof json);\
	    assert(buf[0] == '#');					\
	    assert_errno(!fn(src, sizeof src - 1, buf, 1), ENOMEM);	\
	    assert(buf[0] == '\0');					\
	    assert(buf[1] == '#');					\
	    buf[0] = '#';						\
	    assert_inteq(fn(src, sizeof src - 1,			\
		    buf, sizeof buf), sizeof json);			\
	    assert_streqx(buf, json, " from " #fn "(" #src ", ...)");	\
	} while (0)

#define _assert_bad(fn, src) do {					\
	    assert_errno(!fn(src, sizeof src - 1, NULL, 0), EINVAL); 	\
	    assert_errno(!fn(src, sizeof src - 1, buf, sizeof buf), EINVAL); \
	} while (0)

#define Q(json) "\"" json "\""
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

	memset(buf, '#', sizeof buf);
	assert_inteq(json_string_from_strn("helloxx", 5, buf, sizeof buf),
	    sizeof "\"hello\"");
	assert_streq(buf, "\"hello\"");

	memset(buf, '#', sizeof buf);
	assert_inteq(json_string_from_str("hello", buf, sizeof buf),
	    sizeof "\"hello\"");
	assert_streq(buf, "\"hello\"");

	memset(buf, '#', sizeof buf);
	assert_inteq(json_string_from_unsafe_strn("helloxx", 5,
	    buf, sizeof buf), sizeof "\"hello\"");
	assert_streq(buf, "\"hello\"");

	memset(buf, '#', sizeof buf);
	assert_inteq(json_string_from_unsafe_str("hello", buf, sizeof buf),
	    sizeof "\"hello\"");
	assert_streq(buf, "\"hello\"");

	/* Range tests */

	assert_safe("i", "i");
	assert_safe("hi", "hi");
	assert_safe("", "");
	assert_safe("\t\r\n \b/\"", "\\t\\r\\n \\b/\\\"");
	assert_safe("\0\1\x1f", "\\u0000\\u0001\\u001f");
	assert_bad("\xf0\x9f\x80");
	assert_bad("\xf0\x9f");
	assert_bad("\xf0");
	assert_bad("\x80");
	assert_bad("\xc0\x80");
	assert_safe("\\", "\\\\");
	assert_unsafe(U_DC5C, "\\");
	assert_unsafe("a"U_DC5C"b", "a\\b");
	assert_safe(U_1F01C, U_1F01C);

	/* NULL tests */
	assert_errno(!json_string_from_str(NULL, buf, sizeof buf), EINVAL);
	assert_errno(!json_string_from_unsafe_str(NULL, buf, sizeof buf), EINVAL);

	return 0;
}
