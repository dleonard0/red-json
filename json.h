#ifndef RED_JSON_H
#define RED_JSON_H

/**
 * @mainpage Red JSON parser
 *
 * @section intro Introduction
 *
 * This is a lightweight, just-in-time parser for JSON RFC 7159.
 * Instead of converting all the JSON input into in-memory data structure,
 * you keep the source input as-is, and use the functions below to seek
 * around, converting the JSON values to C as you need them.
 *
 * JSON input text is always NUL-terminated UTF-8, or NULL.
 * NULL is always safe and treated as if it were an empty JSON input.
 *
 * Interior values within structured values are represented by pointers
 * into the JSON text where their encoding begins. This is true for both
 * simple and structured sub-values.
 *
 *
 * @section types  Simple type conversion
 *
 * Converting any JSON value into a simple C type returns a "best-effort"
 * result, and @c errno will indicate any conversion difficulty. For example,
 * converting JSON number <code>1e100</code> to @c int will clamp the value
 * to INT_MAX and indicate ERANGE.
 *
 * If you need to know the type of a JSON value before converting, there is
 * a fast type classifier #json_type() that guesses without fully checking
 * whether the input is well-formed. Even when the input isn't well-formed,
 * the converter functions will still try to return something useful and
 * indicate EINVAL.
 *
 * @see #json_as_int(),
 *      #json_as_long(),
 *      #json_as_double(),
 *      #json_as_bool(),
 *      #json_is_null(),
 *      #json_type()
 *
 *
 * @section Strings
 *
 * Normally, a string conversion error (EINVAL) occurs when
 * <ul><li>the input JSON contains invalid or overlong UTF-8 sequences, or
 *     <li>the output UTF-8 C string would contain NUL or a
 *         code point not permitted by RFC 3629.
 * </ul>
 *
 * However, "unsafe" variants of the string conversion functions are provided
 * that instead translate problematic input bytes into U+DC00..U+DCFF.
 * Converting these code points back to JSON will losslessly preserve the
 * original bytes.
 * If you need to pass binary data with JSON, you may prefer to use the
 * BASE64 functions.
 *
 * Most string conversions requires you to supply the output buffer.
 * If you supply a zero-sized buffer, the conversion functions will return
 * the minimum size of buffer to use for that input.
 * Functions ending with "_strdup" will do that step for you and
 * return heap storage filled with the translated string.
 *
 * If all you want to do is compare a JSON string with a UTF-8 C string,
 * you can use #json_strcmp() which performs the translation and comparison
 * in one step without requiring an output buffer or conversion step.
 *
 * @see #json_as_str(),          #json_string_from_str(),
 *      #json_as_unsafe_str(),   #json_string_from_unsafe_str(),
 *      #json_as_base64(),       #json_base64_from_bytes(),
 *      #json_as_strdup(),
 *      #json_as_unsafe_strdup(),
 *      #json_strcmp(),
 *      #json_strcmpn()
 *
 *
 * @section Selection
 *
 * 'Selection' is seeking within a nest of arrays and objects to access
 * a particular value. 'Selectors' are expessions modeled after
 * Javascript syntax.
 *
 * For example, the selction path "foo[1].bar" applied to the JSON value below
 * will select the value 7.
 * <pre>
 *        {
 *          "foo": [
 *                   null,
 *                   { "bar": 7 }
 *                 ]
 *        }
 * </pre>
 *
 * Selectors can also use %d or %s to refer to variadic arguments.
 *
 * @see #json_select(),
 *      #json_selectv(),
 *      #json_select_int(),     #json_default_select_int(),
 *      #json_select_bool(),    #json_default_select_bool(),
 *      #json_select_double(),  #json_default_select_double(),
 *      #json_select_array(),   #json_default_select_array(),
 *      #json_select_object(),  #json_default_select_object(),
 *      #json_select_strdup(),  #json_default_select_strdup()
 *
 *
 * @section Iteration
 *
 * 'Iteration' is accessing elements of an array (or members of objects)
 * in sequence. A simple pointer ('iterator') is used to keep track of the
 * position within the sequence.
 *
 * First, an array or object iterator is obtained, then the '_next'
 * function is applied repeatedly to either get a pointer to the next value,
 * or a NULL indication that the iteration is exhausted.
 *
 * @see #json_as_array(),       #json_as_object(),
 *      #json_array_next(),     #json_object_next()
 *
 *
 * @section Spans
 *
 * It can be useful sometimes to have access to an entire sub-value
 * without interpretation. For example, you may be wrapping a JSON
 * query within another. The 'span' of a value is the number of bytes
 * it occupies within the JSON input text.
 *
 * @see #json_span()
 *
 *
 * @section limits Extensions and limitations
 *
 * The parsers accept the following extensions to JSON (RFC 7159):
 * <ul>
 * <li> Extra commas may be present at the end of arrays and objects;
 * <li> Unnecessary escapes inside quoted strings are converted to U+DC5C;
 * <li> Strings may be single-quoted. Inside single quoted (') strings, the
 *      double quote (") need not be escaped (\\"), but single quotes must
 *      be (\\').
 * <li> Bare strings, called "words" and matching <code>[^[]{},:" \\t\\n\\r]+</code>
 *      are accepted where quoted strings are expected.
 *      Backslashes within words are treated literally.
 * <li> UTF-8 input that does not conform to RFC3629 is accepted by some
 *      functions. They are marked "unsafe".
 * </ul>
 *
 * The parser has the following limits:
 * <ul>
 * <li>Only UTF-8 is understood (not UTF-16 other encodings)
 * <li>Nesting of arrays and objects is limited to 32768, combined.
 * </ul>
 *
 *
 * @section generate Generating JSON
 *
 * You can use @c snprintf() to generate most of your own JSON, assisted with
 * the following functions and string constants.
 *
 * @see #json_string_from_str(),
 *      #json_string_from_unsafe_str(),
 *      #json_base64_from_bytes(),
 *      #json_true,
 *      #json_false,
 *      #json_null
 *
 *
 * @author David Leonard; released into Public Domain 2016
 */

