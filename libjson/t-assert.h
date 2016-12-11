#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define cYELLOW  "\033[33m"
#define cRED     "\033[31m"
#define cNORMAL  "\033[m"

/** Puts a character using C escaping */
inline void
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
inline void
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
inline void
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
#define assert_streq(a,b) _assert_streq(_assert_args(#a, #b), a, b)
inline void
_assert_streq(_assert_params, const char *a, const char *b)
{
	if (!a || !b || strcmp(a, b) != 0) {
	    fprintf(stderr,
	       "%s: %d: assertion failure assert_streq(%s, %s)\n"
	       "\t%s => ", file, line, a_arg, b_arg, a_arg);
	    fputs_esc(a, stderr);
	    fprintf(stderr, "\n\t%s => ", b_arg);
	    fputs_esc(b, stderr);
	    fprintf(stderr, "\n");
	    abort();
	}
}

/* A macro to assert two byte arrays are not NULL and have equal content */
#define assert_memeq(a,b,n) _assert_memeq(_assert_args(#a, #b), #n, a, b, n)
inline void
_assert_memeq(_assert_params, const char *n_arg, const char *a, const char *b, size_t n)
{
	if (!a || !b || memcmp(a, b, n) != 0) {
	    fprintf(stderr,
	       "%s: %d: assertion failure assert_streq(%s, %s, %s)\n"
	       "\t%s => ", file, line, a_arg, b_arg, n_arg, a_arg);
	    fputary(a, n, stderr);
	    fprintf(stderr, "\n\t%s => ", b_arg);
	    fputary(b, n, stderr);
	    fprintf(stderr, "\n\t%s => %zu\n", n_arg, n);
	    abort();
	}
}

/* A macro to assert two C strings are not NULL and have equal content */
#define assert_inteq(a,b) _assert_inteq(_assert_args(#a, #b), a, b)
inline void
_assert_inteq(_assert_params, int a, int b)
{
	if (a != b) {
	    fprintf(stderr,
	       "%s: %d: assertion failure assert_inteq(%s, %s)\n"
	       "\t%s => %d\n\t%s => %d\n",
		file, line, a_arg, b_arg,
		a_arg, a,  b_arg, b);
	    abort();
	}
}

/* A macro to assert two C chars are equal */
#define assert_chareq(a,b) _assert_chareq(_assert_args(#a, #b), a, b)
inline void
_assert_chareq(_assert_params, char a, char b)
{
	if (a != b) {
	    fprintf(stderr,
	       "%s: %d: assertion failure assert_chareq(%s, %s)\n"
	       "\t%s => '", file, line, a_arg, b_arg, a_arg);
	    fputc_esc(a, stderr);
	    fprintf(stderr, "'\n\t%s => '", b_arg);
	    fputc_esc(b, stderr);
	    fprintf(stderr, "'\n");
	    abort();
	}
}

/* A macro to clear errno, then test it has certain value */
#define assert_errno(e, expected_errno) \
        do { \
                errno = 0; \
                assert(e); \
                assert_inteq(errno, expected_errno); \
        } while (0)

