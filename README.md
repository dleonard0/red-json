
# Red JSON

A just-in-time, lightweight JSON parser for C.

* No pre-parse; no intermediate types.
  Convert JSON values to C as you need them.
* Iterate a `char *` pointer over JSON objects and arrays.
* Seek straight into JSON structures using patterns like `foo[1].bar`.
* Input safety: `NULL` is treated the same as an empty/malformed input.
* Converter functions are "best effort" in turning JSON values into C.
  On mismatch/error they will set `errno` to `EINVAL`
  and return a best-effort or unsurprising value.

## Examples

Let's convert some numbers:

```c
    #include <redjson.h>

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

    cook = json_select(input, "hotel[1].cook"); /* Now points to { after cook: */

    printf("The cook's age is %d\n", json_select_int(cook, "age"));
    for (unsigned i = 0; i < 5; i++)
        printf("score #%u: %d\n", i, json_select_int(cook, "scores[%u]", i));
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
    int buflen = json_as_bytes("\"SQNDUDQzNw==\"", buf, sizeof buf);
    if (buflen < 0)
        perror("json_as_bytes");
```

## Pointer convention

Any `const char *` pointer into the JSON input text string
represents
the JSON value to which it points.

For example, a pointer to the `[` character
in `[10,2,3]`
represents the whole array,
while a pointer to the `1` represents the 10.

The toplevel JSON value should be terminated with a NUL byte.

### Converters

```c
    double json_as_double(const char *json);
    long   json_as_long(const char *json);
    int    json_as_int(const char *json);
    int    json_as_bool(const char *json);
    int    json_is_null(const char *json);
    size_t json_as_str(const char *json, void *buf, size_t bufsz);
    char * json_as_strdup(const char *json);
```

Converter functions translate JSON into C values.
First they skip leading whitespace, then they convert
the next complete JSON value into the indicated C type.

When there is no clear conversion path
from the JSON value to the desired C type
(for example converting an array into an integer),
then a "sensible" value (usually zero or the empty string) is used
and `errno` is set to `EINVAL`.

#### Understanding errors from converters

This library is designed so that you don't need to check errors,
but if you do, here are the details:

| If you call <br>this converter | and it <br>returned | and it set <br>`errno` to | then it means <br>that the |
|------------------|-----------|--------|-------------------------------|
|`json_as_array()` |`NULL`     |`EINVAL`|input is not an array `[...]`	|
|`json_as_object()`|`NULL`     |`EINVAL`|input is not an object `{...}`	|
|`json_as_double()`|any        |`EINVAL`|input is a quoted string	|
|`json_as_double()`|`NAN`¹     |`EINVAL`|input is not a valid number	|
|`json_as_double()`|±`HUGE_VAL`|`ERANGE`|number would overflow		|
|`json_as_double()`|0²         |`ERANGE`|number would underflow		|
|`json_as_long()`  |0²         |`EINVAL`|input is not a number		|
|`json_as_long()`  |`LONG_MIN`²|`ERANGE`|negative number is too big	|
|`json_as_long()`  |`LONG_MAX`²|`ERANGE`|positive number is too big	|
|`json_as_int()`   |0²         |`EINVAL`|input is not a number		|
|`json_as_int()`   |`INT_MIN`² |`ERANGE`|negative number is too big	|
|`json_as_int()`   |`INT_MAX`² |`ERANGE`|positive number is too big	|
|`json_as_bool()`  |true²      |`EINVAL`|input is not a boolean		|
|`json_as_bool()`  |false²     |`EINVAL`|input is a falsey non-boolean  |
|`json_as_str()`   |0          |`EINVAL`|input is not a string		|
|`json_as_str()`   |0          |`ENOMEM`|output buffer was too small	|
|`json_as_strdup()`|`NULL`     |`EINVAL`|input is not a string		|
|`json_as_strdup()`|`NULL`     |`ENOMEM`|call to `malloc()` failed	|
|`json_as_bytes()` |-1         |`EINVAL`|input is not a BASE-64 string	|
|`json_as_bytes()` |-1         |`ENOMEM`|output buffer is too small	|
|`json_span()`     |0          |`EINVAL`|input is not a JSON value	|
|`json_span()`     |0          |`ENOMEM`|input is too deeply nested	|

* ¹ a NAN result should be detected using the `isnan()` macro from `<math.h>`
* ² This value may be returned for valid inputs, leaving `errno` unchanged.
    To detect this, set `errno` to 0 before the call.

All converters will treat `NULL` JSON as though it were empty input.

#### Conversion between types

Given a "wrong" input type, the converter functions will
set `errno` but
still return a reliable result:

|JSON input|`json_as_`<br>`bool`|`json_as_`<br>`int`/`long`|`json_as_`<br>`double`|`json_as_`<br>`str`|`json_as_`<br>`array`|`json_as_`<br>`object`|`json_is_`<br>`null`|
|--------------|----------|-------|-------|----------|----|----|---|
|`false`       | false    |  0ⁱ   |NaNⁱ   |`"false"`ⁱ|[]ⁱ |{}ⁱ |no |
|`true`        | true     |  0ⁱ   |NaNⁱ   |`"true"`ⁱ |[]ⁱ |{}ⁱ |no |
|*number*      | ∉{0,NaN}ⁱ|strtolʳ|strtod |strcpyⁱ   |[]ⁱ |{}ⁱ |no |
|`"`*string*`"`| ≠""ⁱ     |strtodⁱ|strtodⁱ|unescaped |[]ⁱ |{}ⁱ |no |
|`[`*array*`]` | trueⁱ    |  0ⁱ   |NaNⁱ   |`""`ⁱ     |iter|{}ⁱ |no |
|`{`*object*`}`| trueⁱ    |  0ⁱ   |NaNⁱ   |`""`ⁱ     |[]ⁱ |iter|no |
|`null`        | falseⁱ   |  0ⁱ   |NaNⁱ   |`"null"`ⁱ |[]ⁱ |{}ⁱ |yes|
|*empty/inval* | falseⁱ   |  0ⁱ   |NaNⁱ   |`""`ⁱ     |[]ⁱ |{}ⁱ |no |

