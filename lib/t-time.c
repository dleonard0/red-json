
#include "../redjson.h"
#include "t-assert.h"

#define assert_json_from_time(t, exp_s)  do {\
	buf[0] = '\0'; \
	assert_inteq(json_from_time(t, buf, sizeof buf), 2 + sizeof exp_s - 1); \
	assert_streq(buf, "\"" exp_s "\""); \
    } while (0)

#define assert_json_as_time(s, exp_t) do {\
	errno = 0; \
	assert_timeeq(json_as_time("\"" s "\""), exp_t); \
    } while (0)

#define assert_both_ways(s, t) do { \
	assert_json_as_time(s, t); \
	assert_json_from_time(t, s); \
    } while (0)

#define assert_json_from_time_inval(s) \
	assert_inteq_errno(json_as_time("\"" s "\""), (time_t)-1, EINVAL)

/*
 * Reference dates obtained from GNU coreutils' date(1) command, eg
 *   date -u +%s -d '2000-01-01t00:00:00-01:00'
 *   date -u -d @1000000000 --rfc-3339=seconds
 */

int
main()
{
	char buf[JSON_FROM_TIME_SZ];

	/* The Unix epoch */
	assert_both_ways("1970-01-01T00:00:00Z", 0);
	/* Moments on either side of the epoch */
	assert_both_ways("1970-01-01T00:00:01Z", 1);
	assert_both_ways("1969-12-31T23:59:59Z", -1);
	/* Endpoints of signed 32 bit time_t */
	assert_both_ways("1901-12-13T20:45:52Z", -2147483648);
	assert_both_ways("2038-01-19T03:14:07Z", 2147483647);
	/* Great moments in IRC history */
	assert_both_ways("2001-04-19T04:25:21Z", 987654321);
	assert_both_ways("2001-09-09T01:46:40Z", 1000000000);
	assert_both_ways("2009-02-13T23:31:30Z", 1234567890);
	/* The future is now */
	assert_both_ways("2000-01-01T00:00:00Z", 946684800);

	/* Allow leading and trailing whitespace */
	assert_json_as_time(" 1970-01-01T00:00:01Z", 1);
	assert_json_as_time("1970-01-01T00:00:02Z ", 2);
	assert_json_as_time(" 1970-01-01T00:00:03Z ", 3);

#define Y2K 946684800
	/* Allow a space instead of the T (extension?) */
	assert_json_as_time("2000-01-01 00:00:00Z", Y2K);
	/* Allow lowercase */
	assert_json_as_time("2000-01-01t00:00:00z", Y2K);
	/* Allow fractional seconds */
	assert_json_as_time("2000-01-01 00:00:00.0Z", Y2K);
	assert_json_as_time("2000-01-01 00:00:00.1Z", Y2K);
	assert_json_as_time("2000-01-01 00:00:00.9999Z", Y2K);
	/* Allow time offsets */
	assert_json_as_time("2000-01-01 00:00:00+23:59", Y2K - 23*60*60-59*60);
	assert_json_as_time("2000-01-01 00:00:00+01:00", Y2K - 1*60*60);
	assert_json_as_time("2000-01-01 00:00:00-01:00", Y2K + 1*60*60);
	assert_json_as_time("2000-01-01 00:00:00-01:23", Y2K + 1*60*60+23*60);
	/* Allow leap seconds (but ignore them) */
	assert_json_as_time("1999-12-31 23:59:59Z", Y2K - 1);
	assert_json_as_time("1999-12-31 23:59:60Z", Y2K - 1);
	assert_json_as_time("1999-12-31 23:59:61Z", Y2K - 1);

	/* Don't crash when decoding things */
	assert_inteq_errno(json_as_time("null"), (time_t)-1, EINVAL);
	assert_inteq_errno(json_as_time("false"), (time_t)-1, EINVAL);
	assert_inteq_errno(json_as_time("0"), (time_t)-1, EINVAL);
	assert_inteq_errno(json_as_time(""), (time_t)-1, EINVAL);
	assert_inteq_errno(json_as_time(NULL), (time_t)-1, EINVAL);
	assert_inteq_errno(json_as_time("\""), (time_t)-1, EINVAL);
	assert_inteq_errno(json_as_time("1970-01-01T00:00:00Z"), (time_t)-1, EINVAL);

	/* Don't crash on partial inputs */
	assert_json_from_time_inval("");
	assert_json_from_time_inval("0");
	assert_json_from_time_inval("-");
	assert_json_from_time_inval("T");
	assert_json_from_time_inval("1970");
	assert_json_from_time_inval("1970-");
	assert_json_from_time_inval("1970-01");
	assert_json_from_time_inval("1970-01-");
	assert_json_from_time_inval("1970-01-01");
	assert_json_from_time_inval("1970-01-01T");
	assert_json_from_time_inval("1970-01-01T00");
	assert_json_from_time_inval("1970-01-01T00:");
	assert_json_from_time_inval("1970-01-01T00:00");
	assert_json_from_time_inval("1970-01-01T00:00:");
	assert_json_from_time_inval("1970-01-01T00:00:00");
	assert_json_from_time_inval("1970-01-01T00:00:00.");
	assert_json_from_time_inval("1970-01-01T00:00:00.0");
	/* assert_json_from_time_inval("1970-01-01T00:00:00.Z"); */
	assert_json_from_time_inval("1970-01-01T00:00:00Q");
	assert_json_from_time_inval("1970-01-01T00:00:00ZQ");
	assert_json_from_time_inval("1970-01-01T00:00:00-");
	assert_json_from_time_inval("1970-01-01T00:00:00+");
	assert_json_from_time_inval("1970-01-01T00:00:00+Z");
	assert_json_from_time_inval("1970-01-01T00:00:00+00");
	assert_json_from_time_inval("1970-01-01T00:00:00+00:");
	assert_json_from_time_inval("1970-01-01T00:00:00+00:00Z");

	/* Detect out-of-range values */
	assert_json_from_time_inval("10000-01-01T00:00:00+00:00");
	assert_json_from_time_inval("1970-00-01T00:00:00+00:00");
	assert_json_from_time_inval("1970-13-01T00:00:00+00:00");
	assert_json_from_time_inval("1970-01-00T00:00:00+00:00");
	assert_json_from_time_inval("1970-01-32T00:00:00+00:00");
	assert_json_from_time_inval("1970-01-01T24:00:00+00:00");
	assert_json_from_time_inval("1970-01-01T00:60:00+00:00");
#if 0
	/* Detect offset out-of-range (strict) */
	assert_json_from_time_inval("1970-01-01T00:00:00+24:00");
	assert_json_from_time_inval("1970-01-01T00:00:00-24:00");
	assert_json_from_time_inval("1970-01-01T00:00:00+00:60");
	assert_json_from_time_inval("1970-01-01T00:00:00-00:60");
#endif
	/* Detect shortened numbers */
	assert_json_from_time_inval("970-01-01T00:00:00-00:00");
	assert_json_from_time_inval("1970-1-01T00:00:00-00:00");
	assert_json_from_time_inval("1970-01-1T00:00:00-00:00");
	assert_json_from_time_inval("1970-01-01T0:00:00-00:00");
	assert_json_from_time_inval("1970-01-01T00:0:00-00:00");
	assert_json_from_time_inval("1970-01-01T00:00:0-00:00");
	assert_json_from_time_inval("1970-01-01T00:00:00-0:00");
	assert_json_from_time_inval("1970-01-01T00:00:00-00:0");

	/* Detect a too-small buffer */
	assert_inteq_errno(json_from_time(0, buf, 0), -1, ENOMEM);
	assert_inteq_errno(json_from_time(0, buf, sizeof JSON_FROM_TIME_SZ-1), -1, ENOMEM);
}
