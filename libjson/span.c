#include "libjson_private.h"

__PUBLIC
size_t
json_span(const __JSON char *json)
{
	const __JSON char *json_start = json;

	skip_white(&json);
	skip_value(&json);
	return json - json_start;
}
