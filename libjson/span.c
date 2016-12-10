#include "libjson_private.h"

__PUBLIC
size_t
json_span(const __JSON char *json)
{
	const __JSON char *json_start = json;

	skip_white(&json);
	if (skip_value(&json) == 0)
		return 0;
	return json - json_start;
}
