#include <assert.h>
#include <errno.h>
#include <math.h>
#include <limits.h>

#include "json.h"
#include "t-assert.h"

int
main()
{
	errno = 0;
	assert(json_as_double("0") == 0);
	assert(json_as_double(" 123") == 123);
	assert(json_as_double("1e9") == 1e9);
	assert(json_as_double("-1e9") == -1e9);
	assert(errno == 0);

	errno = 0;
	assert(isnan(json_as_double("garbage")));
	assert(errno == EINVAL);

	errno = 0;
	assert(json_as_int(" 123") == 123);
	assert(json_as_int("123") == 123);
	assert(json_as_int("-1") == -1);
	assert(json_as_int("1 e9") == 1);
	assert(json_as_int("1e3") == 1000);

	assert(json_as_long("1e3") == 1000);
	assert(json_as_long("0") == 0);
	assert(json_as_long("-50") == -50);
	assert(errno == 0);

	errno = 0;
	assert(json_as_int("1e99") == INT_MAX);
	assert(errno == ERANGE);

	errno = 0;
	assert(json_as_int("-1e99") == INT_MIN);
	assert(errno == ERANGE);

	errno = 0;
	assert(json_as_long("1e99") == LONG_MAX);
	assert(errno == ERANGE);

	errno = 0;
	assert(json_as_long("-1e99") == LONG_MIN);
	assert(errno == ERANGE);

	return 0;
}