/** @file */

#include <stdarg.h>
#include <stdlib.h>

/* Type qualifiers used to document function signatures */
#define __JSON         /**< Indicates the string contains JSON */
#define __JSON_ARRAYI  /**< Indicates pointer is an array iterator */
#define __JSON_OBJECTI /**< Indicates pointer is an object iterator */

/**
 * Selects an element within a JSON structure.
 *
 * For example, the path "[1].name" will select the value "Fred" from
 * <code>[{"name":"Tim", "age":28},{"name":"Fred", "age":26}]</code>.
 *
 * A selection path is any sequence of <code>[<i>index</i>]</code> or
 * <code>.<i>key</i></code> path components.
 * Path components can be literal (e.g. <code>[7]</code> or <code>.foo</code>)
 * or parameter references (<code>[%u]</code> or <code>.%s</code>, which
 * expect parameteric arguments of type <code>unsigned int</code>
 * or <code>const char *</code>, respectively).
 *
 * There are some limitations on object key path components:
 * <ul>
 *     <li>literal <code>.<i>key</i></code> must not contain '.' or '['
 *         characters, and occurrences of '%' must be doubled.
 *     <li>literal keys and key parameter strings must be encoded in
 *         shortest-form UTF-8 because #json_strcmp() is used to compare them.
 * </ul>
 *
 * <code>[%d]</code> may be used in place of <code>[%u]</code> and expects
 * an argument of type @c int. Negative indicies will result in EINVAL.
 *
 * @param json  (optional) JSON value to select within
 * @param path  selection path, matches <code>(.key|.%s|[int]|[%u])*</code>
 *              The leading '.' may be omitted.
 * @param ...   Variadic parameters for each "%s" and "[%u]"
 *              component of the @a path
 *
 * @returns pointer within @a json to the selected value
 * @retval  NULL  [ENOENT] The path was not found in the value
 * @retval  NULL  [EINVAL] The path was malformed
 * @retval  NULL  [EINVAL] An array index was negative
 */
