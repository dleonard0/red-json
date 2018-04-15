#include <stdio.h>
#include <errno.h>
#include "private.h"
#include "utf8.h"

/*
 * An RFC3339 date:
 *  - yyyy-mm-ddThh:mm:ss[.sss][Z|[+|-]hh:mm]
 *  - years are 4 digits and always between 0000AD and 9999AD
 *  - Z or local offset to UTC (no timezones nor DST!)
 *      Example: T18:50-04:00 == T22:50Z
 *  - no hour 24, but 60th second is legal.
 *
 * Because posix time_t is defined in a way that leap
 * seconds 'do not exist', there is no need for a TAI-UTC
 * conversion table, but we do have the problem that leapsec
 * 1990-12-31T23:59:60Z cannot have a time_t representation.
 * As a compromise, I decided to decay :60 to :59, and
 * set errno = EOVERFLOW as a warning.
 * http://www.madore.org/~david/computers/unix-leap-seconds.html
 *
 * Implementation note: Locale issues and the non-standardness
 * of timegm() mean that this module implements its own Gregorian
 * calendar conversion code. Calendar code is notoriously hard to
 * get right, so I apologise in advance :)
 */
struct tm3339 {
	int year;
	int month;
	int day;
	int hour;
	int minute;
	int second;
	int offset; /* offset to subtract, in seconds */
};

#define SECONDS_PER_DAY (24 * 60 * 60)

/*
 * Convert a Gregorian date Y-M-D into a Julian day number (JDN).
 * Note: JDN only goes negative for Y < -4713 (ie prior to 4713 BC).
 * From https://en.wikipedia.org/wiki/Julian_day
 * Credit: L E Doggett, per _Calendars_ (Seidelmann 1992)
 */
#define GREGORIAN_TO_JDN(Y, M, D) \
	( (1461 * ((Y) + 4800 + ((M) - 14L) / 12)) /4  \
	+ (367 * ((M) - 2 - 12 * (((M) - 14L) / 12))) / 12 \
	- (3 * (((Y) + 4900 + ((M) - 14L) / 12) / 100)) / 4 \
	+ (D) - 32075 )

/* Compute the unix time at YYYY-MM-DDT00:00:00Z
 * which is the number of (TAI) seconds since 1970-01-01 */
#define GREGORIAN_TO_UNIX(Y, M, D) \
	((GREGORIAN_TO_JDN(Y, M, D) - GREGORIAN_TO_JDN(1970, 1, 1)) \
	 * SECONDS_PER_DAY)

/**
 * Converts a tm3339 structure into the number of TAI seconds
 * since 1970-01-01T00:00:00Z.
 *
 * @param tm RFC3339 structured input date
 * @param t_return storage for the number of TAI seconds since Unix epoch
 *
 * @retval 0 on success
 * @retval 0 [EOVERFLOW] the input was a leap second and was truncated
 * @retval -1 [EINVAL] the structure had invalid month or day values
 */
static int
tm3339_to_time(const struct tm3339 *tm, time_t *t_return)
{
	time_t t = 0;

	if (tm->year < 0 || tm->year > 9999 ||
	    tm->month < 1 || tm->month > 12 ||
	    tm->day < 1 || tm->day > 31 ||
	    /* todo: stricter day of month check, leap year */
	    tm->hour >= 24 || tm->minute >= 60)
	{
		errno = EINVAL;
		return -1;
	}

	t = GREGORIAN_TO_UNIX(tm->year, tm->month, tm->day);

	/* Add the offset from midnight */
	t += tm->hour * 60 * 60;
	t += tm->minute * 60;
	if (tm->second < 60) {
		t += tm->second;
	} else { /* Leap second */
		errno = EOVERFLOW;
		t += 59;
	}

	t -= tm->offset;

	*t_return = t;
	return 0;
}

/**
 * Converts a unix time_t into an RFC3339 date structure.
 * @param t0  number of TAI seconds since the unix epoch
 * @param tm  return storage for converted time
 * @retval -1 [EINVAL] the time_t would lie outside of the
 *                     year range 0000..9999.
 * @return 0 on success.
 */
static int
time_to_tm3339(time_t t0, struct tm3339 *tm)
{
	time_t s;
	long jdn, e, f, g, h;

	/* Check for 0000-01-01 .. 9999-12-31T23:59:59 */
	if (t0 < GREGORIAN_TO_UNIX(0, 1, 1) ||
	    t0 >= GREGORIAN_TO_UNIX(10000, 1, 1))
	{
		errno = EINVAL;
		return -1;
	}

	/* Posix time_t days are always a fixed number of seconds long
	 * (because they don't include leap seconds), so we can calculate
	 * the seconds offset from midnight immediately with a modulus,
	 * and the residue will be the days since the unix epoch. */
	s = t0 % SECONDS_PER_DAY;
	if (s < 0)
		s += SECONDS_PER_DAY;
	t0 -= s;
	tm->second = s % 60; s /= 60;
	tm->minute = s % 60; s /= 60;
	tm->hour = s;

	jdn = (t0 / SECONDS_PER_DAY) + GREGORIAN_TO_JDN(1970, 1, 1);

	/* Convert from Julian day number to Gregorian date.
	 * The algorithm is only valid for non-negative JDNs, which is
	 * the case here because tm->year >= 0 > -4713.
	 * https://en.wikipedia.org/wiki/Julian_day
	 * http://aa.usno.navy.mil/publications/docs/c15_usb_online.pdf
	 * Credit: E G Richards, _Calendars_ (2013) */
	f = jdn + 1401 + (((4 * jdn + 274277) / 146097) * 3) / 4 - 38;
	e = 4 * f + 3;
	g = (e % 1461) / 4;
	h = 5 * g + 2;
	tm->day = ((h % 153)) / 5 + 1;
	tm->month = (h / 153 + 2) % 12 + 1;
	tm->year = (e / 1461) - 4716 + (12 + 2 - tm->month) / 12;

	tm->offset = 0;
	return 0;
}

