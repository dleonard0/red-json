
# Red JSON

A just-in-time, helpful JSON parser for C.

* No pre-parse. You keep the JSON text in memory until you need the values.
* Seek or iterate a `char *` pointer over nested JSON objects and arrays
  without converting.
* Converter functions turn a JSON value into a safe or "best-effort" C value.
  If you ignore errors, the returned value is probably sensible.
* Functions are`NULL`-tolerant.
  NULL is treated the same as malformed or empty JSON input.

## Examples

Let's convert some numbers:

```c
    double x = json_as_double("1.24e8");
    double n = json_as_double(NULL); /* NAN, errno==EINVAL */
```

Iterate over an array:

```c
    const char *iter;
    const char *elem;

    iter = json_as_array("[1, 2, null, []]");
    while ((elem = json_array_next(&iter)))
        printf("%d\n", json_as_int(elem));    /* could check for 0/EINVAL */
```

Search within a structure using a Javascript-like selector:

```c
    extern const char *input;   /* See the hotel JSON below */
    const char *cook;

    cook = json_select(input, "hotel[1].cook");

    printf("The cook's age is %d\n", json_select_int(cook, "age"));
    for (int i = 0; i < 5; i++)
        printf("score #%d: %d\n", i, json_select_int(cook, "scores[%d]", i));
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

Lets print some UTF-8-safe, bounded strings:

```c
    char buf[1024];

    /* buf is reliably NUL-terminated, even on error */
    if (json_as_str(json_select(cook, "name"), buf, sizeof buf))
        printf("The cook's name is %s\n", buf);
```

Although, if you prefer the heap:

```c
    char *cuisine = json_select_strdup(cook, "cuisine");
    if (cuisine) {
        printf("Enjoy your %s\n", cuisine);
        free(cuisine);
    }
```

But, really, most of the time you just want to compare a string's content:

```c
    const char *jstr = json_select(cook, "cuisine");
    if (json_strcmp(jstr, "Swedish-Maori Fusion") == 0)
        raise(eyebrows);
```

Or pull BASE-64 content:

```c
    int buflen = json_as_base64("\"SQNDUDQzNw==\"", buf, sizeof buf);
    if (buflen < 0)
        perror("json_as_base64");
