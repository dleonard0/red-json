#include <assert.h>
#include <errno.h>

#include "redjson.h"
#include "t-assert.h"

int
main()
{
	/* These convert correctly to boolean without error */
	assert_errno(json_as_bool("true"), 0);
	assert_errno(!json_as_bool("false"), 0);

	/* These convert to 'true' but signal EINVAL */
	assert_errno(json_as_bool("something"), EINVAL);
	assert_errno(json_as_bool("\"false\""), EINVAL);
	assert_errno(json_as_bool("\"1\""), EINVAL);
	assert_errno(json_as_bool("1"), EINVAL);
	assert_errno(json_as_bool("0.1"), EINVAL);
	assert_errno(json_as_bool("-1"), EINVAL);
	assert_errno(json_as_bool("[false]"), EINVAL);
	assert_errno(json_as_bool("{\"bool\":false}"), EINVAL);

	assert_errno(json_as_bool("[ ]"), EINVAL);
	assert_errno(json_as_bool("[,]"), EINVAL);
	assert_errno(json_as_bool("{ }"), EINVAL);
	assert_errno(json_as_bool("{,}"), EINVAL);

	/* These convert to 'false' but signal EINVAL */
	assert_errno(!json_as_bool("undefined"), EINVAL);
	assert_errno(!json_as_bool("null"), EINVAL);
	assert_errno(!json_as_bool("0"), EINVAL);
	assert_errno(!json_as_bool("-0"), EINVAL);
	assert_errno(!json_as_bool("+0"), EINVAL);
	assert_errno(!json_as_bool("0.0"), EINVAL);
	assert_errno(!json_as_bool(".0"), EINVAL);
	assert_errno(!json_as_bool("\"\""), EINVAL);
	assert_errno(!json_as_bool("''"), EINVAL);

	assert_errno(!json_as_bool(NULL), EINVAL);
	assert_errno(!json_as_bool(""), EINVAL);
	assert_errno(!json_as_bool(","), EINVAL);
	assert_errno(!json_as_bool("   :"), EINVAL);

	return 0;
}
