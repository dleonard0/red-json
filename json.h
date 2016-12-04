#ifndef JSON_H
#define JSON_H

/** @file
 * Red JSON parser.
 *
 * This is a lightweight, just-in-time parser for JSON. Instead of
 * converting all JSON into in-memory data structures, this parser assumes
 * you keep the source UTF-8 input ("JSON text") in memory. Then it gives
 * you the tools to seek around within the input and convert the JSON values
 * to C as you need them.
 *
 * The parser implements all of JSON RFC 7159 with some extensions, such as
 * accepting trailing commas and unquoted key strings.
 *
 * To generate JSON output, functions are provided to quote and escape UTF-8
 * C strings and generate BASE-64 strings from byte arrays. For generating
 * other types (number, boolean) you should use #printf() and friends.
 *
 * <h3>Input converter functions</h3>
 *
 * The most important functions are the input converters. These
 * convert JSON values into C values.
 *
 *    int    #json_as_bool(const char *)
 *    int    #json_as_int(const char *)
 *    long   #json_as_long(const char *)
 *    double #json_as_double(const char *)
 *    size_t #json_as_str(const char *, void *, size_t)
 *    size_t #json_as_bytes(const char *, void *, size_t)
 *    int    #json_is_null(const char *)
 *
 * Converters always take a pointer into NUL-terminated JSON text.
 * After skipping any whitespace, the converters then process only the
 * first complete JSON value they encounter.
 * JSON following the value is ignored. This way any pointer into the JSON
 * text represents only the value at that position, be it a simple
 * <code>null</code> or a deeply-nested object.
 *
 * The #json_type() function indicates the type of JSON value at the pointer.
 * It is as reliable as the JSON input is well-formed. This check is
 * usually unnecessary if you are comfortable with the input converters
 * performing a 'best effort' conversions from JSON to C.
 *
 * Conversions are best effort and "unsuprising". For inter-type conversions
 * where no unsurprising method is available, a 'zero' return is provided.
 * An exception to the zero rule is #json_as_bool() which effectively measures
 * the 'falsiness' of a value, and #json_as_double() which returns NaN (not 0)
 * to represent a failed conversion. Converters always set #errno in the case
 * of inexact conversions. You can choose to ignore that if you wish.
 *
 * The converter functions treat input JSON values according to this table:
 * <pre>
 *   Input                     json_as_...
 *            :  bool    int    double string  array object null
 *   -------- -  ------- ------ ------ ------  ----- ------ ----
 *   false    :  0       0      NaN    "false" []    {}     no
 *   true     :  1       1      NaN    "true"  []    {}     no
 *   <number> :  !=0|NaN strtod strtod strcpy  []    {}     no
 *   <string> :  !=""    strtod strtod utf8    []    {}     no
 *   <array>  :  !=[]    0      NaN    strcpy  =     {}     no
 *   <object> :  !={}    0      NaN    strcpy  []    =      no
 *   null     :  0       0      NaN    "null"  []    {}     yes
 *   <empty>  :  0       0      NaN    ""      []    {}     no
 * </pre>
 *
 * If a converted value would exceed a C type limit, the value will be clamped
 * (e.g. to #HUGE_VAL or #INT_MAX) and #errno will be set to #ERANGE or
 * #ENOMEM depending on the function.
 *
 * All converters are safe to call with @c NULL pointers or with
 * malformed input.
 * @c NULL is always treated as if it were empty text.
 * Malformed JSON text is usually recognised as a "word" (unquoted string)
 * up until the next JSON delimiter or whitespace.
 *
 *
 * <h3>String conversions</h3>
 *
 * Standard JSON strings are surrounded by double quotes (").
 * This parser also accepts strings quoted with single quotes (')
 * and unquoted "word" sequences.
 *
 * "Words" are UTF-8 input sequences terminated by either
 * whitespace, a NUL byte or a JSON delimiting character (<code>[]{},:"</code>).
 * For example, <code>hello</code>, <code>1.0e8</code>, <code>null</code>,
 * <code>true-blue</code> and <code>don't</code> are words.
 * Depending on the converter, words are interpreted in different ways.
 * For example, the JSON text <code>true</code> will be recognized
 * by #json_as_str() as an unquoted string.
 *
 *
 * <h3>UTF-8, NUL bytes and C strings</h3>
 *
 * A goal of this parser is to provide both safe and lossless (but not at
 * the same time!) inter-conversion between JSON input strings and
 * NUL-terminated C strings.
 *
 * NUL bytes cannot occur within JSON text; they indicate end-of-input.
 * Invalid UTF-8 sequences (eg 80) are mapped byte-for-byte into the
 * code point range U+DC00..U+DCFF. This includes overlong input encodings
 * (eg C0 80) and otherwise valid encodings of surrogates U+D800..U+DFFF.
 *
 * A second level of unicode translation happens with escape sequences.
 * Escape sequences that would generate invalid UTF-8 (i.e \uD800..\uDFFF
 * and \u0000) will have their first backslash character treated as an
 * invalid byte (U+DC5C).
 *
 * Twelve-byte surrogate escapes from JSON are accepted, and converted
 * immediately to code points.
 *
 * When a C string is created from code points, it is encoded in shortest-form
 * UTF-8. If it contains any code points from U+DC00..U+DCFF,
 * #json_as_unsafe_str() will encode them as 3-byte UTF-8.
 * In particular, the JSON escape <code>\u0000</code> will result in the C
 * string sequence ED B1 9C 75 30 30 30 30.
 *
 * The function #json_as_str() will simply fail with #EINVAL if the
 * code points U+0,U+D800..U+DFFF would be present in the input.
 *
 * When a JSON string is converted from a UTF-8 C string, the presence of
 * code points U+D800..U+DFFF will cause #json_string_from_str() to fail
 * with an error. However, the companion #json_unsafe_string_from_str()
 * will convert U+DC00..U+DCFF back into unescaped bytes, reversing the above
 * process to achieve invalidity-preserving conversion between C and JSON.
 *
 * If you need to pass binary data in JSON, use BASE-64 encoding. The functions
 * #json_as_base64() and #json_base64_from_bytes() do this.
 *
 *
 * <h3>Selection</h3>
 *
 * 'Selection' is a mechanism for seeking to a value within a nesting of
 * arrays and objects. Selectors are patterns modeled after Javascript syntax.
 *
 * For example, given the JSON text @code {"foo":[null,{"bar":7}]} @endcode
 * the '7' could be accessed using the selector "foo[1].bar".
 *
 * <pre>
 *        const char *s = "{foo:[null,{bar:7}]}";
 *        int seven = json_select_int(s, "foo[1].bar");
 * </pre>
 *
 * #json_select() always sets #errno to #ENOENT if the element was not
 * found (and empty input was used); or sets #errno to 0 if it was found.
 *
 *
 * <h3>Iterators</h3>
 *
 * Elements of array and members of objects can be accessed using iterators.
 * An iterator is a simple pointer used to keep track of position within
 * the structure value.
 *
 * For example, to iterate over the members of an object, first call
 * #json_as_object() to obtain the initial iterator position, then
 * repeatedly call #json_object_next() on the iterator to access the
 * key and value of the current member, and advance the iterator to the
 * next member. Use #json_strcmp() to compare the JSON string with a C string.
 * The end of the object is detected when #json_object_next() returns false.
 *
 * @code
 *     const char *oi, *key, *value;
 *     char buf[256];
 *
 *     oi = json_as_object("{a:1, b:2}");
 *     while ((value = json_object_next(&oi, &key))) {
 *          db_insert(json_as_str(key, buf, sizeof buf),
 *                    json_as_int(value));
 *     }
 * @endcode
 *
 * Iterating over the elements of an array is similar.
 *
 *
 * <h3>Parser extensions and limitations</h3>
 *
 * The parser accepts the following extensions to JSON (RFC 7159):
 * <ul>
 * <li> Extra commas may be present at the end of arrays and objects;
 * <li> Unnecessary escapes inside quoted strings are converted to U+DC5C;
 * <li> Strings may be single-quoted. Inside single quoted (') strings, the
 *      double quote (") need not be escaped (\"), but single quotes must
 *      be (\').
 * <li> Bare strings (matching [^[]{},:" \t\n\r]+) are accepted as strings.
 *      They are treated as UTF-8. Backslashes are treated literally.
 * </ul>
 *
 * The parser has the following limits:
 * <ul>
 * <li>Only UTF-8 is understood
 * <li>Nesting of arrays and objects is limited to 32768, combined.
 * </li>
 *
 * <h3>Generator functions</h3>
 *
 * The generator functions return a pointer to NUL-terminated JSON text.
 *
 *    #json_string_from_str()
 *    #json_string_from_unsafe_str()
 *    #json_base64_from_bytes()
 *    #json_bool()
 *
 * @author David Leonard; released into Public Domain 2016
 */


