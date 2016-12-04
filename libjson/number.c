#include <errno.h>
#include <math.h>		/* C99's isnan() and NAN are macros */

#include "libjson_private.h"

static int
is_digit(char ch)
{
	return ch >= '0' && ch <= '9';
}

double
json_as_double(const char *json)
{
	char *end = NULL;
	double number;

	if (!json) {
		errno = EINVAL;
		return NAN;
	}
	skip_white(&json);
	if (*json != '-' && *json != '.' && !is_digit(*json)) {
		/* Not likely a number */
		if (word_strcmpn(json, json_true, 4) == 0) {
			errno = EINVAL;
			return 1;
		}
		if (word_strcmpn(json, json_false, 5) == 0) {
			errno = EINVAL;
			return 0;
		}
		if (*json == '['
		 || *json == '{'
		 || word_strcmpn(json, json_null, 4) == 0)
		{
			errno = EINVAL;
			return NAN;
		}
		if (*json == '"' || *json == '\'') {
			json++;
			skip_white(&json);
		}
		/* treat string content as a number */
	}
	number = strtod(json, &end);
	if (end == json) {
		errno = EINVAL;
		return NAN;
	};
	return number;
}

long
json_as_long(const char *json)
{
	long number;
	int save_errno = errno;
	char *end = NULL;
	double fp;

	if (!json) {
		errno = EINVAL;
		return 0;
	}
	skip_white(&json);

	errno = 0;
	number = strtol(json, &end, 0);
	if (!end || end == json) {
		errno = EINVAL;
		return 0;
	}
	if (*end != '.' && *end != 'e' && *end != 'E') {
		if (!errno)
			errno = save_errno;
		return number;
	}

	errno = 0;
	fp = json_as_double(json);
	if (isnan(fp)) {
		if (!errno) /* explicit NaN converted to integer */
			errno = ERANGE;
		return 0;
	}
	if (fp < LONG_MIN) {
		errno = ERANGE;
		return LONG_MIN;
	}
	if (fp > LONG_MAX) {
		errno = ERANGE;
		return LONG_MAX;
	}
	if (fp == 0 && errno == ERANGE)
		errno = save_errno; /* ignore underflow */
	return fp; /* truncates */
}

int
json_as_int(const char *json)
{
	long number = json_as_long(json);
	if (number > INT_MAX) {
		errno = ERANGE;
		return INT_MAX;
	}
	if (number < INT_MIN) {
		errno = ERANGE;
		return INT_MIN;
	}
	return number;
}
