
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

[![Build Status](https://travis-ci.org/dleonard0/red-json.svg?branch=master)](https://travis-ci.org/dleonard0/red-json)

## Examples

Let's convert some numbers:

```c
    #include <redjson.h>

    double a = json_as_double("1.24e8");              /* 124000000 */
    double b = json_as_double(NULL);                  /* NAN, errno==EINVAL */
    int c = json_as_int("9.26e1");                    /* 92 */
    int d = json_select_int("[true,5,{}]", "[1]");    /* 5 */
```

Iterate over an array:

```c
    const char *iter;                                 /* Iterator state */
    const char *elem;                                 /* Loop variable */

    iter = json_as_array("[1, 2, null, []]");         /* Starts iterator */
    while ((elem = json_array_next(&iter)))
        printf("%d ", json_as_int(elem));

    /* Output: 1 2 0 0 */
```

Search within a structure using a Javascript-like selector:

```c
    extern const char *input;                         /* See hotel JSON below */
    const char *cook;

    cook = json_select(input, "hotel[1].cook");       /* Points to { after "cook": */

    printf("The cook's age is %d\n", json_select_int(cook, "age"));
    for (unsigned i = 0; i < 3; i++)
        printf("score #%u: %d\n", i, json_select_int(cook, "scores[%u]", i));

    /* Output: The cook's age is 91
               score #0: 4
               score #1: 5
               score #2: 1           */
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
                    "scores": [ 4, 5, 1 ]
                },
            }
        ]
   }
```

Let's print safe, UTF-8 strings:

```c
    char buf[1024];

    /* buf will always be NUL-terminated, even on error */
    if (json_as_str(json_select(cook, "name"), buf, sizeof buf))
        printf("The cook's name is %s\n", buf);

    /* Output: The cook's name is Mr LeCheﬀ */
```

Or get your strings on the heap:

```c
    char *cuisine = json_select_strdup(cook, "cuisine");
    if (cuisine) {
        printf("Enjoy your %s\n", cuisine);
        free(cuisine);
    }

    /* Output: Enjoy your Fish and chips */
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
|`json_as_double()`|*n*        |`EINVAL`|input is almost a valid number	|
|`json_as_double()`|`NAN`¹     |`EINVAL`|input is not a valid number	|
|`json_as_double()`|±`HUGE_VAL`|`ERANGE`|number would overflow		|
|`json_as_double()`|0²         |`ERANGE`|number would underflow		|
|`json_as_long()`  |0²         |`EINVAL`|input is not a number		|
|`json_as_long()`  |`LONG_MIN`²|`ERANGE`|negative number is too big	|
|`json_as_long()`  |`LONG_MAX`²|`ERANGE`|positive number is too big	|
|`json_as_int()`   |0²         |`EINVAL`|input is not a number		|
|`json_as_int()`   |`INT_MIN`² |`ERANGE`|negative number is too big	|
|`json_as_int()`   |`INT_MAX`² |`ERANGE`|positive number is too big	|
|`json_as_bool()`  |true²      |`EINVAL`|input is not boolean		|
|`json_as_bool()`  |false²     |`EINVAL`|input is not boolean but looks falsey|
|`json_as_str()`   |*size*     |`EINVAL`|input is not a safe string	|
|`json_as_str()`   |*size*     |`ENOMEM`|output buffer was too small	|
|`json_as_strdup()`|`NULL`     |`EINVAL`|input is not a safe string	|
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

Given a "wrong" input type,
a converter functions will set `errno` to `EINVAL` then
return a (hopefully) unsurprising result.
Here is a table of those results.
It summarises what each function along the top will return
when given the JSON input from the first column.

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

Instead of checking for `EINVAL`,
you can call `json_type()` to guess the type of a value
(from its first non-whitespace character),
and use that to choose the converter to call.

### String conversion

```c
    size_t json_as_str(const char *json, void *buf, size_t bufsz);
```

This function stores a "safe" C string in the output buffer *buf*.
A "safe" string consists only of UTF-8 characters permitted by
[RFC 3629](https://tools.ietf.org/html/rfc3629)
but excluding NUL (U+0) which is always present as the string terminator.

`json_as_str()` will NUL-truncate the output at the first point where the input
* has a malformed or overlong UTF-8 encoding (`EINVAL`), or
* would require the storing of an unsafe UTF-8 codepoint in
  the output (`EINVAL`), or
* would otherwise exceed the size of the output buffer (`ENOMEM`).

Examples of partial inputs where `json_as_str()` will terminate
conversion are:
`\u0000`, `\ud800` and `<C0 A0>` (an overlong SPC).

#### Sanitized strings

If you need to work with unsafe or malformed JSON input, then use:

```c
    size_t json_as_unsafe_str(const char *json, void *buf, size_t bufsz);
```

This function does not reject any inputs,
storing a "sanitized" UTF-8 string in *buf*.
"Sanitized" is the same as "safe" but
with the addition of unsafe codepoints { U+DC00 … U+DCFF }
onto which are mapped all malformed and "difficult" input bytes.
For example, the overlong `<C0 A0>` is santized to `<U+DCC0><U+DCA0>`
and `\u0000` is sanitized to `<U+DC5C>u0000`.

"Sanitized" strings are an extension of
[Kuhn's UTF-8b](http://hyperreal.org/~est/utf-8b/releases/utf-8b-20060413043934/kuhn-utf-8b.html)
and are incompatible with RFC 3629 or
[Unicode Scalar Values](http://www.unicode.org/glossary/#unicode_scalar_value).
They have the useful property that the
original input string can be recovered exactly
(and `json_string_from_unsafe_str()` does precisely that).
Byte-wise preservation of uninterpreted JSON input can
also be performed using `json_span()` and `memcpy()`,
which works even when the JSON input is not a quoted string.

#### Sizing the output string buffer

Providing an output buffer is the caller's responsibility.
If the buffer provided is too small,
then the stored string will be NUL-truncated at the UTF-8 boundary
nearest the end of the buffer,
and the ideal buffer size will returned.

To dynamically allocate the buffer,
the caller can specify a buffer size of 0,
allocate a buffer using the returned value,
and then re-call the function with the new buffer.
This has been implemented already,
using `malloc()`,
by these convenience functions:

```c
    char * json_as_strdup(const char *json);
    char * json_as_unsafe_strdup(const char *json);
```

### Decoding BASE-64 to bytes

It is common to receive binary data as BASE-64 encoded strings, per
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

Generated, quoted JSON strings will not contain the sequences `</` nor `]]>`.
These sequences are replaced by `<\/` and `]]\u003e`, respectively, as a
guard against their use in HTML documents.

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

