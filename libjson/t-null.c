#include <assert.h>

#include "json.h"

int
main()
{
	assert(json_is_null("null"));
	assert(json_is_null(" null"));
	assert(json_is_null("null "));
	assert(json_is_null(" null "));

	assert(!json_is_null("\"null\""));
	assert(!json_is_null("\"\""));
	assert(!json_is_null(""));
	assert(!json_is_null("0"));
	assert(!json_is_null("[]"));
	assert(!json_is_null("{}"));
	assert(!json_is_null("[null]"));
	assert(!json_is_null(NULL));
	assert(!json_is_null("nul"));
	assert(!json_is_null("nulll"));
	assert(!json_is_null("nu ll"));

	return 0;
}
