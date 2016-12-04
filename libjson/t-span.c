#include <assert.h>

#include "json.h"
#include "t-assert.h"

int
main()
{
	assert(json_span(NULL) == 0);
	assert(json_span("") == 0);

	assert(json_span("0") == 1);
	assert(json_span("0:") == 1);
	assert(json_span("0 ,") == 2);
	assert(json_span(" 0,") == 2);
	assert(json_span(" 0 ,") == 3);
	assert(json_span("1,2 ") == 1);
	assert(json_span("[[[[ ]]]],null") == 9);
	assert(json_span(" null ,") == 6);
	assert(json_span("foo bar") == 4);
	assert(json_span(" \"foo\\\"bar\",") == 11);

	return 0;
}
