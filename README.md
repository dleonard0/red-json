
# Red JSON

A just-in-time, helpful JSON parser for C.

* No pre-parse. You keep the JSON text in memory until you need the values.
* Seek or iterate a `char *` pointer over nested JSON objects and arrays
  without converting. Fast.
* Call converter functions to get a safe or "best-effort" C value.
  If you choose to ignore errors, the returned value is sensible.
* All functions are `NULL`-tolerant.

## Examples

Let's convert some numbers

```c
    double x = json_as_double("1.24e8");
    double n = json_as_double(NULL); /* NAN, errno==EINVAL */
```

Iterate over an array

```c
    const char *iter;
    const char *elem;

    iter = json_as_array("[1, 2, null, []]");
    while ((elem = json_array_next(&iter)))
        printf("%d\n", json_as_int(elem));    /* could check for 0/EINVAL */
```

Search within a structure

```c
    extern const char *input;   /* See the hotel JSON below */
    const char *cook;
    
    cook = json_select(input, "hotel[1].cook");

    printf("The cook's age is %d\n", json_select_int(cook, "age"));
    for (int i = 0; i < 5; i++)
        printf("score #%d is %d\n", json_select_int(cook, "scores[%d]", i));
```

```json
    {
        "hotel": [
            null,
            {
                "cook": {
                    "name": "Mr LeChe\ufb00",
                    "age": 91,
                    "cuisine": "Fish and chips",
                    "scores": [ 4, 5, 1, 9, 0 ]
                },
            }
        ]
   }
```

Safe strings:

```c
    char buf[1024]; /* UTF-8, NUL-termination guaranteed */

    if (json_as_str(json_select(cook, "name"), buf, sizeof buf))
        printf("The cook's name is %s\n", buf);

    char *cuisine = json_select_strdup(cook, "cuisine");
    if (cuisine) {
        printf("Enjoy your %s\n", cuisine);
        free(cuisine);
    }

    if (!json_as_str("\"\\u0000 \\udc00\"", buf, sizeof buf))
        perror("That UTF-8 is unsafe");

    json_as_unsafe_str("\"\\u0000 \\udc00\"", buf, sizeof buf);
    /* Now buf contains nonstandard UTF-8. */

    /* Best way to carry binary data */
    int buflen = json_as_base64("\"SQNDUDQzNw==\"", buf, sizeof buf);
```

## General usage

The input JSON text should be held NUL-terminated in memory.

Any `const char *` into JSON text represents
just the JSON value to which it points, structured or not.
For example, a pointer to the '`[`' in JSON text `[10,2,3]` represents
the whole array, while a pointer to the '`1`' represents just the
number 10.

Converter functions skip leading whitespace, then converts
the next complete JSON value that they encounter.
Any following data is ignored.

`NULL` is treated the same as empty or malformed input.

### Converters

Converters translate from JSON to C values in a "best effort" or
unsuprising way.
Where no clear conversion is possible, a best-effort return value
is accompanied with `EINVAL` stored in `errno`.

| When converter | Returns | And sets<br>`errno` to | It means |
|-----------|-------------|----------|---------|
|`json_as_array()`|`NULL`|`EINVAL`|not an array `[...]`|
|`json_as_object()`|`NULL`|`EINVAL`|not an object `{...}`|
|`json_as_double()`|`NAN`|`EINVAL`|not a number|
|`json_as_double()`|&plusmn;`HUGE_VAL`|`ERANGE`|overflow|
|`json_as_double()`|0|`ERANGE`|underflow|
|`json_as_long()`|0|`EINVAL`|not a number|
|`json_as_long()`|`LONG_MIN`|`ERANGE`|negative overflow|
|`json_as_long()`|`LONG_MAX`|`ERANGE`|positive overflow|
|`json_as_int()`|0|`EINVAL`|not a number|
|`json_as_int()`|`INT_MIN`|`ERANGE`|negative overflow|
|`json_as_int()`|`INT_MAX`|`ERANGE`|positive overflow|
|`json_as_bool()`|1|`EINVAL`|not a boolean|
|`json_as_bool()`|0|`EINVAL`|not a boolean, but looks falsey|
|`json_as_str()`|0|`EINVAL`|not a valid string (or word)|
|`json_as_str()`|0|`ENOMEM`|output buffer is too small|
|`json_as_strdup()`|`NULL`|`EINVAL`|not a valid string (or word)|
|`json_as_strdup()`|`NULL`|`ENOMEM`|`malloc()` failed|
|`json_as_base64()`|-1|`ENOMEM`|output buffer is too small
|`json_type()`|`JSON_BAD`| |malformed JSON|
|`json_span()`|0| |malformed JSON|

