#include <assert.h>

#include "redjson.h"
#include "t-assert.h"

int
main()
{
	/* Happy path: simple JSON strings compare with C equivs, works */
	errno = 0;
	assert_errno(json_strcmp("\"json\"", "json") == 0, 0);

	/* Comparison of strings uses lexicographic ordering */
	assert_errno(json_strcmp("\"jso\"", "json") < 0, 0);
	assert_errno(json_strcmp("\"json\"", "jso") > 0, 0);
	assert_errno(json_strcmp("\"jsoz\"", "json") > 0, 0);
	assert_errno(json_strcmp("\"json\"", "jsoz") < 0, 0);

	/* Comparisons with empty strings don't crash */
	assert(json_strcmp("\"\"", "") == 0);
	assert(json_strcmp("\"a\"", "") > 0);
	assert(json_strcmp("\"\"", "a") < 0);

	/* Comparisons of bare words (not JSON strings) works as strings */
	assert_errno(json_strcmp("true", "true") == 0, EINVAL);
	assert_errno(json_strcmp("false", "true") != 0, EINVAL);
	assert_errno(json_strcmp("key:", "key") == 0, EINVAL);
	assert_errno(json_strcmp("123", "123") == 0, EINVAL);
	assert_errno(json_strcmp("5.0", "5.0") == 0, EINVAL);
	assert_errno(json_strcmp("5.0", "5") > 0, EINVAL);

	/* JSON's null is not special */
	assert_errno(json_strcmp("null", "null") == 0, EINVAL);
	assert_errno(json_strcmp("nullx", "nullx") == 0, EINVAL);
	assert_errno(json_strcmp("nul", "nul") == 0, EINVAL);
	assert_errno(json_strcmp("nul", "null") < 0, EINVAL);

	/* Malformed inputs compare equal to "" */
	assert_errno(json_strcmp(",", "") == 0, EINVAL);
	assert_errno(json_strcmp(NULL, "") == 0, EINVAL);
	assert_errno(json_strcmp("", "") == 0, EINVAL);

	/* Objects and arrays compare equal to "" */
	assert_errno(json_strcmp("{}", "") == 0, EINVAL);
	assert_errno(json_strcmp("[]", "") == 0, EINVAL);

	/* JSON string escapes are decoded before comparison */
	assert(json_strcmp("\"\\u0061\"", "a") == 0);
	assert(json_strcmp("\"\\\"\"", "\"") == 0);
	assert(json_strcmp("\"'\"", "'") == 0);
	assert(json_strcmp("\"error", "") > 0); /* not all of "error is read */

	/* Invalid JSON string escapes become something starting with U+DC5C */
#define DC5C "\xed\xb1\x9c"
	assert(json_strcmp("\"\\u0000\"", DC5C "u0000") == 0);
	assert(json_strcmp("\"\\u0001\"", "\001") == 0);
	assert(json_strcmp("\"\\u00028\"", "\0028") == 0);
	assert(json_strcmp("\"\\u0080\"", "\xc2\x80") == 0);
	assert(json_strcmp("\"\\\\\"", "\\") == 0);
	assert(json_strcmp("\"\\\\\\n\"", "\\\n") == 0);
	assert(json_strcmp("\"\\/\"", "/") == 0);
	assert(json_strcmp("\"\\b\"", "\010") == 0);
	assert(json_strcmp("\"\\f\"", "\xc") == 0);
	assert(json_strcmp("\"\\n\"", "\n") == 0);
	assert(json_strcmp("\"\\r\"", "\r") == 0);
	assert(json_strcmp("\"\\t\"", "\t") == 0);
	assert(json_strcmp("\"a\\\"b\"", "a\"b") == 0);

	return 0;
}