#include <stdarg.h>
#include <stdlib.h>

/* Type qualifiers used to document function signatures */
#define __JSON         /* indicates a UTF-8 JSON-encoded string */
#define __JSON_ARRAYI  /* indicates an array iterator */
#define __JSON_OBJECTI /* indicates an object iterator */
#define __JSON_MUTF8   /* indicates a nul-terminated MUTF-8 C string */

/**
 * Select an element within a JSON structure.
 *
 * For example, the selector "[1].name" will locate the value "Fred" in
 * text <code>[{"name":"Tim", "age":28},{"name":"Fred", "age":26}]</code>.
 *
 * A selector path is any sequence of array-index or object-key
 * components of the forms "[<index>]" and ".<key>". Path components can be
 * literal (e.g. "[7]" or ".foo") or references to varargs ("[%u]" or ".%s").
 * Vararg references expect parameters of type <code>unsigned int</code>
 * and <code>const char *</code> respectively.
 *
 * There are some limitations on literal ".<key>" components:
 * <ul>
 *     <li>literal keys must not contain '%', '.' or '[';
 *     <li>literal keys (and vararg key strings) must be encoded as
 *         shortest-form MUTF-8 because #json_strcmp() is used to compare them.
 * </ul>
 *
 * "%d" may be used for "%u", but negative indicies will result in #ENOENT.
 *
 * @param json  (optional) JSON to select within
 * @param path  selector path, matches <code>(.key|.%s|[int]|[%u])*</code>
 *	        The leading '.' may be omitted.
 * @param ...   Variadic parameters for each "%s" and "[%u]"
 *		component of the @a path
 * @returns pointer within @a json to the selected value (sets #errno to 0); or
 *          NULL if the path was not found (sets #errno to #ENOENT); or
 *          NULL if the path was malformed (sets #errno to #EINVAL).
 */
