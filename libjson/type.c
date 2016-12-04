#include <ctype.h>

#include "libjson_private.h"

__PUBLIC
enum json_type
json_type(const __JSON char *json)
{
	if (!json)
		return JSON_BAD;

	skip_white(&json);

	switch (*json) {
	case '{':
		return JSON_OBJECT;
	case '[':
		return JSON_ARRAY;
	case 't':
	case 'f':
		return JSON_BOOL;
	case 'n':
		return JSON_NULL;
	case '"':
		return JSON_STRING;
	case '+':
	case '-':
	case '.':
		return JSON_NUMBER;
	}

	if (isdigit(*json))
		return JSON_NUMBER;

	return JSON_BAD;
}
