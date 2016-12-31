
# Red JSON

A just-in-time JSON parser for C.

* No pre-parse; no intermediate types.
* Iterate a `char *` pointer over JSON objects and arrays.
* Seek into structures using "selector" patterns, e.g. `foo[1].bar`.
* Input safety: `NULL` treated the same as empty/malformed JSON.
* Converter functions turn JSON values into C.
  On mismatch/error they set `errno`
  but still return a best-effort or "sensible" value.

## Examples

Let's convert some numbers:

```c
    double x = json_as_double("1.24e8");
    double n = json_as_double(NULL); /* NAN, errno==EINVAL */
    int i = json_as_int("9.26e1"); /* 92 */
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

Did you notice the `%d` in the path?

Let's print safe, UTF-8 strings:

```c
    char buf[1024];

    /* buf is reliably NUL-terminated, even on error */
    if (json_as_str(json_select(cook, "name"), buf, sizeof buf))
        printf("The cook's name is %s\n", buf);
```

Or get your strings on the heap:

```c
    char *cuisine = json_select_strdup(cook, "cuisine");
    if (cuisine) {
        printf("Enjoy your %s\n", cuisine);
        free(cuisine);
    }
```

Sometimes you just want to compare a string:

```c
    if (json_strcmp(json_select(cook, "cuisine"), "Swedish-Maori") == 0)
        raise(eyebrows);
```

Extract BASE-64 content:

```c
    int buflen = json_as_base64("\"SQNDUDQzNw==\"", buf, sizeof buf);
    if (buflen < 0)
        perror("json_as_base64");
```

## General usage

Make sure your JSON input text is NUL-terminated.

Any `const char *` pointer into the JSON text
can be used to represent
the value to which it points, structured or not.

For example, a pointer to the `[` character
in `[10,2,3]`
will represent the entire array,
while a pointer to the `1` will represent just the value 10.

Leading whitespace is acceptable.

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
First they skip leading whitespace, then they convert
the next complete JSON value to the indicated C type.

When there is no clear conversion path
from the JSON value to the desired C type
(for example converting an array into an integer),
then a "sensible" value (usually zero) is returned
and `errno` is set (usually to `EINVAL`).

When a conversion would exceed a C type limit, then the functions will
return a clamped or reduced-precision result, and set `errno` to `ERANGE`.

#### Errors

Detect strict conversion errors using the following table.

| If you call <br>this converter | and it <br>returned | and set <br>`errno` to | then it means <br>that the |
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

* ¹ NAN result should be tested using the `isnan()` macro from `<math.h>`
* ² This value may be returned with `errno` unchanged.
    To detect this, set `errno` to 0 before the call.

All converters will treat `NULL` JSON as though it were empty input.

#### Conversion between types

Given a "wrong" input type, the converter functions still give a reliable result:

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

`json_as_bool()` implements the same "falsey" rules as Javascript.

You can also call `json_type()` to guess the type of a value from its
first non-whitespace character.
This is very fast.

### String conversion

    size_t json_as_str(const char *json, void *buf, size_t bufsz);
    char * json_as_strdup(const char *json);

These functions generate "safe" UTF-8 C strings,
which are shortest encodings of unicode characters from the set
{ U+1 … U+D7FF, U+E000 … U+10FFFF }.
This meets RFC 3629, with the single exception of prohibiting NUL (U+0).

An attempt to generate an unsafe string will signal `EINVAL` and
these functions returns an empty string.

If you need to work with unsafe or malformed JSON strings, use:

    size_t json_as_unsafe_str(const char *json, void *buf, size_t bufsz);
    char * json_as_unsafe_strdup(const char *json);

These functions map malformed and difficult input bytes into codepoints from
{ U+DC00 … U+DCFF }. The mapping can be reversed by
`json_string_from_unsafe_str()`.

To extract BASE-64 binary, use:

    int    json_as_base64(const char *json, void *buf, size_t bufsz);

A non-string input to any of these functions will signal `EINVAL`
and a best-effort string will be returned.

String parsing functions ending with `_str()` and `json_as_base64()`
require you to provide a buffer. If you want to allocate exactly
the right sized buffer, pass a size of `0`, and the mimimum size is returned.

## Generating JSON


    size_t json_string_from_str(const char *src,
                                char *dst, size_t dstsz);
    size_t json_string_from_strn(const char *src, int srclen,
                                char *dst, size_t dstsz);
    size_t json_string_from_unsafe_str(const char *src,
                                char *dst, size_t dstsz);
    size_t json_string_from_unsafe_strn(const char *src, int srclen,
                                char *dst, size_t dstsz);
    int json_base64_from_bytes(const void *src, size_t srcsz,
                                char *dst, size_t dstsz);

    extern const char json_true[];   /* "true" */
    extern const char json_false[];  /* "false" */
    extern const char json_null[];   /* "null" */

## More

See [json.h](json.h) for details.

Structure iterators

    const char *json_as_array(const char *json);
    const char *json_array_next(const char **index_p);
    const char *json_as_object(const char *json);
    const char *json_object_next(const char **index_p, const char **key_ret);

Structure navigation

    const char *json_select(const char *json, const char *path, ...);
    const char *json_selectv(const char *json, const char *path, va_list ap);

Miscellaneous

    enum json_type json_type(const char *json);
    size_t json_span(const char *json);

    int json_strcmp(const char *json, const char *cstr);
    int json_strcmpn(const char *json, const char *cstr, size_t cstrsz);

Generating JSON strings


## Standards and extensions

This parser implements RFC 7159 with the following extensions:

* Extra commas may be present at the end of arrays and objects
* Strings may be also be single-quoted.
  Inside a single-quoted string, the double quote need not be
  escaped, but single quotes must.
* Unquoted strings called 'words' (matching `[^[]{},:" \t\n\r]+`) are
  accepted where strings are expected.
  Words are treated like quoted strings, except that backslash is not
  treated specially.
  Words may contain single quotes, but cannot begin with one.

### Parser limitations

* No support for streaming
* UTF-8 input only
* Nested arrays and objects limited to a combined depth of 32768

