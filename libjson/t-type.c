#include <assert.h>

#include "json.h"
#include "t-assert.h"

int
main()
{
	/* Invalid JSON input has type JSON_BAD */
	assert(json_type("") == JSON_BAD);
	assert(json_type(" ") == JSON_BAD);
	assert(json_type(NULL) == JSON_BAD);
	assert(json_type(":") == JSON_BAD);
	assert(json_type(",") == JSON_BAD);

	/* Double quoted strings have type JSON_STRING */
	assert(json_type(" \"foo\"") == JSON_STRING);
	assert(json_type("\"\"") == JSON_STRING);

	/* json_type() guesses conservatively about single quoted strings */
	assert(json_type("'") == JSON_BAD);

	/* Arrays and objects have type JSON_ARRAY and JSON_OBJECT */
	assert(json_type("[]") == JSON_ARRAY);
	assert(json_type("{}") == JSON_OBJECT);

	/* The correct null value has type JSON_NULL */
	assert(json_type("null") == JSON_NULL);
	/* Other values will look like null too by their first letter */
	assert(json_type("Null") == JSON_BAD);
	assert(json_type("nul") == JSON_NULL);
	assert(json_type("nu ll") == JSON_NULL);

	/* Correct true and false have type JSON_BOOL */
	assert(json_type("true") == JSON_BOOL);
	assert(json_type("false") == JSON_BOOL);

	/* Any word starting with t or f will look like JSON_BOOL */
	assert(json_type("truer") == JSON_BOOL);
	assert(json_type("fals") == JSON_BOOL);

	/* Correct numbers look JSON_NUMBER */
	assert(json_type("-5") == JSON_NUMBER);
	assert(json_type("0") == JSON_NUMBER);
	assert(json_type("0.0") == JSON_NUMBER);
	assert(json_type("1e9") == JSON_NUMBER);

	/* Numbers starting with + or . are conservatively JSON_BAD */
	assert(json_type("+1") == JSON_BAD);
	assert(json_type(".5") == JSON_BAD);

	return 0;
}