* ʳ falls back to strtod on failure
* ⁱ sets `errno` to `EINVAL`

`json_as_bool()` implements the same "falsey" rules as Javascript.

You can also call `json_type()` to guess the type of a value from its
first non-whitespace character.
This is very fast.

### String conversion

```c
    size_t json_as_str(const char *json, void *buf, size_t bufsz);
```

This function stores a "safe" C strings in the output buffer.
A "safe" string contains only UTF-8 characters from the set
{ U+1 … U+D7FF, U+E000 … U+10FFFF }.
This matches the definition from
[RFC 3629](https://tools.ietf.org/html/rfc3629),
with the single exclusion of NUL (U+0).

Importantly, `json_as_str()` will rejects any input that
requires storing an unsafe string.
Instead, the function stores an empty string in the output buffer
and sets `errno` to `EINVAL`.

If you need to work with unsafe or malformed JSON strings, then use:

```c
    size_t json_as_unsafe_str(const char *json, void *buf, size_t bufsz);
```

This function stores a "safe-ish" string in *buf*.
This is the same as "safe" but with a further exception:
malformed and difficult input bytes are mapped into codepoints in
{ U+DC00 … U+DCFF }.
This is another deviation from RFC 3629,
but it may be reasonably safe enough for you to store and use.
The mapping also has the useful property that the
original JSON input string will be recovered *exactly*
by calling `json_string_from_unsafe_str()`.

If you are interested in byte-wise preservation of JSON input values,
consider using `json_span()` and `memcpy()`, which will work even
when the JSON value is not a string.

#### Sizing the output string buffer

Providing an output buffer is the caller's responsibility.
If the buffer provided is too small,
then the stored string will be truncated (at a UTF-8 boundary)
and the ideal buffer size will returned.

A caller can pass zero as a buffer size,
allocate a buffer using the returned value,
and then re-call the function with the new buffer.

This has been implemented already using `malloc()` as an allocator:

```c
    char * json_as_strdup(const char *json);
    char * json_as_unsafe_strdup(const char *json);
```

### Decoding BASE-64 to bytes

It is common to receive binary data as BASE-64 encoded strings
[RFC 3548](https://tools.ietf.org/html/rfc3548).

```c
    int    json_as_bytes(const char *json, void *buf, size_t bufsz);
```

## Generating JSON

```c
    size_t json_string_from_str(const char *src,
                                char *dst, size_t dstsz);
    size_t json_string_from_strn(const char *src, int srclen,
                                char *dst, size_t dstsz);
    size_t json_string_from_unsafe_str(const char *src,
                                char *dst, size_t dstsz);
    size_t json_string_from_unsafe_strn(const char *src, int srclen,
                                char *dst, size_t dstsz);
    int json_from_bytes(const void *src, size_t srcsz,
                                char *dst, size_t dstsz);

    extern const char json_true[];   /* "true" */
    extern const char json_false[];  /* "false" */
    extern const char json_null[];   /* "null" */
```


## More

See [redjson.h](redjson.h) for details.

Structure iterators

```c
    const char *json_as_array(const char *json);
    const char *json_array_next(const char **index_p);
    const char *json_as_object(const char *json);
    const char *json_object_next(const char **index_p, const char **key_ret);
```

Structure navigation

```c
    const char *json_select(const char *json, const char *path, ...);
    const char *json_selectv(const char *json, const char *path, va_list ap);
```

Miscellaneous

```c
    enum json_type json_type(const char *json);
    size_t json_span(const char *json);

    int json_strcmp(const char *json, const char *cstr);
    int json_strcmpn(const char *json, const char *cstr, size_t cstrsz);
```

## Standards and extensions

This parser implements
[RFC 7159](https://tools.ietf.org/html/rfc7159)
with the following extensions:

* Extra commas may be present at the end of arrays and objects
* Single-quoted strings are supported.
* Unquoted strings called *words* matching `[^[]{},:" \t\n\r]+` are
  accepted where strings are expected.
  Words are treated like quoted strings, except that backslash is not
  treated specially.
  Words may contain single quotes, but cannot begin with one.
  This means the input `{foo:won't}` will be treated as
  `{"foo":"won't"}`.

### Limitations

* No support for streaming
* UTF-8 input only
* Nested arrays and objects are limited to a combined depth of 32768.
  (approximately 4kB of stack is consumed by `json_span()` and `json_select()`)

## Design principles

* *Don't be surprising:*
  Exhibit familiar behaviours in unfamiliar situations.
  For example, integer conversions truncate using the same rules as C.
  Boolean conversion edge cases use the same rules as Javascript.
  API functions are consistent with each other.
  Error codes are reused from POSIX, API idioms from standard C library.
* *Be safe:*
  Reject dangerous inputs (eg bad UTF-8).
  Avoid dyamic allocations.
  Require the caller to manage their memory; ideally don't burden them at all.
  Avoid complicated interfaces; simpler interfaces lead to fewer bugs
  and don't waste the user's time.
* *Encourage tolerance:*
  Support propagating clamped or error-mapped values;
  single control paths are easier to understand.

