#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>

#include "json.h"
#include "t-assert.h"

/** @file
*
 * Tests for:
 *
 *    #json_as_str()
 *    #json_as_strdup()
 *    #json_as_unsafe_str()
 *    #json_as_unsafe_strdup()
 */

static char buf[1024]; /* shared across tests */

/** Asserts that function @a FN treats @a bad_json as invalid input */
#define _assert_fn_inval(FN, bad_json) do {                                \
	/* FN(bad_json,,0) return 0/EINVAL (bufsz request) */              \
        errno = 0;                                                         \
        assert_inteq(FN(bad_json, NULL, 0), 0);                            \
        assert_inteq(errno, EINVAL);                                       \
                                                                           \
        /* FN(bad_json) returns 0/EINVAL with a reasonable buffer */       \
        errno = 0;                                                         \
        memset(buf, '$', sizeof buf);                                      \
        assert_inteq(FN(bad_json, buf, sizeof buf), 0);                    \
        assert_inteq(errno, EINVAL);                                       \
	/* FN(bad_json) still terminated the output buffer */              \
        assert_chareq(buf[0], '\0');                                       \
                                                                           \
        /* FN(bad_json) returns 0/EINVAL with a 1-byte buffer */           \
        errno = 0;                                                         \
        memset(buf, '#', sizeof buf);                                      \
        assert_inteq(FN(bad_json, buf, 1), 0);                             \
        assert_inteq(errno, EINVAL); /* not ENOMEM! */                     \
        assert_chareq(buf[0], '\0');                                       \
                                                                           \
    } while (0)

/** Asserts that function @a FN converts @a json into @a expected */
#define _assert_fn_good(FN, json, expected) do {                           \
        const char *_typecheck __attribute__((unused));                    \
        _typecheck = (expected); /* catch type errors in this test */      \
        assert(sizeof expected <  sizeof buf);                             \
                                                                           \
        /* FN(json,,0) bufsz request matches expected size (incl NUL) */   \
        assert_inteq(FN(json, NULL, 0), sizeof expected);                  \
                                                                           \
        /* FN(json) works with an oversized output buffer */               \
        memset(buf, '$', sizeof buf);                                      \
        assert_inteq(FN(json, buf, sizeof buf), sizeof expected);          \
        assert_streq(buf, expected);                                       \
                                                                           \
        /* FN(json) works with a just-right-sized output buffer */         \
        memset(buf, '$', sizeof buf);                                      \
        assert_inteq(FN(json, buf, sizeof expected), sizeof expected);     \
        assert_streq(buf, expected);                                       \
        assert_chareq(buf[sizeof expected], '$');                          \
                                                                           \
	if ((expected)[0]) {						   \
            /* FN(json) fails with a just-too-small output buffer */       \
            memset(buf, '$', sizeof buf);                                  \
            errno = 0;                                                     \
            assert_inteq(FN(json, buf, 1), 0);                             \
            assert_inteq(errno, ENOMEM);                                   \
            /* It should still have NUL-terminate the output */            \
            assert_chareq(buf[0], '\0');                                   \
            assert_chareq(buf[1], '$');                                    \
        }                                                                  \
    } while (0)


/** Asserts that @a bad_json is correctly treated as invalid JSON
*   by these functions:
 *   #json_as_str()
 *   #json_as_unsafe_str()
 *   #json_as_strdup()
 *   #json_as_unsafe_strdup()
 **/
#define assert_invalid(bad_json) do {					   \
	_assert_fn_inval(json_as_str, bad_json);			   \
	_assert_fn_inval(json_as_unsafe_str, bad_json);			   \
	assert_errno(json_as_strdup(bad_json) == NULL, EINVAL);		   \
	assert_errno(json_as_unsafe_strdup(bad_json) == NULL, EINVAL);	   \
    } while (0)

/** Asserts that @a json is correctly converted to @a expected by
*   these functions:
 *   #json_as_str()
 *   #json_as_unsafe_str()
 *   #json_as_strdup()
 *   #json_as_unsafe_strdup()
 */
#define assert_good(json, expected) do {				   \
	char *_p;							   \
	_assert_fn_good(json_as_str, json, expected);			   \
	_assert_fn_good(json_as_unsafe_str, json, expected);		   \
	assert_streq((_p = json_as_strdup(json)), expected);		   \
	free(_p);							   \
	assert_streq((_p = json_as_unsafe_strdup(json)), expected);	   \
	free(_p);							   \
    } while (0)

