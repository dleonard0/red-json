#include <errno.h>
#include <assert.h>

#include "json.h"
#include "t-assert.h"

int
main()
{
	const char *i, *e;

	/* Happy path: iterating over a valid, non-empty array */
	/* starting the iterator doesn't return error/NULL */
	assert((i = json_as_array("[null,1,\"]\",[],{},9e3]")));
	/* its first element is null */
	assert((e = json_array_next(&i)));
	assert(json_is_null(e));
	/* its 2nd element is 1 */
	assert((e = json_array_next(&i)));
	assert_inteq(json_as_int(e), 1);
	/* its 3rd element is the string ']' */
	assert((e = json_array_next(&i)));
	assert(json_strcmp(e, "]") == 0);
	/* its 4th element is an array */
	assert((e = json_array_next(&i)));
	assert(json_type(e) == JSON_ARRAY);
	/* its 5th element is an object */
	assert((e = json_array_next(&i)));
	assert(json_type(e) == JSON_OBJECT);
	/* its 6th is 9000 */
	assert((e = json_array_next(&i)));
	assert(json_as_double(e) == 9e3);
	/* it doesn't have a 7th */
	assert(!json_array_next(&i));
	/* nor an 8th */
	assert(!json_array_next(&i));


	/* Various malformed inputs signal NULL+EINVAL */
	assert_errno(!json_as_array(NULL), EINVAL);
	assert_errno(!json_as_array(""), EINVAL);
	assert_errno(!json_as_array("false"), EINVAL);
	assert_errno(!json_as_array("{}"), EINVAL);
	assert_errno(!json_as_array("]"), EINVAL);

	/* an empty array will iterate */
	errno = 0;
	assert((i = json_as_array("[]")));
	assert(errno == 0);
	/* the empty array has no elements */
	assert(!json_array_next(&i));

	/* An array iterator in error "looks like" an empty array */
	i = NULL;
	assert(!json_array_next(&i));

	return 0;
}