__attribute__((format(printf,2,3)))
const __JSON char *json_select(const __JSON char *json, const char *path, ...);

/** @see #json_select() */
const __JSON char *json_selectv(const __JSON char *json, const char *path, va_list ap);

/* Convenience macros */
#define json_select_int(...)    json_as_int(json_select(__VA_ARGS__))
#define json_select_bool(...)   json_as_bool(json_select(__VA_ARGS__))
#define json_select_double(...) json_as_double(json_select(__VA_ARGS__))
#define json_select_array(...)  json_as_array(json_select(__VA_ARGS__))
#define json_select_object(...) json_as_object(json_select(__VA_ARGS__))
#define json_select_strdup(...) json_as_strdup(json_select(__VA_ARGS__))

/* Convenience selector functions that return a default value
 * when the selector path is not found */
int json_default_select_int(int default_, const __JSON char *json, const char *path, ...);
int json_default_select_bool(int default_, const __JSON char *json, const char *path, ...);
double json_default_select_double(double default_, const __JSON char *json, const char *path, ...);
const __JSON_ARRAYI char *json_default_select_array(const __JSON_ARRAYI char *default_, const __JSON char *json, const char *path, ...);
const __JSON_OBJECTI char *json_default_select_object(const __JSON_OBJECTI char *default_, const __JSON char *json, const char *path, ...);

/**
 * Begin an iterator over a JSON array.
 *
 * Example usage:
 * <code>
 *     const char *ai;
 *     const char *v;
 *
 *     ai = json_as_array("[1,2,3]");
 *     while ((v = json_array_next(&ai))) {
 *         printf("%d\n", json_as_int(v));
 *     }
 * </code>
 *
 * @param json  (optional) JSON text
 * @return Initial iterator value for use with #json_array_next(); or
 *         NULL if @a json is not an array (#errno is set to #EINVAL).
 */
const __JSON_ARRAYI char *json_as_array(const __JSON char *json);

/**
 * Fetch the element at the array iterator position, then
 * advance the iterator to the next element.
 *
 * @param ai_ptr  Pointer to storage holding an array iterator;
 *                If the iterator is NULL, the array is assumed exhausted.
 *                @see #json_as_array() for initial value.
 * @return pointer to the current element of the array
 *         (prior to advancing the iterator); or
 *         @c NULL if there are no more elements in the array.
 */
const __JSON char *json_array_next(const __JSON_ARRAYI char **ai_ptr);

