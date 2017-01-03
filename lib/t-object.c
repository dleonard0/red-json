#include <errno.h>
#include <assert.h>

#include "redjson.h"
#include "t-assert.h"

const char placeholder[] = "placeholder";

int
main()
{
	const char *key, *val;
	const char *i;

	/* Happy path: we can start iterating over this valid object */
	i = json_as_object("{"
			"\"a\": null,"
			"\"true\": true,"
			"\"sub\": {},"
			"\"a\": [1],"
			"\"z\": 0,"
			"}");
	assert(i);

	/* Its first member is "a":null */
	assert((val = json_object_next(&i, &key)));
	assert(json_strcmp(key, "a") == 0);
	assert(json_is_null(val));

	/* Its next member is "true":true */
	assert((val = json_object_next(&i, &key)));
	assert(json_strcmp(key, "true") == 0);
	assert(json_as_bool(val));

	/* Its next member is "sub":{} but we don't care about the key name */
	assert((val = json_object_next(&i, NULL)));
	assert(json_type(val) == JSON_OBJECT);

	/* Its next member is also keyed "a" but has a different value
	 * to the earlier member. */
	assert((val = json_object_next(&i, &key)));
	assert(json_strcmp(key, "a") == 0);
	assert(json_type(val) == JSON_ARRAY);

	/* The last member is "z":0 */
	assert((val = json_object_next(&i, &key)));
	assert(json_strcmp(key, "z") == 0);
	assert_errno(json_as_int(val) == 0, 0);

	/* There are no more members */
	key = NULL;
	assert(!json_object_next(&i, &key));
	assert(!key);

	/* The empty object yields a valid iterator */
	assert((i = json_as_object("{}")));

	/* Stepping to a non-existent member does not modify the key */
	key = placeholder;
	assert(!json_object_next(&i, &key));
	assert(key == placeholder);

	/* An extra comma is allowed in a simple object */
	i = json_as_object("{\"a\":1,}");
	assert(i);
	key = NULL;
	val = json_object_next(&i, &key);
	assert(json_strcmp(key, "a") == 0);
	assert(json_as_int(val) == 1);
	assert(!json_object_next(&i, &key));
	assert(!json_object_next(&i, &key)); /* try it twice */

	/* A bare comma in an empty object yields key=empty value=empty */
	i = json_as_object("{,}");
	assert(i);
	val = json_object_next(&i, &key);
	assert(key); assert(json_span(key) == 0);
	assert(val); assert(json_span(val) == 0);
	/* And a successive iteration fails */
	assert(!json_object_next(&i, &key));

	/* Bad inputs return NULL+EINVAL */
	assert_errno(!json_as_object(NULL), EINVAL);
	assert_errno(!json_as_object(""), EINVAL);
	assert_errno(!json_as_object("[]"), EINVAL);
	assert_errno(!json_as_object("\"{}\""), EINVAL);
	assert_errno(!json_as_object("}"), EINVAL);
	assert_errno(!json_as_object(",{}"), EINVAL);

	/* Stepping a NULL object iterator has no effect */
	i = NULL;
	key = placeholder;
	assert(!json_object_next(&i, &key));
	assert(key == placeholder);

	/* We allow the extension of 'bare' words as keys and values */
	i = json_as_object("{ name:Fred, age:99 }");
	assert(i);
	assert((val = json_object_next(&i, &key)));
	assert(json_strcmp(key, "name") == 0);
	assert(json_strcmp(val, "Fred") == 0);
	assert((val = json_object_next(&i, &key)));
	assert(json_strcmp(key, "age") == 0);
	assert(json_as_int(val) == 99);
	assert(!json_object_next(&i, &key));

	return 0;
}
