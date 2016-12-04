#include <errno.h>
#include <assert.h>

#include "json.h"
#include "t-assert.h"

int
main()
{
	const char *i, *e;

	assert((i = json_as_array("[null,1,\"]\",[],{},9e3]")));

	assert((e = json_array_next(&i)));
	assert(json_is_null(e));

	assert((e = json_array_next(&i)));
	assert_inteq(json_as_int(e), 1);

	assert((e = json_array_next(&i)));
	assert(json_strcmp(e, "]") == 0);

	assert((e = json_array_next(&i)));
	assert(json_type(e) == JSON_ARRAY);

	assert((e = json_array_next(&i)));
	assert(json_type(e) == JSON_OBJECT);

	assert((e = json_array_next(&i)));
	assert(json_as_double(e) == 9e3);

	assert(!json_array_next(&i));
	assert(!json_array_next(&i));


	assert_errno(!json_as_array(NULL), EINVAL);
	assert_errno(!json_as_array(""), EINVAL);
	assert_errno(!json_as_array("false"), EINVAL);
	assert_errno(!json_as_array("{}"), EINVAL);
	assert_errno(!json_as_array("]"), EINVAL);

	assert((i = json_as_array("[]")));
	assert(!json_array_next(&i));

	return 0;
}
