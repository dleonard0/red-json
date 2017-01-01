#include <errno.h>
#include <math.h>		/* C99's isnan() and NAN are macros */
#include <limits.h>		/* {INT,LONG}_{MIN,MAX} */

#include "libjson_private.h"

/* Const-corrected strtod */
static double
const_strtod(const char *nptr, const char **endptr)
{
	/*
	 * The cast is safe, because a pointer derived from `const char *ntptr`
	 * is stored in `*endptr`.
	 *
	 * The library function `double strtod(const char *, char **)`
	 * has the signature it does because C does not permit
	 * automatic conversion of `const char **` to `char **`
	 * in the second argument, but it will convert 'char *' to
	 * 'const char *' in the first argument, which I suppose is
	 * more common.
	 */
	return strtod(nptr, (char **)endptr);
}

__PUBLIC
double
json_as_double(const __JSON char *json)
{
	const __JSON char *end = NULL;
	double number;

	if (!json) {
		errno = EINVAL;
		return NAN;
	}
	skip_white(&json);

	if (*json == '"' || *json == '\'') {
		/*
		 * strtod() does not know how to decode JSON string escapes,
		 * so to do this properly, we would allocate a buffer,
		 * decode the string into it, and then call strtod.
		 *
		 * However, the allocation size is unknown, and allocating
		 * memory like that would be suprising to the caller.
		 *
		 * Anyway, treating quoted numbers as numbers is
		 * an extension of JSON and we're going to return EINVAL,
		 * so I think simply calling strtod directly on the
		 * escaped content of the string will get a reasonable
		 * result for least effort and risk.
		 */
		__JSON char quote = *json++;
		number = const_strtod(json, &end);
		if (end == json)
			number = NAN;
		else {
			/* Guard against tricky inputs such as "0\u0031",
			 * by returning NAN on chars that strtod rejects. */
			skip_white(&end);
			if (*end != quote)
				number = NAN;
		}
		errno = EINVAL;
		return number;
	}

	number = const_strtod(json, &end);
	if (end == json) {
		errno = EINVAL;
		return NAN;
	};
	skip_white(&end);
	if (!is_delimiter(*end))
		errno = EINVAL;
	return number;
}

__PUBLIC
long
json_as_long(const __JSON char *json)
{
	long number;
	int save_errno = errno;
	__JSON char *end = NULL;
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

__PUBLIC
int
json_as_int(const __JSON char *json)
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
