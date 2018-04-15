#ifndef REDJSON_T_ASSERT_H
#define REDJSON_T_ASSERT_H

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>		/* NAN, isnan() */

#define cYELLOW  "\033[33m"
#define cRED     "\033[31m"
#define cNORMAL  "\033[m"

#define __static_inline static inline

/** Puts a character using C escaping */
__static_inline void
fputc_esc(int c, FILE *f)
{
	if (c == '"' || c == '\\')
		fprintf(f, cYELLOW "\\%c" cNORMAL, c);
	else if (c >= ' ' && c < 0x7f)
		fputc(c, f);
	else
		fprintf(f, cYELLOW "\\x%02X" cNORMAL, c & 0xff);
}

/** Puts a string using C quoting and escaping */
__static_inline void
fputs_esc(const char *s, FILE *f)
{
	if (!s)
		fputs(cRED "NULL" cNORMAL, f);
	else {
		fputc('"', f);
		while (*s)
			fputc_esc(*s++, f);
		fputc('"', f);
	}
}

/** Puts a byte array */
__static_inline void
fputary(const char *a, size_t n, FILE *f)
{
	if (!a)
		fputs(cRED "NULL" cNORMAL, f);
	else {
		fputc('"', f);
		while (n--)
			fputc_esc(*a++, f);
		fputc('"', f);
	}
}

#define _assert_params \
	const char *file, int line, const char *a_arg, const char *b_arg
#define _assert_args(a_arg, b_arg) \
	__FILE__, __LINE__, a_arg, b_arg

/* A macro to assert two C strings are not NULL and have equal content */
#define assert_streq(a,b) _assert_streq(_assert_args(#a, #b), a, b, "")
#define assert_streqx(a,b,x) _assert_streq(_assert_args(#a, #b), a, b, x)
__static_inline void
_assert_streq(_assert_params, const char *a, const char *b, const char *ext)
{
	if (!a || !b || strcmp(a, b) != 0) {
	    fprintf(stderr,
	       "%s:%d: assertion failure assert_streq(%s, %s) %s\n"
	       "\t%s => ", file, line, a_arg, b_arg, ext, a_arg);
	    fputs_esc(a, stderr);
	    fprintf(stderr, "\n\t%s => ", b_arg);
	    fputs_esc(b, stderr);
	    fprintf(stderr, "\n");
	    abort();
	}
}

/* A macro to assert two byte arrays are not NULL and have equal content */
#define assert_memeq(a,b,n) _assert_memeq(_assert_args(#a, #b), #n, a, b, n)
__static_inline void
_assert_memeq(_assert_params, const char *n_arg, const char *a, const char *b,
    size_t n)
{
	if (!a || !b || memcmp(a, b, n) != 0) {
	    fprintf(stderr,
	       "%s:%d: assertion failure assert_streq(%s, %s, %s)\n"
	       "\t%s => ", file, line, a_arg, b_arg, n_arg, a_arg);
	    fputary(a, n, stderr);
	    fprintf(stderr, "\n\t%s => ", b_arg);
	    fputary(b, n, stderr);
	    fprintf(stderr, "\n\t%s => %zu\n", n_arg, n);
	    abort();
	}
}

/* A macro to assert two integers have equal value */
#define assert_inteq(a,b) _assert_inteq(_assert_args(#a, #b), a, b)
__static_inline void
_assert_inteq(_assert_params, int a, int b)
{
	if (a != b) {
	    fprintf(stderr,
	       "%s:%d: assertion failure assert_inteq(%s, %s)\n"
	       "\t%s => %d\n\t%s => %d\n",
		file, line, a_arg, b_arg,
		a_arg, a,  b_arg, b);
	    abort();
	}
}

/* A macro to assert two long integers have equal value */
#define assert_longeq(a,b) _assert_longeq(_assert_args(#a, #b), a, b)
__static_inline void
_assert_longeq(_assert_params, long a, long b)
{
	if (a != b) {
	    fprintf(stderr,
	       "%s:%d: assertion failure assert_longeq(%s, %s)\n"
	       "\t%s => %ld\n\t%s => %ld\n",
		file, line, a_arg, b_arg,
		a_arg, a,  b_arg, b);
	    abort();
	}
}