/**
 * Begin an iterator over a JSON object.
 *
 * Example usage:
 * <code>
 *     const char *oi;
 *     const char *key, *value;
 *     char buf[256];
 *
 *     oi = json_as_object("{a:1, b:2}");
 *     while ((value = json_object_next(&oi, &key))) {
 *         printf("%s: %d\n", json_as_str(key, buf, sizeof buf),
 *                            json_as_int(value));
 *     }
 * </code>
 *
 * @param json  (optional) JSON text
 * @return index pointer to the first field in the object, or
 *         @c NULL if the object contains no fields; or
 *	   @c NULL if @a json does not point to a valid object.
 */
const __JSON_OBJECTI char *json_as_object(const __JSON char *json);

/**
 * Fetch the key and value at the object iterator position, then
 * advance the iterator to the next member.
 *
 * @param oi_ptr Pointer to storage holding an object iterator;
 *               If the iterator is NULL, the object is assumed exhausted.
 *               @see #json_as_object() for initial value.
 * @param key_return  (optional) Storage to receive the position of
 *                    the member's key (prior to advancing the iterator).
 * @return pointer to an element's value; or
 *         @c NULL when there are no more members,
 *                    and @a key_return was not modified.
 */
const __JSON char *json_object_next(const __JSON_OBJECTI char **indexp,
		const __JSON char **key_return);

/**
 * Guess at the type of a JSON value.
 *
 * This function examines the first non-whitespace character of
 * the JSON text to guess the JSON type. The result is reliable
 * only if the input is strict JSON (which is not detected).
 *
 * @param json  (optional) JSON text
 * @return the guessed type of the JSON value at the pointer.
 */
enum json_type {
	JSON_BAD = 0,	/* NULL, invalid char, comma, closing brace, EOS */
	JSON_OBJECT,	/* '{' */
	JSON_ARRAY,	/* '[' */
	JSON_BOOL,	/* 't' or 'f' */
	JSON_NULL,	/* 'n' */
	JSON_NUMBER,	/* [-+.0-9] */
	JSON_STRING	/* " */
} json_type(const __JSON char *json);

/**
 * Determine the span length in bytes of a JSON value.
 *
 * Knowing a span a value occupies in the JSON text allows you to copy out
 * and re-use sub-structures without needing to decode and re-encode them.
 *
 * The span will include any leading whitespace, but not trailing whitespace.
 * (Note: iterators and select will return pointers that have skipped over
 * leading whitespace.)
 *
 * #json_span() walks the JSON encoded value, entering arrays and objects
 * and stopping when it encounters NUL ':' ',' or an unbalanced ']' or '}'.
 *
 * @param json  (optional) JSON text
 * @return the number of bytes from @a json to just after the end of the
 *             first value found; or
 *         0 if @a json is NULL
 */
size_t json_span(const __JSON char *json);

/**
 * Convert JSON to a floating-point number.
 *
 * The conversion depends on the JSON text:
 * <ul>
 * <li><code>true</code>, <code>false</code>, <code>null</code>,
 *     arrays, objects and EOS convert to NaN (#EINVAL)
 * <li>JSON strings are converted using the next step after skipping the
 *     leading quote. Any escapes within the JSON string are
 *     left unexpanded.
 * <li>JSON numbers are converted using your libc's #strtod(),
 *     which may understand special sequences such as "Inf" or "NaN(7)".
 *     The JSON text is not canonicalized before calling #strtod().
 * </ul>
 * @param json pointer to JSON text
 * @returns the converted numeric value (#errno is unchanged); or
 *          #NAN if the value was malformed (sets #errno to #EINVAL), or
 *          #HUGE_VAL or -#HUGE_VAL if the value is too big (#ERANGE), or
 *          0 if the value would underflow (#ERANGE).
 */
double json_as_double(const __JSON char *json);

/**
 * Convert JSON to a long integer.
 *
 * JSON values are converted as follows:
 * <ul>
 * <li><code>true</code> converts to 1L
 * <li><code>false</code> converts to 0L
 * <li><code>null</code>, arrays, objects and EOS convert to 0L
 *     and set #errno to #EINVAL
 * <li>JSON strings are converted using the next step after skipping the
 *     leading quote. Any escapes within the JSON string are
 *     left unexpanded.
 * <li>JSON numbers are first converted using #strtoul(). If this conversion
 *     ends at a '.', 'e' or 'E', then #strtod() is used and its result
 *     is clamped to [#LONG_MIN,#LONG_MAX] (and sets #errno to #ERANGE).
 *     Underflow from #strtod() is ignored, and standard C truncation
 *     (rounding towards zero) is applied to get an integer.
 * </ul>
 *
 * @param json  (optional) JSON text
 * @return an integer (#errno unchanged); or
 *         0 if the value was invalid (#EINVAL); or
 *         #LONG_MIN or #LONG_MAX if the value was out of range (#ERANGE).
 */