const __JSON char *json_select(const __JSON char *json, const char *path, ...)
    __attribute__((format(printf,2,3)));

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
 * only when the selector path is not found. */
int json_default_select_int(int default_, const __JSON char *json,
	const char *path, ...);
int json_default_select_bool(int default_, const __JSON char *json,
	const char *path, ...);
double json_default_select_double(double default_, const __JSON char *json,
	const char *path, ...);
const __JSON_ARRAYI char *json_default_select_array(
	const __JSON_ARRAYI char *default_, const __JSON char *json,
	const char *path, ...);
const __JSON_OBJECTI char *json_default_select_object(
	const __JSON_OBJECTI char *default_, const __JSON char *json,
	const char *path, ...);
char *json_default_select_strdup(
	const char *default_, const __JSON char *json,
	const char *path, ...)
	__attribute__((malloc));

/**
 * Begins iterating over a JSON array.
 *
 * Example usage:
 *
 * @code
 *     const char *iter;
 *     const char *elem;
 *
 *     iter = json_as_array("[1,2,3]");
 *     while ((elem = json_array_next(&iter))) {
 *         printf("%d\n", json_as_int(elem));
 *     }
 * @endcode
 *
 * @param json  (optional) JSON text
 *
 * @returns initial element pointer, for use with #json_array_next()
 * @retval NULL [EINVAL] the value is not an array
 */
const __JSON_ARRAYI char *json_as_array(const __JSON char *json);

/**
 * Accesses the next element of a JSON array.
 * Then advances the array iterator.
 *
 * @param index_ptr storage holding the array iterator.
 *                  It should be initialized from #json_as_array().
 *                  This will be advanced by one position.
 *
 * @returns the next element of the array
 * @retval NULL There are no more elements in the array.
 * @retval NULL [EINVAL] The array is malformed.
 */
const __JSON char *json_array_next(const __JSON_ARRAYI char **index_ptr);

/**
 * Begins iterating over a JSON object.
 *
 * Example usage:
 * <code>
 *     const char *iter;
 *     const char *key, *value;
 *     char buf[256];
 *
 *     iter = json_as_object("{a:1, b:2}");
 *     while ((value = json_object_next(&iter, &key))) {
 *         printf("%s: %d\n", json_as_str(key, buf, sizeof buf),
 *                            json_as_int(value));
 *     }
 * </code>
 *
 * @param json  (optional) JSON text
 *
 * @returns initial index pointer, for use with #json_object_next()
 * @retval NULL [EINVAL] The value is not an object.
 */
const __JSON_OBJECTI char *json_as_object(const __JSON char *json);

/**
 * Accesses the next key and value of a JSON object.
 * Then advances the object iterator.
 *
 * @param index_ptr   storage holding the object iterator.
 *                    It should be initialized from #json_as_object().
 *                    This will be advanced by one position.
 * @param key_return  (optional) storage for the next member's key.
 *
 * @returns the next member's value
 * @retval NULL There are no more members in the object.
 * @retval NULL [EINVAL] The object is malformed.
 */
const __JSON char *json_object_next(const __JSON_OBJECTI char **index_ptr,
		const __JSON char **key_return);

/** The type of a JSON value, as guessed by #json_type(). */
enum json_type {
	JSON_BAD = 0,	/**< The value is NULL, starts with an
	                 *   invalid character, comma, closing brace,
			 *   colon or is an empty string. */
	JSON_OBJECT,	/**< The value starts with @c { */
	JSON_ARRAY,	/**< The value starts with @c [ */
	JSON_BOOL,	/**< The value starts with @c t or @c f */
	JSON_NULL,	/**< The value starts with @c n */
	JSON_NUMBER,	/**< The value starts with @c [-+.0-9] */
	JSON_STRING	/**< The value starts with @c " */
};

