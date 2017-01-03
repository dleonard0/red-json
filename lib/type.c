#include <ctype.h>

#include "redjson_private.h"

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
	case '-':
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		return JSON_NUMBER;
	default:
		return JSON_BAD;
	}
}