long json_as_long(const __JSON char *json);

/**
 * Convert JSON value into an integer.
 *
 * This is the same as #json_as_long() but clamping the value
 * to [#INT_MIN,#INT_MAX].

 * @param json  (optional) JSON text
 * @return an integer (#errno unchanged); or
 *         0 if the value was invalid (#EINVAL); or
 *         #INT_MIN or #INT_MAX if the value was out of range (#ERANGE).
 */
int json_as_int(const __JSON char *json);

/**
 * Converts JSON text to a boolean value.
 *
 * Converts <code>true</code> and <code>false</code> to C values 1 and 0.
 *
 * Non-boolean input causes #errno to be set to #EINVAL,
 * and the return result approximated Javascript's "falsey" values.
 * Most non-boolean input is converted to 1 (true), but the following
 * values convert to 0:
 * <code>false</code>,
 * <code>null</code>,
 * <code>""</code>,
 * <code>0</code>,
 * <code>undefined</code>,
 * <code>NaN</code>,
 * and empty or @c NULL input.
 *
 * Note that non-empty JSON strings convert to true (#EINVAL), including
 * <code>"false"</code> and <code>"0"</code>.
 *
 * @param json  (optional) JSON text
 * @return 0 for <code>false</code>; or
 *         1 for <code>true</code>; or
 *         0 and sets #errno to #EINVAL when @a json is @c NULL; or
 *         0 and sets #errno to #EINVAL when @a json is a "falsey" value; or
 *         1 and sets #errno to #EINVAL for other values
 */
int json_as_bool(const __JSON char *json);

/**
 * Returns true iff the JSON text is exactly <code>null</code>.
 *
 * @param json  (optional) JSON text
 * @return 0 unless @a json is <code>null</code>
 */
int json_is_null(const __JSON char *json);


/**
 * Convert JSON into a UTF-8 C string.
 *
 * The conversion to string depends on the type of source JSON value:
 * <ul><li>quoted strings: as described below
 *     <li>objects and arrays: empty string, #EINVAL
 *     <li>missing tokens, NULL: empty string, #EINVAL
 *     <li>boolean, numbers, <code>null</code>, other words: as if quoted,
 *         but escape sequences are ignored
 * </ul>
 *
 * JSON quoted strings are converted into NUL-terminated, shortest-form
 * UTF-8 C strings, with the exception reagrding 'unsafe' input.strings.
 * Unsafe strings result in an empty string, with #errno set to #EINVAL.
 * An input string is considered unsafe if it would not convert to strict
 * UTF-8. This happens when any of the input UTF-8:
 * <ul><li>is an overlong encoding (eg C0 80 for U+0);
 *     <li>has a sequence that decodes to any codepoint in the
 *         unsafe codepoint set, {U+0, U+D800..U+DFFF, U+110000..};
 *     <li>contains an escape sequence that would decode to an
 *         unsafe codepoint
 * </ul>
 *
 * If a supplied @a bufsz is too small to hold the whole result, the output
 * is truncated at the nearest UTF-8 sequence boundary such that a terminating
 * NUL byte can be written (unless @a bufsz is zero).
 * A non-zero return value will always indicate the minimum buffer size
 * required to avoid such truncation during conversion.
 *
 * On error, this function returns 0 and sets #errno to #EINVAL. It
 * will fill the output buffer with a best-effort nul-terminated translation,
 * storing unsafe codepoints as U+FFFD.
 *
 * 'Words' is the name for unquoted JSON values terminated by whitespace,
 * NUL, or one of <code>[]{}:,"</code>. Words includes numbers, booleans
 * and <code>null</code>. Because of this rule, JSON expressions like
 * <code>{foo:bar}</code> convert the same as <code>{"foo":"bar"}</code>.
 *
 * @param json  (optional) input JSON text
 * @param buf   storage for the returned UTF-8 string.
 *              This will always be NUL-terminated if @a bufsz > 0,
 *              regardless of whether an error occurs or not.
 * @param bufsz the size of @a buf, or 0 if only a return value is wanted
 * @return the minimum @a bufsz required to convert the input without
 *         truncation; or
 *         0 on error, setting #errno to #EINVAL.
 */