/**
 * Estimates the type of a JSON value.
 *
 * Only the JSON text's first non-whitespace character is used to
 * estimate the type.
 * This is reliable if the input is valid JSON.
 * Converter function errors can be used to find out if that is so.
 *
 * @param json  (optional) JSON text
 *
 * @returns the estimated value type
 */
enum json_type json_type(const __JSON char *json);

/**
 * Determines the span in bytes of a JSON value.
 *
 * The span of a value is the minimum number of bytes it occupies in the
 * JSON text.
 *
 * The span can be used to copy out and re-use sub-structures without
 * a conversion step.
 *
 * The span will include leading whitespace, but not trailing whitespace.
 *
 * @param json  (optional) JSON text
 *
 * @returns the number of bytes from @a json to just after the end of the
 *             first value found
 * @retval 0 [EINVAL] The JSON text is invalid or malformed.
 * @retval 0 [ENOMEM] The substructure exceeds a nesting limit.
 */
size_t json_span(const __JSON char *json);

/**
 * Converts JSON to a floating-point number.
 *
 * The conversion depends on the JSON text:
 * <ul>
 * <li><code>true</code>, <code>false</code>, <code>null</code>,
 *     arrays, objects and EOS convert to NaN and set @c errno to @c EINVAL.
 * <li>JSON strings are converted using the next step after skipping the
 *     leading quote. Any escapes within the JSON string are
 *     left unexpanded.
 * <li>JSON numbers are converted using your libc's @c strtod(),
 *     which may understand special sequences such as "Inf" or "NaN(7)".
 *     The JSON text is not canonicalized before calling @c strtod().
 * </ul>
 *
 * @param json pointer to JSON text
 *
 * @returns the value converted to floating-pointer
 * @retval  NAN [EINVAL] The JSON text is invalid or malformed.
 * @retval  HUGE_VAL [ERANGE] The value was too positive.
 * @retval -HUGE_VAL [ERANGE] The value was too negative.
 * @retval  0 [ERANGE] The value was too small and underflowed.
 */
double json_as_double(const __JSON char *json);

/**
 * Converts JSON to a long integer.
 *
 * JSON values are converted as follows:
 * <ul>
 * <li><code>true</code> converts to 1L
 * <li><code>false</code> converts to 0L
 * <li><code>null</code>, arrays, objects and EOS convert to 0L
 *     and set @c errno to @c EINVAL
 * <li>JSON strings are converted using the next step after skipping the
 *     leading quote. Any escapes within the JSON string are
 *     left unexpanded.
 * <li>JSON numbers are first converted using @c strtoul(). If this conversion
 *     ends at a '.', 'e' or 'E', then @c strtod() is used and its result
 *     is clamped to [@c LONG_MIN, @c LONG_MAX] and sets @c errno to @c ERANGE.
 *     Underflow from @c strtod() is ignored, and standard C truncation
 *     (rounding towards zero) is applied to get an integer.
 * </ul>
 *
 * @param json  (optional) JSON text
 *
 * @returns the value converted to a long integer
 * @retval 0 [EINVAL] The value was @c NULL or invalid.
 * @retval LONG_MIN [ERANGE] the value was too negative.
 * @retval LONG_MAX [ERANGE] the value was too positive.
 */
long json_as_long(const __JSON char *json);

/**
 * Converts JSON to an integer.
 *
 * This is the same as #json_as_long() but clamps the value
 * to [@c INT_MIN, @c INT_MAX].
 *
 * @param json  (optional) JSON text
 *
 * @returns the value converted to an integer
 * @retval 0 [EINVAL] The value was @c NULL or invalid.
 * @retval INT_MIN [ERANGE] the value was too negative.
 * @retval INT_MAX [ERANGE] the value was too positive.
 */
int json_as_int(const __JSON char *json);

