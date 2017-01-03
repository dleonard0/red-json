#include <string.h>
#include <assert.h>

#include "redjson.h"
#include "t-assert.h"

/**
 * Returns a nested JSON array of the given depth.
 * <code>[[...[[value]]...]]</code>
 */
const char *
nested_array(unsigned depth, const char *value)
{
	static char json[33000 * 2 + 256];
	unsigned i;
	char *j = json;

	/* static buffer overflow check */
	assert(depth * 2 + strlen(value) + 1 <= sizeof json);

	for (i = 0; i < depth; i++) *j++ = '[';
	while (*value)
		*j++ = *value++;
	for (i = 0; i < depth; i++) *j++ = ']';
	*j = '\0';
	return json;
}

/**
 * Returns a nested JSON object of the given depth.
 * <code>{key:{key:...{key:value}...}}</code>
 */
const char *
nested_object(unsigned depth, const char *key, const char *value)
{
	static char json[33000 * 16 + 256];
	unsigned i;
	char *j = json;

	/* static buffer overflow check */
	assert(depth * (3 + strlen(key)) + strlen(value) + 1 <= sizeof json);

	for (i = 0; i < depth; i++) {
		const char *k = key;
		*j++ = '{';
		while (*k) *j++ = *k++;
		*j++ = ':';
	}
	while (*value) *j++ = *value++;
	for (i = 0; i < depth; i++)
		*j++ = '}';
	*j = '\0';
	return json;
}

int
main()
{
	/* Spans of malformed JSON return 0 */
	assert_errno(json_span(NULL) == 0, EINVAL);
	assert_errno(json_span("") == 0, EINVAL);
	assert_errno(json_span(" ") == 0, EINVAL);
	assert_errno(json_span(",") == 0, EINVAL);
	assert_errno(json_span(":") == 0, EINVAL);
	assert_errno(json_span("]") == 0, EINVAL);
	assert_errno(json_span("}") == 0, EINVAL);

	/* Spans of well-formed JSON are correctly measured up to a delimiter */
	assert(json_span("0") == 1);
	assert(json_span("0:") == 1);
	assert(json_span("0 ,") == 2);
	assert(json_span(" 0,") == 2);
	assert(json_span(" 0 ,") == 3);
	assert(json_span("1,2 ") == 1);
	assert(json_span("[[[[ ]]]],null") == 9);
	assert(json_span(" null ,") == 6);
	assert(json_span("foo bar") == 4);
	assert(json_span(" \"foo\\\"bar\",") == 11);

	/* Deeply nested arrays are correct measured, or return ENOMEM */
	assert(json_span(nested_array(8192, "0")));
	assert(json_span(nested_array(32767, "0")));
	assert(json_span(nested_array(32768, "0")));
	assert_errno(!json_span(nested_array(32769, "0")), ENOMEM);
	assert_errno(!json_span(nested_array(32770, "0")), ENOMEM);

	/* Deeply nested objects are correct measured, or return ENOMEM */
	assert(json_span(nested_object(8192, "\"a\"", "0")));
	assert(json_span(nested_object(32767, "\"a\"", "0")));
	assert(json_span(nested_object(32768, "\"a\"", "0")));
	assert_errno(!json_span(nested_object(32769, "\"a\"", "0")), ENOMEM);
	assert_errno(!json_span(nested_object(32770, "\"a\"", "0")), ENOMEM);

	return 0;
}