/**
 * Scans a unicode character from a JSON string.
 * Perfoms escape expansion, and sanitizes the unicode.
 *
 * @param jsonp pointer inside a JSON quoted string
 *
 * @return sanitized Unicode code point
 * @retval 0 end of string reached
 */
static __SANITIZED ucode
scan_ch(const __JSON char **jsonp)
{
	if (!**jsonp || **jsonp == '"')
		return 0;
	return get_escaped_sanitized(jsonp);
}

/**
 * Scans a decimal integer from a JSON string.
 * Leaves the jsonp pointer pointing at the next non-digit character.
 *
 * @param jsonp pointer inside a JSON quoted string
 * @param val_return storage for the scanned decimal value
 *
 * @return length of the scanned integer in digits */
static unsigned int
scan_int(const __JSON char **jsonp, int *val_return)
{
	int val;
	unsigned int len;
	const __JSON char *j;
	__SANITIZED ucode u;

	val = 0;
	len = 0;
	j = *jsonp;
	while ((u = scan_ch(&j)) >= '0' && u <= '9') {
		val = 10 * val + u - '0';
		len++;
		*jsonp = j;
	}
	*val_return = val;
	return len;
}

static int
isdigit(ucode u) {
	return u >= '0' && u <= '9';
}

/**
 * Scans a substring of a JSON string as an RFC3339 date.
 * @param jsonp  pointer into JSON string
 * @param tm     return storage for scanned date
 * @retval 0 success
 * @retval -1 [EINVAL] malformed RFC3339 date
 */
static int
scan_tm3339(const __JSON char **jsonp, struct tm3339 *tm)
{
	__JSON char ch;

	skip_white(jsonp);

	if (scan_int(jsonp, &tm->year) != 4 ||
	    scan_ch(jsonp) != '-' ||
	    scan_int(jsonp, &tm->month) != 2 ||
	    scan_ch(jsonp) != '-' ||
	    scan_int(jsonp, &tm->day) != 2)
		goto fail;
	ch = scan_ch(jsonp);
	if (ch != 't' && ch != 'T' && ch != ' ')
		goto fail;
	if (scan_int(jsonp, &tm->hour) != 2 ||
	    scan_ch(jsonp) != ':' ||
	    scan_int(jsonp, &tm->minute) != 2 ||
	    scan_ch(jsonp) != ':' ||
	    scan_int(jsonp, &tm->second) != 2)
		goto fail;
	ch = scan_ch(jsonp);
	if (ch == '.') { /* Ignore fractional seconds */
		ch = scan_ch(jsonp);
		/* if (!isdigit(ch)) goto fail; */ /* 1*DIGIT */
		while (isdigit(ch))
			ch = scan_ch(jsonp);
	}
	if (ch == '-' || ch == '+') {
		int offset_hour, offset_min;
		if (scan_int(jsonp, &offset_hour) != 2 ||
		    scan_ch(jsonp) != ':' ||
		    scan_int(jsonp, &offset_min) != 2)
			goto fail;
		/* Note: don't check the offset for exceeding 23:59;
		 * it's too strict. ISO 8601 permits 24:00 anyway. */
		tm->offset = offset_hour * 60 * 60 +
			offset_min * 60;
		if (ch == '-')
			tm->offset = -tm->offset;
	} else if (ch == 'z' || ch == 'Z')
		tm->offset = 0;
	else
		goto fail;
	return 0;

fail:
	errno = EINVAL;
	return -1;
}


__PUBLIC
time_t json_as_time(const __JSON char *json)
{
	struct tm3339 tm;
	time_t t;

	skip_white(&json);
	if (!json || *json != '"') {
		errno = EINVAL;
		return -1;
	}

	json++;
	if (scan_tm3339(&json, &tm) == -1)
		return -1;
	skip_white(&json);
	if (*json != '"') {
		errno = EINVAL;
		return -1;
	}
	if (tm3339_to_time(&tm, &t) == -1)
		return -1;
	return t;
}

__PUBLIC
size_t
json_from_time(time_t t, __JSON char *dst, size_t dstsz)
{
	struct tm3339 tm;

	if (dstsz < JSON_FROM_TIME_SZ) {
		errno = ENOMEM;
		return -1;
	}
	if (time_to_tm3339(t, &tm) == -1)
		return -1;
	return snprintf(dst, dstsz, "\"%04u-%02u-%02uT%02u:%02u:%02uZ\"",
		tm.year, tm.month, tm.day,
		tm.hour, tm.minute, tm.second);
}
