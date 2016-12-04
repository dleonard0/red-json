#include <assert.h>

#include "json.h"
#include "t-assert.h"

int
main()
{

	assert(json_strcmp("\"json\"", "json") == 0);
	assert(json_strcmp("\"jsoz\"", "json") > 0);
	assert(json_strcmp("\"json\"", "jsoz") < 0);
	assert(json_strcmp("\"jsona\"", "json") > 0);
	assert(json_strcmp("\"json\"", "jsona") < 0);
	assert(json_strcmp("\"\"", "") == 0);
	assert(json_strcmp("\"a\"", "") > 0);
	assert(json_strcmp("\"\"", "a") < 0);

	assert(json_strcmp("true", "true") == 0);
	assert(json_strcmp("false", "true") != 0);
	assert(json_strcmp("key:", "key") == 0);
	assert(json_strcmp("123", "123") == 0);

	assert(json_strcmp("nullx", "") > 0);
	assert(json_strcmp("nul", "") > 0);
	assert(json_strcmp("null", "") < 0); /* null < "" */
	assert(json_strcmp("null", NULL) == 0);
	assert(json_strcmp("foo", NULL) > 0);

	assert(json_strcmp("{}", "") < 0);
	assert(json_strcmp("[]", "") < 0);
	assert(json_strcmp(",", "") < 0);
	assert(json_strcmp(NULL, "") < 0);
	assert(json_strcmp("", "") < 0);

	assert(json_strcmp("\"\\u0061\"", "a") == 0);
	assert(json_strcmp("\"\\\"\"", "\"") == 0);
	assert(json_strcmp("\"'\"", "'") == 0);
	assert(json_strcmp("\"error", "") > 0); /* not all of "error is read */

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