/* A macro to assert two doubles have equal value.
 * Comparing with NAN is OK */
#define assert_doubleeq(a,b) _assert_doubleeq(_assert_args(#a, #b), a, b)
__static_inline void
_assert_doubleeq(_assert_params, double a, double b)
{
	if (isnan(a) ? !isnan(b) : isnan(b) ? !isnan(a) : a != b) {
	    fprintf(stderr,
	       "%s:%d: assertion failure assert_doubleeq(%s, %s)\n"
	       "\t%s => %f\n\t%s => %f\n",
		file, line, a_arg, b_arg,
		a_arg, a,  b_arg, b);
	    abort();
	}
}

/* A macro to assert two C chars are equal */
#define assert_chareq(a,b) _assert_chareq(_assert_args(#a, #b), a, b)
__static_inline void
_assert_chareq(_assert_params, char a, char b)
{
	if (a != b) {
	    fprintf(stderr,
	       "%s:%d: assertion failure assert_chareq(%s, %s)\n"
	       "\t%s => '", file, line, a_arg, b_arg, a_arg);
	    fputc_esc(a, stderr);
	    fprintf(stderr, "'\n\t%s => '", b_arg);
	    fputc_esc(b, stderr);
	    fprintf(stderr, "'\n");
	    abort();
	}
}

/* A macro to assert two time_t have equal value */
#define assert_timeeq(a,b) _assert_timeeq(_assert_args(#a, #b), a, b)
__static_inline void
_assert_timeeq(_assert_params, time_t a, time_t b)
{
	if (a != b) {
	    fprintf(stderr,
	       "%s:%d: assertion failure assert_timeeq(%s, %s)\n"
	       "\t%s => %jd\n\t%s => %jd\n\t(difference: %jd)\n",
		file, line, a_arg, b_arg,
		a_arg, (intmax_t)a,  b_arg, (intmax_t)b,
		(intmax_t)(a - b));
	    if (a == -1 || b == -1)
	        fprintf(stderr, "\terrno = %d (%s)\n",
		    errno, strerror(errno));
	    abort();
	}
}

/* A macro to assert two errno's have equal value */
#define assert_errnoeq(a,b) _assert_errnoeq(_assert_args(#a, #b), a, b)
__static_inline void
_assert_errnoeq(_assert_params, int a, int b)
{
	if (a != b) {
	    fprintf(stderr,
	       "%s:%d: assertion failure assert_errnoeq(%s, %s)\n"
	       "\t%s => %d (%s)\n",
		file, line, a_arg, b_arg,
		a_arg, a, a ? strerror(a) : "");
	    fprintf(stderr, "\t%s => %d (%s)\n",
	        b_arg, b, b ? strerror(b) : "");
	    abort();
	}
}

/* A macro to clear errno, then test it has certain value */
#define assert_errno(e, expected_errno) \
        do { \
                errno = 0; \
                assert(e); \
                assert_errnoeq(errno, expected_errno); \
        } while (0)

/* Combines testing for integer equality and errno result */
#define assert_inteq_errno(a, b, expected_errno) \
        do { \
                errno = 0; \
                assert_inteq(a, b); \
                assert_errnoeq(errno, expected_errno); \
        } while (0)

/* Combines testing for long integer equality and errno result */
#define assert_longeq_errno(a, b, expected_errno) \
        do { \
                errno = 0; \
                assert_longeq(a, b); \
                assert_errnoeq(errno, expected_errno); \
        } while (0)

/* Combines testing for double equality and errno result */
#define assert_doubleeq_errno(a, b, expected_errno) \
        do { \
                errno = 0; \
                assert_doubleeq(a, b); \
                assert_errnoeq(errno, expected_errno); \
        } while (0)

#endif /* REDJSON_T_ASSERT_H */
