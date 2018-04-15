#include <errno.h>
#include <assert.h>

#include "redjson.h"
#include "t-assert.h"

/* These tests abuse the format strings in ways that GCC
 * can detect and it likes to complain about */
#pragma GCC diagnostic ignored "-Wformat-security"
int
main()
{
	const char input_A[] =
	"{ \"hotel\": ["
	      "null, "
	      "{"
	          "\"cook\": {"
		      "\"name\": \"Mr LeChe\\ufb00\","
		      "\"age\": 91,"
		      "\"cuisine\": \"Fish and chips\","
		  "},"
		  "\"scores\": [4,5, 1, 9, 0]"
	      "}"
	    "]"
	"}";

	const char *cook;
	const char *scores;
	const char *value;

	/* The following are for defeating GCC warnings */
	const char *EMPTY_FORMAT = "";
	const char *SINGLE_PERCENT_FORMAT = "%";

	/* Happy path: selecting the object keyed by "cook" */
	cook = json_select(input_A, "hotel[1].cook");
	assert(cook);
	assert(json_type(cook) == JSON_OBJECT);

	/* Using the same path with a leading . is equivalent */
	assert(json_select(input_A, ".hotel[1].cook") == cook);

	/* Selecting the empty string returns the original value */
	assert(json_select(cook, EMPTY_FORMAT) == cook);

	/* We can select a member within the cook object */
	assert(json_select(input_A, "hotel[1].cook.age") ==
	       json_select(cook, "age"));

	/* Selecting within a NULL source returns ENOENT */
	assert_errno(!json_select(NULL, "height"), ENOENT);
	assert_errno(!json_select(NULL, EMPTY_FORMAT), ENOENT);

	/* Selecting non-existent members returns ENOENT */
	assert_errno(!json_select(cook, "height"), ENOENT);
	assert_errno(!json_select(input_A, "hotel.noexist[1].cook"),
							ENOENT);
	assert_errno(!json_select(input_A, "hotel[1][0].cook"),
							ENOENT);
	assert_errno(!json_select(input_A, "hotel[0].cook"),
							ENOENT);
	assert_errno(!json_select(input_A, "hotel[1].cook.height"),
							ENOENT);
	assert_errno(!json_select(input_A, "hotel[1].owner.age"),
							ENOENT);
	assert_errno(!json_select(input_A, "hotel[7].age"),
							ENOENT);
	assert_errno(!json_select(input_A, "hotel[-1].age"), EINVAL);

	/* An empty key pattern '.' is not permitted */
	assert_errno(!json_select(input_A, "hotel..age"), EINVAL);
	assert_errno(!json_select(input_A, "hotel."), EINVAL);
	assert_errno(!json_select(input_A, "..hotel"), EINVAL);
	assert_errno(!json_select("{\"\":[1]}", ".[0]"), EINVAL);
	assert_errno(!json_select("{\"\":true}", "."), EINVAL);

	/* Empty keys can still be used via .%s */
	assert_errno(!json_select(input_A, "hotel.%s.age", ""), ENOENT);
	assert_errno(!json_select(input_A, "hotel.%s", ""), ENOENT);
	assert_errno(!json_select(input_A, ".%s.hotel", ""), ENOENT);
	value = json_select("{\"\":[1]}", ".%s[0]", "");
	assert_inteq(*value, '1');
	value = json_select("{\"\":true}", ".%s", "");
	assert_inteq(*value, 't');

	/* Happy path using format arguments .%s, [%d] and [%u] */
	scores = json_select(input_A, "%s[1].%s", "hotel", "scores");
	assert(json_type(scores) == JSON_ARRAY);
	assert_inteq(json_select_int(scores, "[%u]", 3), 9);
	assert(json_select(scores, "[%d]", 4));

	/* Can have a leading plus (but no leading minus) */
	assert_inteq(json_select_int(scores, "[+3]"), 9);
	assert_errno(!json_select(scores, "[-4]"), EINVAL);

	/* Variadic negative indices indicate ENOENT (not EINVAL!)
	 * on the reasoning that an index like -1 could be reasonably
	 * used to match no element. Also, if the caller wants negative
	 * checking they should use the preferred [%u] */
	assert_errno(!json_select(scores, "[%d]", -4), ENOENT);
	/* Huge indicies indicate EINVAL */
	assert_errno(!json_select(scores, "[9999999999999999999999999999999]"),
	    ENOENT);

	/* Don't allow % in key literals */
	assert_errno(!json_select("{\"%x\":9", ".%%x"), EINVAL);
	assert_errno(!json_select("{\"%%x\":9", ".%%x"), EINVAL);

	/* Invalid paths */
	assert_errno(!json_select(input_A, "a%s", ""), EINVAL);
	assert_errno(!json_select(input_A, "[%s]", ""), EINVAL);
	assert_errno(!json_select(input_A, "[a]"), EINVAL);
	assert_errno(!json_select(input_A, "[0a]"), EINVAL);
	assert_errno(!json_select(input_A, "[]"), EINVAL);
	assert_errno(!json_select(input_A, "[0%d]", 1), EINVAL);
	assert_errno(!json_select(input_A, "[%d0]", 1), EINVAL);
	assert_errno(!json_select(input_A, "%x", 1), EINVAL);
	assert_errno(!json_select(input_A, SINGLE_PERCENT_FORMAT), EINVAL);


	/* Invalid source appears as ENOENT */
	assert_errno(!json_select(",", "x"), ENOENT);
	assert_errno(!json_select(",", "[0]"), ENOENT);

	/* Invalid JSON behaviour */
	assert_errno(!json_select("{\"a\":1,:,\"x\":0}", "x"), EINVAL);
	assert_errno(!json_select("[0,1,2,:]", "[4]"), EINVAL);


	return 0;
}