/** Asserts that @a unsafe_json is treated as invalid by:
 *   #json_as_str()
 *   #json_as_strdup()
 *  and that it is accepted and converted into @a expected by:
 *   #json_as_unsafe_str()
 *   #json_as_unsafe_strdup()
 */
#define assert_unsafe(unsafe_json, expected) do {			   \
	char *_p;							   \
	_assert_fn_inval(json_as_str, unsafe_json);			   \
	_assert_fn_good(json_as_unsafe_str, unsafe_json, expected);	   \
	assert_errno(json_as_strdup(unsafe_json) == NULL, EINVAL);	   \
	assert_streq((_p = json_as_unsafe_strdup(unsafe_json)), expected); \
	free(_p);							   \
    } while (0)


int
main()
{

	/*
	 * We divide input strings into 3 classes:
	 *   - good     .. good UTF-8 output, adequately formed input
	 *   - unsafe   .. rejected only because they generate bad UTF-8
	 *   - invalid  .. inputs so bad they have to be rejected
	 * and use the macros above to exercise each function over
	 * each of those strings.
	 */

	/* Smoke test */
	assert_good("\"hello\"", "hello");

	/* Malformed inputs */
	assert_invalid(NULL);
	assert_invalid("");

	assert_invalid(",\"ok\"");
	assert_invalid(" ");
	assert_invalid(",\"ok\"");
	assert_invalid(":\"ok\"");
	assert_invalid("[\"ok\"]");
	assert_invalid("{\"ok\":null}");
	assert_invalid("{}");
	assert_invalid("[]");
	assert_invalid("\"");
	assert_invalid(" \"");
	assert_invalid(" \"'");
	assert_invalid(" } \"ok\"");
	assert_invalid(" ] \"ok\"");

	assert_invalid("\"\\u");   /* unterminated strings */
	assert_invalid("\"\\u0");
	assert_invalid("\"\\u00");
	assert_invalid("\"\\u000");
	assert_invalid("\"\\");

	/*
	 * Define some U+DCxx codepoints for use as
	 * as UTF-8 constant strings.
	 *   U+DCxy =     1101     11 00xx     xx yyyy
	 *   UTF-8 = 1110 1110   1011 00xx   10xx yyyy
	 *             E   D     B  (x>>2) (8+x&3) y
	 */
#       define U_DC5C "\xed\xb1\x9c"
#       define U_DC80 "\xed\xb2\x80"
#       define U_DC9C "\xed\xb2\x9c"
#       define U_DCA0 "\xed\xb2\xa0"
#       define U_DCB0 "\xed\xb2\xb0"
#       define U_DCBC "\xed\xb2\xbc"
#       define U_DCBF "\xed\xb2\xbf"
#       define U_DCC0 "\xed\xb3\x80"
#       define U_DCC1 "\xed\xb3\x81"
#       define U_DCC2 "\xed\xb3\x82"
#       define U_DCE1 "\xed\xb3\xa1"
#       define U_DCED "\xed\xb3\xad"
#       define U_DCF1 "\xed\xb3\xb1"
#       define U_DCF7 "\xed\xb3\xb7"

	assert_unsafe("\"\\u\"", U_DC5C "u" );
	assert_unsafe("\"\\u0000\"", U_DC5C "u0000" );
	assert_unsafe("\"\\ud800\"", U_DC5C "ud800");
	assert_unsafe("\"\\udc00\"", U_DC5C "udc00" );
	assert_unsafe("\"\\udc80\"", U_DC5C "udc80" );
	assert_unsafe("\"\\udfff\"", U_DC5C "udfff" );
	assert_unsafe("\"\x80\x80\"", U_DC80 U_DC80);
	assert_unsafe("\"\xc0\x80\"", U_DCC0 U_DC80);
	assert_unsafe("\"\xc1\x80\"", U_DCC1 U_DC80);
	assert_unsafe("\"\xc2\"", U_DCC2);
	assert_unsafe("\"\xe1\"", U_DCE1);
	assert_unsafe("\"\xe1\xbf\"", U_DCE1 U_DCBF);
	assert_unsafe("\"\xf1\"", U_DCF1);
	assert_unsafe("\"\xf1\xbf\"", U_DCF1 U_DCBF);
	assert_unsafe("\"\xf1\xbf\xbf\"", U_DCF1 U_DCBF U_DCBF);
	assert_unsafe("\"\xf7\xbf\xbf\xbf\"", U_DCF7 U_DCBF U_DCBF U_DCBF);


	assert_good("\"\"", "");
	assert_good(" \"\"", "");	/* leading space */
	assert_good(" \"\" ", "");	/* leading and trailing space */
	assert_good(" \"\",", "");	/* trailing comma */
	assert_good("\t\n\r \"hi\"\r", "hi");

	assert_good("\"x\"", "x");
	assert_good("'x'", "x");	/* single quotes */
	assert_good("''", "");		/* single quotes */
	assert_good("x", "x");

	/* UTF-8 encoding tests */
	assert_good("\"\\u0061\"", "a");
	assert_good("\"\\u0001\"", "\1");
	assert_good("\"a\\u0001b\"", "a\1b");
	assert_good("\"\\u007F,\\u0080\"", "\x7f,\xc2\x80");
	assert_good("\"\\u07ff,\\u0800\"", "\xdf\xbf,\xe0\xa0\x80");
	assert_good("\"\\uffff\"", "\xef\xbf\xbf");
	assert_good("\"\\ufffff\"", "\xef\xbf\xbf""f");

	/* Surrogate pair test
	 * U+1F01C (Mahjong tile four of circles)
	 *
	 *              1    f    0    1    c
	 * 1F01C = 0 0001 1111 0000 0001 1100
	 *
	 * UTF-8: .        .   .        .        .   .
	 *        000     011111     000000     011100
	 * (11110)xxx (10)xxxxxx (10)xxxxxx (10)xxxxxx
	 *     f    0      9   f      8   0      9   c
	 *
	 * Surrogate calculation
	 *  subtract 0x10000
	 *    1F01C - 10000 = 0F01C = 0000 1111 0000 0001 1100
	 *  then split into 2 x 10 bits and add to D800, DC00:
	 *    hi = 00 0011 1100 + D800 = D83C
	 *    lo = 00 0001 1100 + DC00 = DC1C
	 *
	 * D83C = (1110)1101 (10)100000 (10)111100 = ED A0 BC
	 * DC1C = (1110)1101 (10)110000 (10)011100 = ED B0 9C
	 */
#       define U_1F01C "\xf0\x9f\x80\x9c"
#	define U_D83C  "\xed\xa0\xbc"
#	define U_DC1C  "\xed\xb0\x9c"

	assert_good("\"\\ud83c\\udc1c\"", U_1F01C);
	assert_good("\"" U_1F01C "\"", U_1F01C);
	assert_unsafe("\"\\udc1c\\ud83c\"", /* reversed surrogates fail */
				U_DC5C "udc1c" U_DC5C "ud83c");
	assert_unsafe("\"" U_D83C U_DC1C "\"", /* inline surrogates fail */
		U_DCED U_DCA0 U_DCBC U_DCED U_DCB0 U_DC9C);

	/* Test of non-structure values like @c null converted to string */
	assert_good("null", "null");	 /* nothing special! */
	assert_good("\"null\"", "null");
	assert_good("'null'", "null");
	assert_good(" null ", "null");
	assert_good("true", "true");
	assert_good("a_b-c+d=e", "a_b-c+d=e");
	assert_good("false", "false");
	assert_good("true'", "true'");   /* prime! */
	assert_good("false'", "false'");
	assert_good("0", "0");
	assert_good(" 1.2 ", "1.2");
	assert_good("-1.8e+99 ", "-1.8e+99");

	assert_good("-", "-");
	assert_good("-:", "-");
	assert_good(" can't ", "can't");
	assert_good(" \\x ", "\\x");
	assert_good(" a\\x ", "a\\x");
	assert_good(" a\\\" ", "a\\"); /* " terminates the bare word */

	assert_good("\"\t\\t\"", "\t\t");
	assert_good("\"\010\\b\"", "\010\010");
	assert_good("\"\n\\n\"", "\n\n");
	assert_good("\"\r\\r\"", "\r\r");
	assert_good("\"\f\\f\"", "\f\f");
	assert_good("\"\\\"\"", "\"");
	assert_good("\"/\\/\"", "//");
	assert_good("\"\\\\\"", "\\");

	return 0;
}
