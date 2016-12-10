#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

#include "libjson_private.h"

__PUBLIC
const __JSON char *
json_selectv(const __JSON char *json, const char *path, va_list ap)
{
	unsigned index;
	int keylen;
	const char *ji;
	const char *key;
	const char *cur_key;
	int first = 1;

	/* Walk the structure as we process the path components */
	while (json && *path) {
		switch (*path) {
		case '[':
			/* array index component "[int]" */
			path++;
			if (path[0] == '%' && path[1] == 'd') {
				int arg;
				path += 2;
				arg = va_arg(ap, int);
				if (arg < 0) {
					json = NULL;
					break;
				}
				index = arg;
			} else if (path[0] == '%' && path[1] == 'u') {
				path += 2;
				index = va_arg(ap, unsigned int);
			} else {
				index = 0;
				if (!isdigit(*path))
					goto einval;
				while (isdigit(*path)) {
					if (index == (UINT_MAX / 10) &&
					    (*path - '0') > (UINT_MAX % 10))
						goto einval; /* overflow */
					index = index * 10 + (*path - '0');
					path++;
				}
			}
			if (*path++ != ']')
				goto einval;
			/* Skip the first 'index' values in the array */
			ji = json_as_array(json);
			if (!ji)
				goto enoent;
			while ((json = json_array_next(&ji))) {
				if (!index--)
					break;
			}
			break;
		default:
			/* First components simulate a leading '.' */
			if (!first)
				goto einval;
			path--;
		case '.':
			/* Object field component ".key" */
			path++;
			if (path[0] == '%' && path[1] == 's')
			{
				path += 2;
				if (!strchr("[.", *path))
					goto einval;
				key = va_arg(ap, const char *);
				if (!key)
					goto einval;
				keylen = strlen(key);
			} else {
				key = path;
				while (!strchr("[.", *path)) {
					if (*path == '%') {
						path++;
						if (*path != '%')
							goto einval;
					}
					path++;
				}
				keylen = path - key;
			}
			/* Skip to the first matching key in the object */
			ji = json_as_object(json);
			if (!ji)
				goto enoent;
			while ((json = json_object_next(&ji, &cur_key))) {
				if (json_strcmpn(cur_key, key, keylen) == 0)
					break;
			}
			break;
		}
		first = 0;
	}
	if (json) {
		errno = 0;
		return json;
	}
enoent:
	errno = ENOENT;
	return NULL;
einval:
	errno = EINVAL;
	return NULL;
}

__PUBLIC
const __JSON char *
json_select(const __JSON char *json, const char *path, ...)
{
	va_list ap;
	const char *ret;

	va_start(ap, path);
	ret = json_selectv(json, path, ap);
	va_end(ap);
	return ret;
}

/* Implement a json_default_select_* select & convert */
#define IMPL_DEFAULT_SELECT(NAME, T, CONV)				\
    __PUBLIC								\
    T NAME(T default_, const __JSON char *json, const char *path, ...)	\
    {									\
	va_list ap;							\
	const __JSON char *sel;						\
									\
	va_start(ap, path);						\
	sel = json_selectv(json, path, ap);				\
	va_end(ap);							\
	return sel ? CONV(sel) : default_;				\
    }

IMPL_DEFAULT_SELECT(json_default_select_int, int, json_as_int)
IMPL_DEFAULT_SELECT(json_default_select_bool, int, json_as_bool)
IMPL_DEFAULT_SELECT(json_default_select_double, double, json_as_double)
IMPL_DEFAULT_SELECT(json_default_select_array, const __JSON_ARRAYI char *, json_as_array)
IMPL_DEFAULT_SELECT(json_default_select_object, const __JSON_OBJECTI char *, json_as_object)

char *
json_default_select_strdup(const char *default_, const __JSON char *json,
	const char *path, ...)
{
	va_list ap;
	const __JSON char *sel;

	va_start(ap, path);
	sel = json_selectv(json, path, ap);
	va_end(ap);
	return sel ? json_as_strdup(sel) : default_ ? strdup(default_) : NULL;
}