All converters detect `NULL` input as malformed input.

The converter functions treat input JSON values according to this table:

|JSON input|`json_as_`<br>`bool`|`json_as_`<br>`int`/`long`|`json_as_`<br>`double`|`json_as_`<br>`str`|`json_as_`<br>`array`|`json_as_`<br>`object`|`json_is_`<br>`null`|
|--------------|----------|------|------|---------|----|----|----|
|`false`       | 0        |  0   |  NaN |`"false"`|`[]`|`{}`| no |
|`true`        | 1        |  1   |  NaN |`"true"` |`[]`|`{}`| no |
|*number*      | &ne;0    |strtol|strtod|strcpy   |`[]`|`{}`| no |
|*string*      | &ne;""   |strtol|strtod|utf8     |`[]`|`{}`| no |
|`[`*array*`]` | &ne;`[]` |  0   |  NaN |`""`     |=   |`{}`| no |
|`{`*object*`}`| &ne;`{}` |  0   |  NaN |`""`     |`[]`|  = | no |
|`null`        | 0        |  0   |  NaN |`"null"` |`[]`|`{}`| yes|
|*empty*       | 0        |  0   |  NaN |`""`     |`[]`|`{}`| no |

If a converted value would exceed a C type limit, the value will be clamped
(e.g. to `HUGE_VAL` or `INT_MAX`) and `errno` will be set to `ERANGE` or
`ENOMEM` depending on the function.

## String conversion and safety

*Safe UTF-8 strings* contain only shortest-length
encodings of the set of unicode codepoints excluding
{U+0, U+D800&hellip;U+DFFF, U+110000&hellip;}.

The functions `json_as_str()` and `json_as_strdup()` will refuse to
generate unsafe UTF-8 C strings.
Use `json_as_unsafe_str()` `json_as_unsafe_strdup()` instead.

## API summary

Input converters

    double json_as_double(const char *json);
    long   json_as_long(const char *json);
    int    json_as_int(const char *json);
    int    json_as_bool(const char *json);
    int    json_is_null(const char *json);

String input converters

    size_t json_as_str(const char *json, void *buf, size_t bufsz);
    char * json_as_strdup(const char *json);

    int    json_as_base64(const char *json, void *buf, size_t bufsz);
    size_t json_as_unsafe_str(const char *json, void *buf, size_t bufsz);
    char * json_as_unsafe_strdup(const char *json);

Structure iterators

    const char *json_as_array(const char *json);
    const char *json_array_next(const char **index_p);
    const char *json_as_object(const char *json);
    const char *json_object_next(const char **index_p, const char **key_ret);

Structure seeking

    const char *json_select(const char *json, const char *path, ...);
    const char *json_selectv(const char *json, const char *path, va_list ap);

Misc

    enum json_type json_type(const char *json);
    size_t json_span(const char *json);

    int json_strcmp(const char *json, const char *cstr);
    int json_strcmpn(const char *json, const char *cstr, size_t cstrsz);

    extern const char json_true[];   /* "true" */
    extern const char json_false[];  /* "false" */
    extern const char json_null[];   /* "null" */

Generators

    size_t json_string_from_str(const char *src,
                                char *dst, size_t dstsz);
    size_t json_string_from_strn(const char *src, int srclen,
                                char *dst, size_t dstsz);
    size_t json_string_from_unsafe_str(const char *src,
                                char *dst, size_t dstsz);
    size_t json_string_from_unsafe_strn(const char *src, int srclen,
                                char *dst, size_t dstsz);


## Standards and extensions

This parser implements RFC 7159 with the following extensions:

* Extra commas may be present at the end of arrays and objects
* Strings may be also be single-quoted.
  Inside a single quoted `'` strings, the double quote `"` need not be
  escaped (`\"`), but single quotes must be (`\'`).
* Unquoted strings called 'words' (matching `[^[]{},:" \t\n\r]+`) are
  accepted where strings are expected.
  They are treated like quoted strings, except that backslash-escapes are
  ignored. Words may contin single quotes, but cannot begin with one.

### Parser limits

* UTF-8 only (no UTF-16 nor UTF-32)
* Nested arrays and objects are limited to a combined depth of 32768 levels


