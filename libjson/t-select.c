#include <errno.h>
#include <assert.h>

#include "json.h"
#include "t-assert.h"

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
	const char *empty = "";

	cook = json_select(input_A, "hotel[1].cook");
	assert(cook);
	assert(json_select(input_A, ".hotel[1].cook") == cook);

	assert(json_type(cook) == JSON_OBJECT);
	assert(json_select(input_A, "hotel[1].cook.age") ==
	       json_select(cook, "age"));

	assert_errno(!json_select(NULL, "height"), ENOENT);
	assert_errno(!json_select(NULL, empty), ENOENT);
	assert(json_select(cook, empty) == cook);

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

	/* "." means an empty string key; eg { "": "foo" } */
	assert_errno(!json_select(input_A, "hotel..age"), ENOENT);
	assert_errno(!json_select(input_A, "hotel."), ENOENT);
	assert_errno(!json_select(input_A, "..hotel"), ENOENT);

	scores = json_select(input_A, "%s[1].scores", "hotel");
	assert(scores);
	assert(json_type(scores) == JSON_ARRAY);
	assert_inteq(json_select_int(scores, "[%u]", 3), 9);
	assert(json_select(scores, "[%d]", 4));
	assert_errno(!json_select(scores, "[%d]", -4), ENOENT);/*not EINVAL!*/

	/* Invalid paths */
	assert_errno(!json_select(input_A, "%"), EINVAL);
	assert_errno(!json_select(input_A, "a%s", ""), EINVAL);
	assert_errno(!json_select(input_A, "[%s]", ""), EINVAL);
	assert_errno(!json_select(input_A, "[a]"), EINVAL);
	assert_errno(!json_select(input_A, "[]"), EINVAL);

	/* Invalid source appears as ENOENT */
	assert_errno(!json_select(",", "x"), ENOENT);
	assert_errno(!json_select(",", "[0]"), ENOENT);

	return 0;
}
