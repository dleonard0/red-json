#include <errno.h>
#include <math.h>		/* C99's isnan() is a macro */
#include <stdlib.h>
#include <string.h>

#include "redjson_private.h"

__PUBLIC
const char json_true[] = "true";

__PUBLIC
const char json_false[] = "false";

__PUBLIC
int
json_as_bool(const char *json)
{
	if (!json) {
		errno = EINVAL;
		return 0;
	}
	skip_white(&json);
	if (word_strcmp(json, json_false) == 0)
		return 0;
	if (word_strcmp(json, json_true) == 0)
		return 1;

	/* Everything after here is EINVAL,
	 * but we can make a best-effort conversion. */
	errno = EINVAL;

	/* Objects and arrays are truthy */
	if (json[0] == '[')
		return 1;
	if (json[0] == '{')
		return 1;
	/* Empty strings are falsey */
	if (json[0] == '"')
		return json[1] != '"';
	if (json[0] == '\'')
		return json[1] != '\'';
	/* Empty JSON expressions are falsey */
	if (is_delimiter(*json))
		return 0; /* empty */
	if (strchr("+-0.N", *json)) {
		double fp;
		char *end = NULL;
		/* NaN and 0 are falsey */
		fp = strtod(json, &end);
		errno = EINVAL; /* strtod() may have set it to ERANGE */
		if (end && is_delimiter(*end))
			return !(fp == 0 || isnan(fp));
	}
	/* null and undefined are falsey */
	if (word_strcmp(json, json_null) == 0)
		return 0;
	if (word_strcmp(json, "undefined") == 0)
		return 0;

	return 1;
}