/**
 * Converts JSON to a boolean value.
 *
 * Converts <code>true</code> and <code>false</code> to C values 1 and 0.
 *
 * Non-boolean input causes @c errno to be set to @c EINVAL,
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
 * Note that non-empty JSON strings convert to true [EINVAL], including
 * <code>"false"</code> and <code>"0"</code>.
 *
 * @param json  (optional) JSON text
 *
 * @retval nonzero The value was <code>true</code>.
 * @retval 0 The value was <code>false</code>.
 * @retval 0 [EINVAL] The value was <code>null</code>.
 * @retval 0 [EINVAL] The value was <code>""</code> or <code>''</code>.
 * @retval 0 [EINVAL] The value was <code>0</code>.
 * @retval 0 [EINVAL] The value was <code>undefined</code>.
 * @retval 0 [EINVAL] The value was <code>NaN</code>.
 * @retval 0 [EINVAL] The value was @c NULL or empty.
 * @retval nonzero [EINVAL] The value was not a boolean, nor falsey.
 */
int json_as_bool(const __JSON char *json);

/**
 * Tests if JSON text is exactly <code>null</code>.
 *
 * @param json  (optional) JSON text
 *
 * @retval 0 The value was not <code>null</code>.
 * @retval nonzero The value was <code>null</code>.
 */
int json_is_null(const __JSON char *json);


/**
 * Converts JSON into a UTF-8 C string.
 *
 * The conversion to string depends on the type of source JSON value:
 * <ul><li>quoted strings: as described below
 *     <li>objects and arrays: empty string [EINVAL]
 *     <li>missing tokens, NULL: empty string [EINVAL]
 *     <li>boolean, numbers, <code>null</code>, other words: as if quoted,
 *         but escape sequences are ignored
 * </ul>
 *
 * JSON quoted strings are converted into NUL-terminated, shortest-form
 * UTF-8 C strings, with an exception regarding 'unsafe' strings.
 * Input strings that would convert to an 'unsafe' output string will,
 * instead, convert to an empty output string and set @c errno to @c ERANGE.
 * An UTF-8 string is considered unsafe if it contains:
 * <ul><li>an overlong encoding (eg C0 80 for U+0);
 *     <li>a sequence that decodes to any codepoint in the
 *         unsafe codepoint set, {U+0, U+D800..U+DFFF, U+110000..};
 *     <li>an escape sequence that would decode to an unsafe codepoint
 * </ul>
 *
 * If @a bufsz is zero, the function computes the minimum buffer size
 * to supply and returns that.
 *
 * If @a bufsz is non-zero and too small to hold the whole result, the
 * output
 * is truncated at the nearest UTF-8 sequence boundary such that a terminating
 * NUL byte can be written (unless @a bufsz is zero).
 * A non-zero return value will always indicate the minimum buffer size
 * required to avoid such truncation during conversion.
 *
 * On error, this function returns 0 and sets @c errno to @c EINVAL. It
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
 *
 * @returns the minimum @a bufsz required to convert the input without
 *          truncation; or
 * @retval 0 [EINVAL] The JSON text was invalid or malformed.
 * @retval 0 [EINVAL] The JSON string was malformed.
 * @retval 0 [EINVAL] The JSON string was well-formed, but the
 *                    output buffer would have received invalid UTF-8.
 */
size_t json_as_str(const __JSON char *json, void *buf, size_t bufsz);

/**
 * Converts JSON into an unsafe UTF-8 C string.
 *
 * This conversion is the same as #json_as_str(), except that
 * invalid input sequences from the input JSON text are mapped byte-wise
 * into U+DC00..U+DCFF and then stored as UTF-8 in the output buffer.
 *
 * The invalid sequences so mapped are:
 * <ul>
 * <li>Invalid UTF-8 encodings;
 * <li>Overlong UTF-8 encodings;
 * <li>Correct UTF-8 encodings of {U+0, U+D800..U+DFFF, U+110000..};
 * <li>Unicode escapes (<code>\\uXXXX</code>) of {U+0, U+D800..U+DFFF}
 *     (except for 12-byte surrogate pairs explained by RFC7159).
 * </ul>
 *
 * @see #json_string_from_unsafe_str()
 *
 * @param json  (optional) input JSON text
 * @param buf   storage for the returned modified UTF-8 string.
 *              This will always be NUL-terminated if @a bufsz > 0,
 *              regardless of whether an error occurs or not.
 * @param bufsz the size of @a buf, or 0 if only a return value is wanted
 *
 * @returns the minimum @a bufsz required to convert the input without
 *          truncation; or
 * @retval 0 [EINVAL] on error
 */
