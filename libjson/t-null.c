#include <assert.h>

#include "json.h"

int
main()
{
	/* The bare word null is detected as null */
	assert(json_is_null("null"));
	assert(json_is_null(" null"));
	assert(json_is_null("null "));
	assert(json_is_null(" null "));
	assert(json_is_null("null,"));
	assert(json_is_null("null"));

	/* Other things that are vaguely null-like are rejected */
	assert(!json_is_null("\"null\""));
	assert(!json_is_null("\"\""));
	assert(!json_is_null(""));
	assert(!json_is_null("0"));
	assert(!json_is_null("0null"));
	assert(!json_is_null("null0"));
	assert(!json_is_null("[]"));
	assert(!json_is_null("{}"));
	assert(!json_is_null("[null]"));
	assert(!json_is_null(NULL));
	assert(!json_is_null("NULL"));
	assert(!json_is_null("nul"));
	assert(!json_is_null("nulll"));
	assert(!json_is_null("nu ll"));
	assert(!json_is_null("\\null"));

	return 0;
}