size_t json_as_str(const __JSON char *json, void *buf, size_t bufsz);

/**
 * Converts JSON into an unsafe UTF-8 C string.
 *
 * This conversion is the same as #json_as_str(), with the following
 * changes:
 * <ul>
 * <li>code points invalid in UTF-8 (U+0, U+D800..U+DFFF, U+110000..) or
 *     overlong encodings (eg C0 80) will have the source bytes
 *     individually mapped into U+DC00..U+DCFF.
 * </ul>
 *
 * @see #json_string_from_unsafe_str()
 *
 * @param json  (optional) input JSON text
 * @param buf   storage for the returned modified UTF-8 string.
 *              This will always be NUL-terminated if @a bufsz > 0,
 *              regardless of whether an error occurs or not.
 * @param bufsz the size of @a buf, or 0 if only a return value is wanted
 * @return the minimum @a bufsz required to convert the input without
 *         truncation; or
 *         0 on error, setting #errno to #EINVAL.
 */
size_t json_as_unsafe_str(const __JSON char *json, void *buf, size_t bufsz);

/**
 * Convert JSON into a UTF-8 C string on the heap.
 *
 * This is the same conversion as #json_as_str(), except that on success
 * it returns a pointer to storage allocated by #malloc(), and on error
 * it returns @c NULL.
 *
 * Note that JSON <code>null</code> will be converted into the
 * C string <code>"null"</code> instead of @c NULL.
 * (Use #json_is_null() to test if the JSON value is <code>null</code>.)
 *
 * @param json  (optional) JSON text
 * @return a newly allocated C string containing (modified) UTF-8; or
 *         @c NULL on conversion error (#EINVAL); or
 *         @c NULL if allocation failed (#ENOMEM)
 *
 * The caller is responsible for calling #free() on the returned pointer.
 */
__attribute__((malloc))
char *json_as_strdup(const __JSON char *json);

/**
 * Convert JSON into an unsafe UTF-8 C string on the heap.
 *
 * This is the equivalent of #json_as_strdup() except that unsafe UTF-8
 * input is byte-mapped into U+DC00..U+DCFF.
 *
 * @param json  (optional) JSON text
 * @return a newly allocated C string containing (modified) UTF-8; or
 *         @c NULL on conversion error (#EINVAL); or
 *         @c NULL if allocation failed (#ENOMEM)
 *
 * The caller is responsible for calling #free() on the returned pointer.
 */
__attribute__((malloc))
char *json_as_unsafe_strdup(const __JSON char *json);

/**
 * Convert BASE-64 JSON string into an array of bytes.
 *
 * This function is intended for recovering binary data from JSON strings.
 * Whitespace is permitted (and ignored) within the BASE-64 string.
 *
 * @param json  (optional) JSON text
 * @param buf   (optional) storage for the returned binary data
 *              the minimum buffer size required.
 * @param bufsz the size of @a buf
 * @return the number of bytes stored in @a buf on success, or
 *         when @a bufsz is zero, the buffer size required to hold the data; or
 *         -1 #ENOMEM if the buffer size is too small; or
 *         -1 #EINVAL if the JSON text is not a valid BASE-64 JSON string.
 */
int json_as_base64(const __JSON char *json, void *buf, size_t bufsz);

/**
 * Convert binary data into a BASE-64, quoted JSON string.
 *
 * Use #JSON_BASE64_DSTSZ to determine the buffer size required.
 * If the buffer is too small, it will be truncated with NUL and
 * the function will return 0 setting #errno to #ENOMEM.
 *
 * @param src   source bytes
 * @param srcsz length of source
 * @param dst   output buffer for JSON string. This will be NUL-terminated.
 * @param dstsz maximum size of the output buffer
 * @return the number of non-NUL bytes that were stored in @a dst, or
 *         0 and sets #errno to #ENOMEM if @a dstsz is too small.
 */
size_t json_base64_from_bytes(const void *src, size_t srcsz,
			      __JSON char *dst, size_t dstsz);

/** Calculate the dstsz required for #json_base64_from_bytes() */
#define JSON_BASE64_DSTSZ(srcsz)   (3 + ((srcsz + 2) / 3) * 4)