size_t json_as_unsafe_str(const __JSON char *json, void *buf, size_t bufsz);

/**
 * Converts JSON into a UTF-8 C string on the heap.
 *
 * This is the same conversion as #json_as_str(), except that on success
 * it returns a pointer to storage allocated by @c malloc(), and on error
 * it returns @c NULL.
 *
 * Note that JSON <code>null</code> will be converted into the
 * C string <code>"null"</code> instead of @c NULL.
 * (Use #json_is_null() to test if the JSON value is <code>null</code>.)
 *
 * @param json  (optional) JSON text
 *
 * @returns a newly allocated C string containing (modified) UTF-8
 * @retval NULL [EINVAL] Conversion error.
 * @retval NULL [ENOMEM] Allocation failed.
 *
 * The caller is responsible for calling @c free() on the returned pointer.
 */
char *json_as_strdup(const __JSON char *json)
    __attribute__((malloc));

/**
 * Converts JSON into an unsafe UTF-8 C string on the heap.
 *
 * This is the equivalent of #json_as_strdup() except that
 * invalid sequences are byte-mapped into U+DC00..U+DCFF the
 * same as #json_as_unsafe_str().
 *
 * @param json  (optional) JSON text
 *
 * @returns a newly allocated C string containing (modified) UTF-8; or
 * @retval NULL [EINVAL] Conversion error.
 * @retval NULL [ENOMEM] Allocation failed.
 *
 * The caller is responsible for calling @c free() on the returned pointer.
 */
char *json_as_unsafe_strdup(const __JSON char *json)
    __attribute__((malloc));

/**
 * Converts BASE-64 JSON string into an array of bytes.
 *
 * This function is intended for recovering binary data from JSON strings.
 * Whitespace is permitted (and ignored) within the BASE-64 string.
 *
 * @see RFC 3548
 *
 * @param json  (optional) JSON text
 * @param buf   (optional) storage for the returned binary data
 * @param bufsz the size of @a buf
 *
 * @returns the number of bytes stored in @a buf on success, or
 *          the minimum buffer size required when @a bufsz is zero
 * @retval -1 [ENOMEM] The buffer size is too small.
 * @retval -1 [EINVAL] The JSON text is not a valid BASE-64 JSON string.
 */
int json_as_base64(const __JSON char *json, void *buf, size_t bufsz);

/**
 * Converts binary data into a BASE-64, quoted JSON string.
 *
 * Use #JSON_BASE64_DSTSZ() to determine the buffer size required.
 * If the buffer is too small, it will be truncated with NUL and
 * the function will return 0 setting @c errno to @c ENOMEM.
 *
 * @see RFC 3548
 *
 * @param src   source bytes
 * @param srcsz length of source
 * @param dst   output buffer for JSON string. This will be NUL-terminated.
 * @param dstsz maximum size of the output buffer
 *
 * @returns the number of non-NUL bytes that were stored in @a dst
 * @retval -1 [ENOMEM] the @a dstsz is too small
 */
int json_base64_from_bytes(const void *src, size_t srcsz,
			      __JSON char *dst, size_t dstsz);

/** Calculates the dstsz required for #json_base64_from_bytes() */
#define JSON_BASE64_DSTSZ(srcsz)   (3 + ((srcsz + 2) / 3) * 4)

