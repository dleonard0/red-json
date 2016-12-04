#include <errno.h>
#include <math.h>		/* C99's isnan() is a macro */
#include <stdlib.h>
#include <string.h>

#include "libjson_private.h"

__PUBLIC
const char json_true[] = "true";

__PUBLIC
const char json_false[] = "false";

__PUBLIC
int
json_as_bool(const char *json)
{
	if (!json)
		return 0;
	skip_white(&json);
	if (is_delimiter(*json))
		return 0; /* empty */
	if (strchr("+-0.N", *json)) {
		double fp;
		int save_errno = errno;
		char *end = NULL;

		errno = 0;
		fp = strtod(json, &end);
		if (!errno && end && is_delimiter(*end)) {
			errno = save_errno;
			return !(fp == 0 || isnan(fp));
		}
		errno = save_errno;
	}
	if (word_strcmp(json, json_false) == 0)
		return 0;
	if (word_strcmp(json, json_true) == 0)
		return 1;
	if (word_strcmp(json, json_null) == 0)
		return 0;
	if (json[0] == '"' && json[1] == '"')
		return 0;
	if (json[0] == '\'' && json[1] == '\'')
		return 0;
	if (can_skip_delim(&json, '['))
		return (*json == ']') ? 0 : 1;
	if (can_skip_delim(&json, '{'))
		return (*json == '}') ? 0 : 1;

	return 1;
}
