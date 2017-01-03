#include <errno.h>

#include "redjson_private.h"

__PUBLIC
const __JSON_ARRAYI char *
json_as_array(const char *json)
{
	skip_white(&json);
	if (!can_skip_delim(&json, '[')) {
		errno = EINVAL;
		return NULL;
	}
	return json;
}

__PUBLIC
const __JSON char *
json_array_next(const __JSON_ARRAYI char **ji)
{
	const __JSON char *value;
	const __JSON char **json_ptr = (const __JSON char **)ji;
	int advanced = 0;

	value = *json_ptr;
	if (!value || *value == ']') /* Iterators never point at whitespace */
		return NULL;
	advanced |= skip_value(json_ptr);
	advanced |= can_skip_delim(json_ptr, ',');
	if (!advanced)
		*json_ptr = NULL;
	return value;
}
