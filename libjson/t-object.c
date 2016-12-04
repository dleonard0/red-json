#include <errno.h>
#include <assert.h>

#include "json.h"
#include "t-assert.h"

int
main()
{
	const char *key, *val;
	const char *i;

	assert((i = json_as_object("{"
			"\"a\": null,"
			"\"true\": true,"
			"\"sub\": {}"
			"}")));

	assert((val = json_object_next(&i, &key)));
	assert(json_strcmp(key, "a") == 0);
	assert(json_is_null(val));

	assert((val = json_object_next(&i, NULL)));
	assert(json_as_bool(val));

	assert((val = json_object_next(&i, &key)));
	assert(json_strcmp(key, "sub") == 0);
	assert(json_type(val) == JSON_OBJECT);

	assert(!json_object_next(&i, &key));


	/* empty object */
	assert((i = json_as_object("{}")));
	key = json_null; /* placeholder */
	assert(!json_object_next(&i, &key));
	assert(key == json_null);

	/* bad objects */
	assert_errno(!json_as_object(NULL), EINVAL);
	assert_errno(!json_as_object(""), EINVAL);
	assert_errno(!json_as_object("[]"), EINVAL);
	assert_errno(!json_as_object("\"{}\""), EINVAL);
	assert_errno(!json_as_object("}"), EINVAL);
	assert_errno(!json_as_object(",{}"), EINVAL);

	return 0;
}