/**
 * Converts a UTF-8 C string into a quoted JSON string.
 *
 * If the source string contains invalid UTF-8 characters, this
 * function will return 0 and set #errno to #EINVAL.
 *
 * If the @a dstsz is 0, then the minimum destination buffer size
 * will be computed and returned.
 *
 * If there is insufficient space in the @a dst buffer, this function
 * will still fill and NUL-terminate it, then return 0 and #ENOMEM.
 *
 *
 * @param src   (optional) NUL-terminated source string in UTF-8
 * @param dst   output buffer, will be NUL terminated.
 * @param dstsz output buffer size, or 0 to indicate a size request
 * @return 0 and #errno #EINVAL if the source string contains invalid UTF-8; or
 *         0 and #errno #EINVAL if the source string is @c NULL; or
 *         0 and #errno #ENOMEM if the dst buffer has been truncated; or
 *         the minimum size of the buffer required if @a dstsz is zero; or
 *         the number of non-NUL characters stored in @a dst.
 */
size_t json_string_from_str(const char *src,
				__JSON char *dst, size_t dstsz);

/**
 * @see #json_string_from_str().
 * @param src    NUL-terminated source string in UTF-8
 * @param srclen number of bytes in @a src to use
 * @param dst    output buffer, will be NUL terminated.
 * @param dstsz  output buffer size, or 0 to indicate a size request
 * @return same as #json_string_from_str()
 *         except that @a src is not checked for @c NULL.
 */
size_t json_string_from_strn(const char *src, int srclen,
				__JSON char *dst, size_t dstsz);

/**
 * Converts an unsafe UTF-8 C string into a quoted JSON string value.
 * Codepoints U+DC00..U+DCFF are mapped into single bytes.
 *
 * Invalid UTF-8 in the source string is copied through to
 * the string body without change.
 *
 * This function is otherwise similar to #json_string_from_str().
 *
 * @param src   (optional) NUL-terminated source string in UTF-8
 * @param dst   output buffer, will be NUL terminated.
 * @param dstsz output buffer size, or 0 to indicate a size request
 * @return 0 and #errno #EINVAL if the source string is @c NULL; or
 *         0 and #errno #ENOMEM if the dst buffer has been truncated; or
 *         the minimum size of the buffer required if @a dstsz is zero; or
 */
size_t json_string_from_unsafe_str(const char *src,
				__JSON char *dst, size_t dstsz);

/**
 * @see #json_string_from_unsafe_str().
 * @param src    NUL-terminated source string in UTF-8
 * @param srclen number of bytes in @a src to use
 * @param dst    output buffer, will be NUL terminated.
 * @param dstsz  output buffer size, or 0 to indicate a size request
 * @return same as #json_string_from_str()
 *         except that @a src is not checked for @c NULL.
 */
size_t json_string_from_unsafe_strn(const char *src, int srclen,
				__JSON char *dst, size_t dstsz);

/**
 * Compares a JSON value with a UTF-8 C string.
 *
 * This is a convenience function analogous to #strcmp() that
 * compares a UTF-8 C string with a JSON value that is usually
 * a JSON quoted string.
 *
 * This function will act as though @a json had been converted to
 * UTF-8 string by #json_as_str(), and then compared to the UTF-8 in
 * @a cstr. UTF-8 validity of @a cstr is not checked.
 * (It is a property of UTF-8 that bytewise comparison of valid UTF-8
 * strings yields the the same result as codepoint-wise comparison.)
 *
 * @note The JSON value <code>null</code> will compare the same as the
 *       string <code>"null"</code>. @see #json_is_null().
 *
 * @param json (optional) JSON text
 * @param cstr UTF-8 C string to compare against
 *
 * @return  0 if the JSON value compares equal to the string; or
 *         -1 if the JSON string is lexicographically 'smaller' or
 *            if @a json is <code>null</code> or malformed, or
 *         +1 if the JSON string is lexicographically 'larger'.
 */
int json_strcmp(const __JSON char *json, const char *cstr);

/**
 * @return same as #json_strcmp()
 *         except that only the first @a cstrsz bytes of @a cstr are compared
 */
int json_strcmpn(const __JSON char *json, const char *cstr, size_t cstrsz);

extern const char json_true[];	/* "true" */
extern const char json_false[];	/* "false" */
extern const char json_null[];	/* "null" */

#endif /* JSON_H */
