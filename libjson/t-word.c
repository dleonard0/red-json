#include <assert.h>

/*
 * Tests for the internal word_strcmp() function
 */

#include "libjson_private.h"
#include "t-assert.h"

/* Assert a comparison by word_strcmp() and word_strcmpn() */
#define assert_word_strcmp(a, b, r)					\
	do {								\
		assert_inteq(word_strcmp(a, b), r);			\
		assert_inteq(word_strcmpn(a, b"x", sizeof (b) - 1), r);	\
	} while (0)

int
main()
{
	/* NULL compares the same as "" */
	assert_word_strcmp(NULL, "", 0);
	assert_word_strcmp(NULL, "a", -1);

	/* Malformed JSON compares the same as "" */
	assert_word_strcmp("", "", 0);
	assert_word_strcmp(",", "", 0);

	/* Comparison order works on simple inputs */
	assert_word_strcmp("",  "a", -1);
	assert_word_strcmp("a", "a", 0);
	assert_word_strcmp("a", "", 1);
	assert_word_strcmp("x",  "xa", -1);
	assert_word_strcmp("xa", "xa", 0);
	assert_word_strcmp("xa", "x", 1);

	/* Words terminated by a delimiters compare just the same */
	assert_word_strcmp(",",  "a", -1);
	assert_word_strcmp("a,", "a", 0);
	assert_word_strcmp("a,", "", 1);
	assert_word_strcmp("x,",  "xa", -1);
	assert_word_strcmp("xa,", "xa", 0);
	assert_word_strcmp("xa,", "x", 1);

	return 0;
}
