#include <assert.h>
#include <errno.h>
#include <math.h>
#include <limits.h>

#include "json.h"
#include "t-assert.h"

int
main()
{
	/* The happy path for decoding numbers works */
	assert_errno(json_as_double("0") == 0, 0);
	assert_errno(json_as_double(" 123") == 123, 0);
	assert_errno(json_as_double("1e99") == 1e99, 0);
	assert_errno(json_as_double("-1e99") == -1e99, 0);
	assert_errno(json_as_double("-1e-99") == -1e-99, 0);

	/* Happy path for decoding integers works */
	assert(json_as_int(" 123") == 123);
	assert(json_as_int("123") == 123);
	assert(json_as_int("-1") == -1);
	assert(json_as_int("1 e9") == 1);
	assert(json_as_int("1e3") == 1000);

	/* Happy path for decoding longs works */
	assert(json_as_long("1e3") == 1000);
	assert(json_as_long("0") == 0);
	assert(json_as_long("-50") == -50);

	/* Decoding a quoted number works, but signals EINVAL */
	assert_errno(json_as_double("\"123.4\"") == 123.4, EINVAL);
	assert_errno(json_as_double("\" 123.4\"") == 123.4, EINVAL);
	assert_errno(json_as_double("\" 123.4 \"") == 123.4, EINVAL);

	/* Decoding other value types yields NaN/EINVAL */
	assert_errno(isnan(json_as_double("garbage")), EINVAL);
	assert_errno(isnan(json_as_double("null")), EINVAL);
	assert_errno(isnan(json_as_double("\"\"")), EINVAL);
	assert_errno(isnan(json_as_double("\" \"")), EINVAL);
	assert_errno(isnan(json_as_double("true")), EINVAL);
	assert_errno(isnan(json_as_double("false")), EINVAL);
	assert_errno(isnan(json_as_double("[123]")), EINVAL);
	assert_errno(isnan(json_as_double("[]")), EINVAL);
	assert_errno(isnan(json_as_double("{\"value\":123}")), EINVAL);
	assert_errno(isnan(json_as_double("{}")), EINVAL);

	/* Overflows and underflows */
	assert_errno(json_as_double("1e9999") == HUGE_VAL, ERANGE);
	assert_errno(json_as_double("-1e9999") == -HUGE_VAL, ERANGE);
	assert_errno(json_as_double("1e-9999") == 0, ERANGE);
	assert_errno(json_as_double("-1e-9999") == 0, ERANGE);

	/* Overflowing ints and longs are clamped */
	assert_errno(json_as_int("1e9999") == INT_MAX, ERANGE);
	assert_errno(json_as_int("1e99") == INT_MAX, ERANGE);
	assert_errno(json_as_int("-1e99") == INT_MIN, ERANGE);
	assert_errno(json_as_int("-1e9999") == INT_MIN, ERANGE);
	assert_errno(json_as_long("1e9999") == LONG_MAX, ERANGE);
	assert_errno(json_as_long("1e99") == LONG_MAX, ERANGE);
	assert_errno(json_as_long("-1e99") == LONG_MIN, ERANGE);
	assert_errno(json_as_long("-1e9999") == LONG_MIN, ERANGE);

	/* Longs and ints don't underflow */
	assert_errno(json_as_int("1e-9999") == 0, 0);
	assert_errno(json_as_long("1e-9999") == 0, 0);

	return 0;
}