```

## General usage

Your JSON input text should be NUL-terminated and all in memory.

Any pointer into the JSON text will represent
just the JSON value to which it points, structured or not,
and ignorant of any value that comes after it.

For example, a pointer to the `[` character
in the JSON text `[10,2,3]`
will represent the whole array as a value,
while a pointer to the `1` will represent just the number value 10.
Leading whitespace is always acceptable.

### Converters

    double json_as_double(const char *json);
    long   json_as_long(const char *json);
    int    json_as_int(const char *json);
    int    json_as_bool(const char *json);
    int    json_is_null(const char *json);
    size_t json_as_str(const char *json, void *buf, size_t bufsz);
    char * json_as_strdup(const char *json);

Converter functions translate JSON into C values in a "best effort" or
unsuprising way.
First they skip leading whitespace, then they try to convert
the next complete JSON value that they encounter.

Where there is no clear conversion from the JSON type to the desired C type,
a sensible value is returned,
accompanied by setting `errno` to `EINVAL`.
When converters would exceed an output type limit, they will return
an approximate or clamped value, and set `errno` to `ERANGE`.

#### Return values that may mean error

Follows are the values that converter functions will return to indicate
a possible error.

| If you call<br>this converter | and it<br>returned | and set<br>`errno` to | then it means<br>that the|
|------------------|-----------|--------|-------------------------------|
|`json_as_array()` |`NULL`     |`EINVAL`|input is not an array `[...]`	|
|`json_as_object()`|`NULL`     |`EINVAL`|input is not an object `{...}`	|
|`json_as_double()`|`NAN`¹     |`EINVAL`|input is not a number		|
|`json_as_double()`|±`HUGE_VAL`|`ERANGE`|number would overflow		|
|`json_as_double()`|0²         |`ERANGE`|number would underflow		|
|`json_as_long()`  |0²         |`EINVAL`|input is not a number		|
|`json_as_long()`  |`LONG_MIN`²|`ERANGE`|negative number is too big	|
|`json_as_long()`  |`LONG_MAX`²|`ERANGE`|positive number is too big	|
|`json_as_int()`   |0²         |`EINVAL`|input is not a number		|
|`json_as_int()`   |`INT_MIN`² |`ERANGE`|negative number is too big	|
|`json_as_int()`   |`INT_MAX`² |`ERANGE`|positive number is too big	|
|`json_as_bool()`  |true²      |`EINVAL`|input is not a boolean		|
|`json_as_bool()`  |false²     |`EINVAL`|not a boolean, but looks falsey|
|`json_as_str()`   |0          |`EINVAL`|input is not a string		|
|`json_as_str()`   |0          |`ENOMEM`|output buffer was too small	|
|`json_as_strdup()`|`NULL`     |`EINVAL`|input is not a string		|
|`json_as_strdup()`|`NULL`     |`ENOMEM`|call to `malloc()` failed	|
|`json_as_base64()`|-1         |`EINVAL`|input is not a BASE-64 string	|
|`json_as_base64()`|-1         |`ENOMEM`|output buffer is too small	|
|`json_type()`     |`JSON_BAD` |        |input is not a JSON value	|
|`json_span()`     |0          |`EINVAL`|input is not a JSON value	|
|`json_span()`     |0          |`ENOMEM`|input is too deeply nested	|

* ¹ NAN should be tested with the `isnan()` macro from `<math.h>`
* ² This value may be returned with `errno` unchanged.
    To detect this case, you will need to set `errno` to 0 before the call.

All converters will treat `NULL` pointer input as though it were
a malformed or empty input.

#### Conversion between types

|JSON input|`json_as_`<br>`bool`|`json_as_`<br>`int`/`long`|`json_as_`<br>`double`|`json_as_`<br>`str`|`json_as_`<br>`array`|`json_as_`<br>`object`|`json_is_`<br>`null`|
|--------------|----------|-------|-------|----------|----|----|---|
|`false`       | false    |  0ⁱ   |NaNⁱ   |`"false"`ⁱ|[]ⁱ |{}ⁱ |no |
|`true`        | true     |  0ⁱ   |NaNⁱ   |`"true"`ⁱ |[]ⁱ |{}ⁱ |no |
|*number*      | ∉{0,NaN}ⁱ|strtolʳ|strtod |strcpyⁱ   |[]ⁱ |{}ⁱ |no |
|`"`*string*`"`| ≠""ⁱ     |strtolⁱ|strtodⁱ|unescaped |[]ⁱ |{}ⁱ |no |
|`[`*array*`]` | trueⁱ    |  0ⁱ   |NaNⁱ   |`""`ⁱ     |iter|{}ⁱ |no |
|`{`*object*`}`| trueⁱ    |  0ⁱ   |NaNⁱ   |`""`ⁱ     |[]ⁱ |iter|no |
|`null`        | falseⁱ   |  0ⁱ   |NaNⁱ   |`"null"`ⁱ |[]ⁱ |{}ⁱ |yes|
|*empty*       | falseⁱ   |  0ⁱ   |NaNⁱ   |`""`ⁱ     |[]ⁱ |{}ⁱ |no |

* ⁱ sets `errno` to `EINVAL`
* ʳ may set `errno` to `ERANGE`

If a converted value would exceed a C type limit, the value will be clamped
(e.g. to `HUGE_VAL` or `INT_MAX`) and `errno` will be set to `ERANGE` or
`ENOMEM` depending on the function.

## String conversion and safety

Safe UTF-8 strings contain only shortest-length
encodings of unicode characters from the set
{ U+1 … U+D7FF, U+E000 … U+10FFFF }.
This is the same as RFC3629, except excluding NUL (U+0).

The functions `json_as_str()` and `json_as_strdup()` will refuse to
generate unsafe UTF-8 output C strings by signalling `EINVAL`.

If you need to receive malformed UTF-8 strings, see:

    size_t json_as_unsafe_str(const char *json, void *buf, size_t bufsz);
    char * json_as_unsafe_strdup(const char *json);

## More API

    int    json_as_base64(const char *json, void *buf, size_t bufsz);

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


