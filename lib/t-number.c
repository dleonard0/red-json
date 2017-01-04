#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <limits.h>
#include <stdarg.h>

#include "redjson.h"
#include "t-assert.h"

/* Return a static buffer containing a printf-formatted string */
__attribute__((format(printf,1,2)))
static const char *
fmt(const char *f, ...)
{
	static char buf[1024];
	va_list ap;
	int ret;

	va_start(ap, f);
	ret = vsnprintf(buf, sizeof buf, f, ap);
	va_end(ap);
	assert(ret >= 0);

	return buf;
}

int
main()
{
	/* Happy path for decoding floating-point numbers */
	assert_doubleeq_errno(json_as_double("0"), 0, 0);
	assert_doubleeq_errno(json_as_double("0.0"), 0, 0);
	assert_doubleeq_errno(json_as_double(" 123"), 123, 0);
	assert_doubleeq_errno(json_as_double(" -123"), -123, 0);
	assert_doubleeq_errno(json_as_double("1e99"), 1e99, 0);
	assert_doubleeq_errno(json_as_double("-1e99"), -1e99, 0);
	assert_doubleeq_errno(json_as_double("-1e-99"), -1e-99, 0);
	assert_doubleeq_errno(json_as_double("-1e+99"), -1e+99, 0);

	/* Happy path for decoding integers */
	assert_inteq_errno(json_as_int(" 123"), 123, 0);
	assert_inteq_errno(json_as_int("123"), 123, 0);
	assert_inteq_errno(json_as_int("-1"), -1, 0);
	assert_inteq_errno(json_as_int("1 e9"), 1, 0); /* space is delimiter */
	assert_inteq_errno(json_as_int("1e3"), 1000, 0);

	/* Happy path for decoding longs */
	assert_longeq_errno(json_as_long("1e+3"), 1000, 0);
	assert_longeq_errno(json_as_long("0"), 0, 0);
	assert_longeq_errno(json_as_long("-50"), -50, 0);

	/* Clamping at boundaries works */
	assert_inteq_errno(json_as_int(fmt("%d", INT_MAX)), INT_MAX, 0);
	assert_inteq_errno(json_as_int(fmt("%d", INT_MIN)), INT_MIN, 0);
	assert_inteq_errno(json_as_int(fmt("%ld", 1L+INT_MAX)), INT_MAX,ERANGE);
	assert_inteq_errno(json_as_int(fmt("%ld", -1L+INT_MIN)),INT_MIN,ERANGE);
	assert_inteq_errno(json_as_int(fmt("%ld", LONG_MAX)), INT_MAX, ERANGE);
	assert_inteq_errno(json_as_int(fmt("%ld", LONG_MIN)), INT_MIN, ERANGE);
	assert_inteq_errno(json_as_int(fmt("%u", UINT_MAX)), INT_MAX, ERANGE);
	assert_inteq_errno(json_as_int(fmt("-%u", UINT_MAX)), INT_MIN, ERANGE);

	assert_longeq_errno(json_as_long(fmt("%ld", LONG_MAX)), LONG_MAX, 0);
	assert_longeq_errno(json_as_long(fmt("%ld", LONG_MIN)), LONG_MIN, 0);
	assert_longeq_errno(json_as_long(fmt("%f", 2.0*LONG_MAX)), LONG_MAX,
	    ERANGE);
	assert_longeq_errno(json_as_long(fmt("%f", 2.0*LONG_MIN)), LONG_MIN,
	    ERANGE);

	/* Some strictly invalid values convert OK but signal EINVAL */
	assert_doubleeq_errno(json_as_double("+123"), 123, EINVAL);
	assert_doubleeq_errno(json_as_double(".5"), 0.5, EINVAL);
	assert_doubleeq_errno(json_as_double("+.5"), 0.5, EINVAL);
	assert_doubleeq_errno(json_as_double(".0"), 0, EINVAL);

	/* Bad numbers forms signal EINVAL */
	assert_doubleeq_errno(json_as_double("."), NAN, EINVAL);
	assert_doubleeq_errno(json_as_double("e1"), NAN, EINVAL);

	/* Different bases work with integer conversion, but signal EINVAL */
	assert_doubleeq_errno(json_as_double("010"), 10, EINVAL);
	assert_doubleeq_errno(json_as_double("0xf"), 0xf, EINVAL);
	assert_longeq_errno(json_as_long("010"), 010, EINVAL);
	assert_longeq_errno(json_as_long("0xf"), 0xf, EINVAL);

	/* Junk after numbers returns best-effort but signals EINVAL */
	assert_doubleeq_errno(json_as_double("123q"), 123, EINVAL);
	assert_longeq_errno(json_as_long("123q"), 123, EINVAL);

	/* Decoding a quoted number works, but signals EINVAL */
	assert_doubleeq_errno(json_as_double("\"123.4\""), 123.4, EINVAL);
	assert_doubleeq_errno(json_as_double("\" 123.4\""), 123.4, EINVAL);
	assert_doubleeq_errno(json_as_double("\" 123.4 \""), 123.4, EINVAL);
	assert_doubleeq_errno(json_as_double("\" 12.34e1 \""), 123.4, EINVAL);
	assert_longeq_errno(json_as_long("\" 123 \""), 123, EINVAL);
	assert_longeq_errno(json_as_long("\" 12.38e1 \""), 123, EINVAL);

	/* Quoted, malformed numbers are always mistrusted and become NAN/0 */
	assert_doubleeq_errno(json_as_double("\"1z\""), NAN, EINVAL);
	assert_longeq_errno(json_as_long("\"1z\""), 0, EINVAL);
	assert_longeq_errno(json_as_long("\"1 z\""), 0, EINVAL);

	/* Escape sequences inside strings not processed as number */
	assert_doubleeq_errno(json_as_double("\"\\u0030\""), NAN, EINVAL);

	/* Decoding other value types yields NaN/EINVAL */
	assert_doubleeq_errno(json_as_double("garbage"), NAN, EINVAL);
	assert_doubleeq_errno(json_as_double("null"), NAN, EINVAL);
	assert_doubleeq_errno(json_as_double("\"\""), NAN, EINVAL);
	assert_doubleeq_errno(json_as_double("\" \""), NAN, EINVAL);
	assert_doubleeq_errno(json_as_double("[123]"), NAN, EINVAL);
	assert_doubleeq_errno(json_as_double("[]"), NAN, EINVAL);
	assert_doubleeq_errno(json_as_double("{\"value\":123}"), NAN, EINVAL);
	assert_doubleeq_errno(json_as_double("{}"), NAN, EINVAL);
	assert_longeq_errno(json_as_long("garbage"), 0, EINVAL);
	assert_longeq_errno(json_as_long("null"), 0, EINVAL);
	assert_longeq_errno(json_as_long("\"\""), 0, EINVAL);
	assert_longeq_errno(json_as_long("\" \""), 0, EINVAL);
	assert_longeq_errno(json_as_long("[123]"), 0, EINVAL);
	assert_longeq_errno(json_as_long("[]"), 0, EINVAL);
	assert_longeq_errno(json_as_long("{\"value\":123}"), 0, EINVAL);
	assert_longeq_errno(json_as_long("{}"), 0, EINVAL);

	/* true is not converted to 1. It is rejected */
	assert_doubleeq_errno(json_as_double("true"), NAN, EINVAL);
	assert_longeq_errno(json_as_long("true"), 0, EINVAL);
	assert_inteq_errno(json_as_int("true"), 0, EINVAL);

	/* other booleans and quotations are rejected as NAN/0 */
	assert_doubleeq_errno(json_as_double("false"), NAN, EINVAL);
	assert_longeq_errno(json_as_long("false"), 0, EINVAL);
	assert_doubleeq_errno(json_as_double("\"true\""), NAN, EINVAL);
	assert_longeq_errno(json_as_long("\"true\""), 0, EINVAL);
	assert_doubleeq_errno(json_as_double("\"false\""), NAN, EINVAL);
	assert_longeq_errno(json_as_long("\"false\""), 0, EINVAL);

	/* Overflows and underflows */
	assert_doubleeq_errno(json_as_double("1e9999"), HUGE_VAL, ERANGE);
	assert_doubleeq_errno(json_as_double("-1e9999"), -HUGE_VAL, ERANGE);
	assert_doubleeq_errno(json_as_double("1e-9999"), 0, ERANGE);
	assert_doubleeq_errno(json_as_double("-1e-9999"), 0, ERANGE);

	/* Overflowing ints and longs are clamped */
	assert_inteq_errno(json_as_int("1e9999"), INT_MAX, ERANGE);
	assert_inteq_errno(json_as_int("1e99"), INT_MAX, ERANGE);
	assert_inteq_errno(json_as_int("-1e99"), INT_MIN, ERANGE);
	assert_inteq_errno(json_as_int("1e199"), INT_MAX, ERANGE);
	assert_inteq_errno(json_as_int("-1e199"), INT_MIN, ERANGE);
	assert_inteq_errno(json_as_int("-1e9999"), INT_MIN, ERANGE);
	assert_longeq_errno(json_as_long("1e9999"), LONG_MAX, ERANGE);
	assert_longeq_errno(json_as_long("1e99"), LONG_MAX, ERANGE);
	assert_longeq_errno(json_as_long("-1e99"), LONG_MIN, ERANGE);
	assert_longeq_errno(json_as_long("1e199"), LONG_MAX, ERANGE);
	assert_longeq_errno(json_as_long("-1e199"), LONG_MIN, ERANGE);
	assert_longeq_errno(json_as_long("-1e9999"), LONG_MIN, ERANGE);

	/* Longs and ints don't underflow */
	assert_inteq_errno(json_as_int("1e-9999"), 0, 0);
	assert_longeq_errno(json_as_long("1e-9999"), 0, 0);

	return 0;
}
