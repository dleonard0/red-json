
# Red JSON

A just-in-time, lightweight JSON parser for C.

* No need to parse into a value tree.
  Convert JSON directly into C values as you need them.
  Very low on memory use.
* Iterate a `const char *` pointer over your input as you
  extract JSON numbers, strings, objects and arrays.
* Seek directly into deep structures with `json_select()`
  using patterns like `foo[1].bar`.
* All inputs are `NULL`-safe.
  `NULL` is treated the same as empty/malformed input.
* A tight group of converter functions that turn JSON values
  into the C types you need. No fuss, no surprise.
* Consistent error system using `-1` and `errno`.
* Sanitized UTF-8; lock-free and re-entrant safe functions.

[![Build Status](https://travis-ci.org/dleonard0/red-json.svg?branch=master)](https://travis-ci.org/dleonard0/red-json)

## Overview

Make sure your JSON data fits in memory and is NUL-terminated.
Functions named `json_as_<ctype>()` convert JSON into the C type directly.
The `json_select()` function "drills down" to the substructure you need.

## Examples

Let's say we have this input:

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

Then we can process that input with this program:

```c
    extern const char *input;                         /* the JSON text above */
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

Pro tip:
Every time you see `json_select_<ctype>()` know that it's a convenience
function that combines `json_select()` with `json_as_<ctype>()`.

Here are some more conversions to get a feel for `json_as_<ctype>()`:

```c
    #include <redjson.h>

    double a = json_as_double("1.24e8");              /* a = 124000000 */
    double b = json_as_double(NULL);                  /* b = NAN, errno = EINVAL */
    int c = json_as_int("9.26e1");                    /* c = 92 */
    int d = json_select_int("[true,5,{}]", "[1]");    /* d = 5 */
```


String bufers are always left in a safe state:

```c
    char buf[1024];

    if (json_as_str(json_select(cook, "name"), buf, sizeof buf)) {
        /* buf[] will always become NUL-terminated, even on error.
	 * And the text is truncated where it violates RFC 3629 */
        printf("The cook's name is %s\n", buf);
    }

    /* Output: The cook's name is Mr LeCheﬀ */
```

(But `json_as_utf8b()` exists if you don't like safety-truncating.)

Maybe we prefer the heap instead of return buffers:

```c
    char *cuisine = json_select_strdup(cook, "cuisine");
    if (cuisine) {
        printf("Enjoy your %s\n", cuisine);
        free(cuisine);
    }

    /* Output: Enjoy your Fish and chips */
```

And sometimes you just want to compare strings without
copying them anywhere:

```c
    if (json_strcmp(json_select(cook, "cuisine"), "Swedish-Maori") == 0)
        raise(eyebrows);
```

Leaving food behind now, we can demonstrate BASE-64 binary content:

```c
    int buflen = json_as_bytes("\"SQNDUDQzNw==\"", buf, sizeof buf);
    if (buflen < 0)
        perror("json_as_bytes");
```

And finally, an example of iterating over an array, <em>in-place</em>:

```c
    const char *iter;                                 /* Iterator state */
    const char *elem;                                 /* Loop variable */

    iter = json_as_array("[1, 2, null, []]");         /* Starts iterator */
    while ((elem = json_array_next(&iter)))
        printf("%d ", json_as_int(elem));

    /* Output: 1 2 0 0 */

    /* (No need to cleanup elem or iter.) */
```

A similar function exists for walking the key-values of an object.

## Text pointers

A `const char *` pointer into JSON text
represents the JSON value to which it points.
We call these "text pointers".

For example, a pointer to the `[` character
in `[10,2,3]`
represents the whole array,
while a pointer to the `1` represents the 10.

Please ensure JSON text is terminated with a NUL byte
if there is any risk of it being malformed or
truncated.

### Converters

Here are the functions that convert JSON text into C values.

```c
    double json_as_double(const char *json);
    long   json_as_long(const char *json);
    int    json_as_int(const char *json);
    int    json_as_bool(const char *json);
    int    json_is_null(const char *json);
    size_t json_as_str(const char *json, void *buf, size_t bufsz);
    char * json_as_strdup(const char *json);
    time_t json_as_time(const char *json);
```

They are all widely tolerant of different kinds of JSON input.
Here is what they do for various inputs:

|JSON input|`json_as_`<br>`bool`|`json_as_`<br>`int`/`long`|`json_as_`<br>`double`|`json_as_`<br>`str`|`json_as_`<br>`array`|`json_as_`<br>`object`|`json_is_`<br>`null`|
|--------------|----------|-------|-------|----------|----|----|---|
|`false`       | false    |  0ⁱ   |NaNⁱ   |`"false"`ⁱ|[]ⁱ |{}ⁱ |no |
|`true`        | true     |  0ⁱ   |NaNⁱ   |`"true"`ⁱ |[]ⁱ |{}ⁱ |no |
|*number*      | ∉{0,NaN}ⁱ|strtolʳ|strtod |strcpyⁱ   |[]ⁱ |{}ⁱ |no |
|`"`*string*`"`| ≠""ⁱ     |strtodⁱ|strtodⁱ|unescape  |[]ⁱ |{}ⁱ |no |
|`[`*array*`]` | trueⁱ    |  0ⁱ   |NaNⁱ   |`""`ⁱ     |iter|{}ⁱ |no |
|`{`*object*`}`| trueⁱ    |  0ⁱ   |NaNⁱ   |`""`ⁱ     |[]ⁱ |iter|no |
|`null`        | falseⁱ   |  0ⁱ   |NaNⁱ   |`"null"`ⁱ |[]ⁱ |{}ⁱ |yes|
|*empty/inval* | falseⁱ   |  0ⁱ   |NaNⁱ   |`""`ⁱ     |[]ⁱ |{}ⁱ |no |

* ʳ falls back to strtod on failure
* ⁱ and sets `errno` to `EINVAL`

The functions return "benign" values are intended to be safe for when
you forget to check `errno`.

Here is how to handle errors if you need to be careful:

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
|`json_as_time()`  |-1         |`EINVAL`|input is not an RFC3339 string |
|`json_span()`     |0          |`EINVAL`|input is not a JSON value	|
|`json_span()`     |0          |`ENOMEM`|input is too deeply nested	|

* ¹ a NAN result should be detected using the `isnan()` macro from `<math.h>`
* ² This value may be returned for valid inputs, leaving `errno` unchanged.
    To detect this, set `errno` to 0 before the call.

All converters will treat `NULL` JSON as though it were empty input.

#### More type tricks

`json_as_bool()` implements the same "falsey" rules as Javascript.

Instead of looking for errors
you can first call `json_type()` to classify the JSON type of the text
and use that information to choose which converter to use.

```c
    enum json_type json_type(const char *json);
```

### Safe strings and you

```c
    size_t json_as_str(const char *json, void *buf, size_t bufsz);
```

`json_as_str()` stores a "safe" C string in the output buffer *buf*.
By "safe", I mean a string truncated to protect you as per
[RFC 3629](https://tools.ietf.org/html/rfc3629).
Truncation happens when the input
* contains U+0 (`EINVAL`), or
* would require the storing of an unsafe UTF-8 codepoint in
  the output (`EINVAL`), or
* has a malformed or overlong UTF-8 encoding (`EINVAL`), or
* would otherwise overflow the output buffer (`ENOMEM`).

Examples of partial inputs where `json_as_str()` will terminate
conversion are:
`\u0000`, `\ud800` and `<C0 A0>` (an overlong SPC).

#### UTF-8B to the rescue

If you need to work with potentially unsafe or malformed JSON input,
then use:

```c
    size_t json_as_utf8b(const char *json, void *buf, size_t bufsz);
```

This function does not reject any inputs, because
it stores a UTF-8B transformation in *buf*.
UTF-8B uses the codepoints { U+DC00 … U+DCFF }
to hold all the malformed and "difficult" input bytes.

For example, the overlong input byte sequence `<C0><A0>` is
stored as `<U+DCC0><U+DCA0>`,
and the problematic JSON escape sequence `\u0000` as `<U+DC5C>u0000`.

This transformation is exactly reversed when you send the string back with
`json_from_utf8()`.

[Kuhn's UTF-8b](http://hyperreal.org/~est/utf-8b/releases/utf-8b-20060413043934/kuhn-utf-8b.html)
is strictly incompatible with RFC 3629 or
[Unicode Scalar Values](http://www.unicode.org/glossary/#unicode_scalar_value)
but it can be useful for interoperability
when you need to send back exactly the garbage you got.

#### Handling raw or binary data

Another way to preserve uninterpreted JSON input is
to use `json_span()` and `memcpy()`.
This works for any kind of JSON input text.

If you have binary data, consider using `json_as_bytes()`.

#### Sizing the output buffer

Providing a large-enough output buffer to string functions
is the caller's responsibility.
If the buffer provided is too small,
then the stored string will be NUL-truncated at the UTF-8 boundary
nearest the end of the buffer,
although the ideal buffer size will returned.

To dynamically allocate the buffer,
the caller can specify a buffer size of 0,
allocate a buffer using the returned value,
and then re-call the function with the new buffer.
This has been implemented already,
using `malloc()`,
by these convenience functions:

```c
    char * json_as_strdup(const char *json);
    char * json_as_utf8b_strdup(const char *json);
```

### Decoding BASE-64 to bytes

It is common to receive binary data as BASE-64 encoded strings, per
[RFC 3548](https://tools.ietf.org/html/rfc3548).

```c
    int    json_as_bytes(const char *json, void *buf, size_t bufsz);
```

## Generating JSON

Use `sprintf()` to form most of your JSON output.
It works amazingly well.

To generate JSON string text, use the following:

```c
    size_t json_from_str(const char *src,
                        char *dst, size_t dstsz);
    size_t json_from_strn(const char *src, int srclen,
                        char *dst, size_t dstsz);
    size_t json_from_utf8b(const char *src,
                        char *dst, size_t dstsz);
    size_t json_from_utf8bn(const char *src, int srclen,
                        char *dst, size_t dstsz);
    int json_from_bytes(const void *src, size_t srcsz,
                        char *dst, size_t dstsz);
    int json_from_time(time_t t, char *dst, size_t dstsz);
```

Safety note:
Generated JSON will not contain the literal sequences `</` nor `]]>`.
Such sequences are replaced by `<\/` and `]]\u003e`, respectively, as a
guard against their abuse in HTML documents.

### Generating booleans and null

These are provided for consistency and to save space.

```c
    const char *json_from_bool(int b);

    extern const char json_true[];   /* "true" */
    extern const char json_false[];  /* "false" */
    extern const char json_null[];   /* "null" */
```

## More

See [redjson.h](redjson.h) for details.

Structure enumeration

```c
    const char *json_as_array(const char *json);
    const char *json_array_next(const char **index_p);
    const char *json_as_object(const char *json);
    const char *json_object_next(const char **index_p, const char **key_ret);
```

Structure traversal

```c
    const char *json_select(const char *json, const char *path, ...);
    const char *json_selectv(const char *json, const char *path, va_list ap);
```

Date and time ([RFC 3339](https://tools.ietf.org/html/rfc3339))

```c
    time_t json_as_time(const char *json);
    size_t json_from_time(time_t t, char *dst, size_t dstsz);
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

This parser encourages the recommendations from
[RFC 7493](https://tools.ietf.org/html/rfc7493):

* 4.3 use RFC 3339 for dates
* 4.4 use BASE-64 for binary data

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

