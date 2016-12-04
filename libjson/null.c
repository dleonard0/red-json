#include "libjson_private.h"

__PUBLIC
const char json_null[] = "null";

__PUBLIC
int
json_is_null(const char *json)
{
	if (!json)
		return 0;

	skip_white(&json);
	return word_strcmpn(json, json_null, 4) == 0;
}

