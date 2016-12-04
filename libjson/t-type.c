#include <assert.h>

#include "json.h"
#include "t-assert.h"

int
main()
{
	assert(json_type("") == JSON_BAD);
	assert(json_type(NULL) == JSON_BAD);
	assert(json_type(":") == JSON_BAD);
	assert(json_type(",") == JSON_BAD);

	assert(json_type("'") == JSON_BAD);
	assert(json_type(" \"foo\"") == JSON_STRING);

	assert(json_type("[]") == JSON_ARRAY);
	assert(json_type("{}") == JSON_OBJECT);

	assert(json_type("null") == JSON_NULL);
	assert(json_type("Null") == JSON_BAD);
	assert(json_type("nul") == JSON_NULL);
	assert(json_type("nu ll") == JSON_NULL);

	assert(json_type("true") == JSON_BOOL);
	assert(json_type("truer") == JSON_BOOL);
	assert(json_type("fals") == JSON_BOOL);
	assert(json_type("false") == JSON_BOOL);

	assert(json_type("-5") == JSON_NUMBER);
	assert(json_type("0") == JSON_NUMBER);
	assert(json_type("0.0") == JSON_NUMBER);
	assert(json_type(".5") == JSON_NUMBER);
	assert(json_type("1e9") == JSON_NUMBER);

	return 0;
}