/**
 * Converts a UTF-8 C string into a quoted JSON string.
 *
 * If the source string contains invalid UTF-8 characters, this
 * function will return 0 and set @c errno to @c EINVAL.
 *
 * If the @a dstsz is 0, then the minimum destination buffer size
 * will be computed and returned.
 *
 * If there is insufficient space in the @a dst buffer, this function
 * will still fill and NUL-terminate it, then return 0 [ENOMEM].
 *
 *
 * @param src   (optional) NUL-terminated source string in UTF-8
 * @param dst   output buffer, will be NUL terminated.
 * @param dstsz output buffer size, or 0 to indicate a size request
 *
 * @returns the minimum size of the buffer required if @a dstsz is zero; or
 *         the number of non-NUL characters stored in @a dst.
 * @retval 0 [EINVAL] if the source string contains invalid UTF-8
 * @retval 0 [EINVAL] if the source string is @c NULL
 * @retval 0 [ENOMEM] if the dst buffer has been truncated
 */
size_t json_string_from_str(const char *src,
				__JSON char *dst, size_t dstsz);

/**
 * @see #json_string_from_str().
 *
 * @param src    source string in UTF-8
 * @param srclen number of bytes in @a src to use
 * @param dst    output buffer, will be NUL terminated.
 * @param dstsz  output buffer size, or 0 to indicate a size request
 *
 * @returns same as #json_string_from_str()
 */
size_t json_string_from_strn(const char *src, int srclen,
				__JSON char *dst, size_t dstsz);

/**
 * Converts from an unsafe UTF-8 C string into a quoted JSON string value.
 *
 * An unsafe UTF-8 string is one that contains codepoints
 * U+DC00..U+DCFF. This function unwraps them back into individual bytes.
 *
 * Otherwise invalid UTF-8 in the unsafe source string causes this
 * function to return 0 [EINVAL].
 *
 * This function is otherwise similar to #json_string_from_str().
 *
 * @param src   (optional) NUL-terminated source string in UTF-8
 * @param dst   output buffer, will be NUL terminated.
 * @param dstsz output buffer size, or 0 to indicate a size request
 *
 * @returns the minimum size of the buffer filled or required
 * @retval 0 [EINVAL] if the source string contains invalid UTF-8
 *                    other than U+DC00..U+DCFF
 * @retval 0 [EINVAL] if the source string is @c NULL
 * @retval 0 [ENOMEM] if the dst buffer has been truncated
 */
size_t json_string_from_unsafe_str(const char *src,
				__JSON char *dst, size_t dstsz);

/**
 * @see #json_string_from_unsafe_str().
 *
 * @param src    NUL-terminated source string in UTF-8
 * @param srclen number of bytes in @a src to use
 * @param dst    output buffer, will be NUL terminated.
 * @param dstsz  output buffer size, or 0 to indicate a size request
 *
 * @returns same as #json_string_from_str(),
 *          except that @a src is not checked for @c NULL.
 */
size_t json_string_from_unsafe_strn(const char *src, int srclen,
				__JSON char *dst, size_t dstsz);

/**
 * Compares a JSON value with a UTF-8 C string.
 *
 * This is a convenience function analogous to @c strcmp() that
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
 *       string <code>"null"</code>.
 * @see #json_is_null(), #json_strcmpn()
 *
 * @param json (optional) JSON text
 * @param cstr UTF-8 C string to compare against
 *
 * @retval  0 if the JSON value compares equal to the string
 * @retval -1 if the JSON string is lexicographically 'smaller' or
 *            if @a json is <code>null</code> or malformed
 * @retval +1 if the JSON string is lexicographically 'larger'
 */
int json_strcmp(const __JSON char *json, const char *cstr);

/**
 * @see #json_strcmp()
 *
 * @param json   (optional) JSON text
 * @param cstr   UTF-8 string segment to compare against
 * @param cstrsz length of the string segment, in bytes
 *
 * @returns same as #json_strcmp()
 */
int json_strcmpn(const __JSON char *json, const char *cstr, size_t cstrsz);

extern const char json_true[];	/**< "true" */
extern const char json_false[];	/**< "false" */
extern const char json_null[];	/**< "null" */

#endif /* RED_JSON_H */
