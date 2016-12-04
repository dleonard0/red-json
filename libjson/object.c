#include <errno.h>

#include "libjson_private.h"

__PUBLIC
const __JSON_OBJECTI char *
json_as_object(const __JSON char *json)
{
	skip_white(&json);
	if (!can_skip_delim(&json, '{')) {
		errno = EINVAL;
		return NULL;
	}
	return json;
}

__PUBLIC
const __JSON char *
json_object_next(const __JSON_OBJECTI char **ji, const __JSON char **key_return)
{
	const __JSON char *key;
	const __JSON char *value;
	const __JSON char **json_ptr = (const __JSON char **)ji;
	int advanced = 0;

	key = *json_ptr;
	if (!key || *key == '}') /* Iterators never point at whitespace */
		return NULL;
	advanced |= skip_value(json_ptr);
	advanced |= can_skip_delim(json_ptr, ':');
	value = *json_ptr;
	advanced |= skip_value(json_ptr);
	advanced |= can_skip_delim(json_ptr, ',');
	if (!advanced)
		*json_ptr = NULL;

	if (key_return)
		*key_return = key;
	return value;
}
