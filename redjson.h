#ifndef REDJSON_H
#define REDJSON_H

/**
 * @mainpage Red JSON parser
 *
 * This is a lightweight, just-in-time parser for JSON.
 *
 * Instead of converting all the JSON input into in-memory data structures,
 * you keep the source input as-is, and use the functions below to seek
 * around, converting the JSON values to C as you need them.
 *
 * JSON input text is always a pointer to NUL-terminated UTF-8, or NULL.
 * NULL is always safe and treated as if it were an empty JSON input.
 *
 * JSON values interior to arrays and objects are represented by pointers
 * to where the value encoding begins.
 *
 * @see redjson.h, README.md, RFC 7159.
 * @author David Leonard; released into Public Domain 2017
 */

/** @file */

#include <stdarg.h>
#include <stdlib.h>

/* Type qualifiers used to document function signatures */
#define __JSON         /**< Indicates the string contains JSON */
#define __JSON_ARRAYI  /**< Indicates pointer is an array iterator */
#define __JSON_OBJECTI /**< Indicates pointer is an object iterator */

/** Interface version for this header. (major << 8 | minor) */
#define REDJSON_API_VERSION 0x0001

/** Implementation version for the library, eg "1.0" */
extern const char redjson_lib_version[];

/**
 * Selects an element within a JSON structure.
 *
 * For example, the selection path "[1].name" selects the value "Fred" from
 * <code>[{"name":"Tim", "age":28},{"name":"Fred", "age":26}]</code>.
 *
 * A selection path is any sequence of <code>[<i>index</i>]</code> or
 * <code>.<i>key</i></code> path components.
 *
 * Path components can be in-pattern literals
 * (e.g. <code>[7]</code> or <code>.foo</code>)
 * or argument references
 * (e.g. <code>[%u]</code> or <code>.%s</code>, indicating
 * the following arguments of type <code>unsigned int</code>
 * or <code>const char *</code>, respectively).
 *
 * Key path components have the following restrictions:
 * <ul>
 *     <li>an in-pattern <code>.<i>key</i></code> cannot contain
 *         characters '.', '[' or '%'.
 *     <li>both in-pattern keys and argument keys must be encoded in
 *         shortest-form UTF-8 because #json_strcmpn() is used to compare them.
 * </ul>
 *
 * Index path components have the following restrictions:
 * <ul>
 *     <li>in-pattern indicies cannot begin with a minus sign.
 *     <li><code>[%d]</code> may be used, but if its @c int argument is
 *         negative, the selection algorithm will return ENOENT.
 * </ul>
 *
 * @param json  (optional) JSON value to select within
 * @param path  selection path, matches <code>(.key|.%s|[int]|[%u])*</code>
 *              The leading '.' may be omitted.
 * @param ...   Respective arguments for each <code>.%s</code> and
 *              <code>[%u]</code> component of the @a path
 *
 * @returns pointer within @a json to the selected value
 * @retval  NULL  [ENOENT] The path was not found in the value.
 * @retval  NULL  [ENOMEM] The input is too deeply nested.
 * @retval  NULL  [EINVAL] The path is malformed.
 * @retval  NULL  [EINVAL] An array index is negative.
 */
const __JSON char *json_select(const __JSON char *json, const char *path, ...)
    __attribute__((format(printf,2,3)));

/** @see #json_select() */
const __JSON char *json_selectv(const __JSON char *json, const char *path,
    va_list ap);

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
 * @retval NULL [EINVAL] the value is not an array.
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
 * @retval NULL [ENOMEM] The structure is too deeply nested.
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
 * @retval NULL [ENOMEM] The structure is too deeply nested.
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
	JSON_NUMBER,	/**< The value starts with @c [-0-9] */
	JSON_STRING	/**< The value starts with @c " */
};

/**
 * Estimates the type of a JSON value.
 *
 * This function is reliable when the input is strictly-valid JSON.
 * Only the JSON text's first non-whitespace character is used to
 * estimate the type.
 *
 * For example, the malformed JSON value <code>truu</code> will be
 * detected as #JSON_BOOL and the non-strict number <code>.5</code>
 * will be detected as #JSON_BAD.
 *
 * @param json  (optional) JSON text
 *
 * @returns the estimated type
 */
enum json_type json_type(const __JSON char *json);

/**
 * Determines the span in bytes of a JSON value.
 *
 * The span of a value is the minimum number of bytes it occupies in the
 * JSON text.
 * The span will include leading whitespace, but not trailing whitespace.
 *
 * The span can be used to copy out and re-use sub-structures without
 * the need for a conversion step.
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
 * The conversion depends on the JSON input:
 * <ul>
 * <li><code>true</code>, <code>false</code>, <code>null</code>,
 *     arrays, objects and invalid input
 *     convert to NaN and set @c errno to @c EINVAL.
 * <li>JSON strings are converted using the next step after skipping the
 *     leading quote. Escapes within the JSON string are
 *     <em>not</em> expanded.
 * <li>JSON numbers are converted using your libc's @c strtod(),
 *     which may understand special sequences such as "Inf" or "NaN(7)",
 *     or base prefixes such as "0x".
 * </ul>
 *
 * strictly a JSON number. However a best conversion is returned.
 *
 * @param json pointer to JSON text
 *
 * @returns the value converted to a floating-point nuumber, and
 *          @c errno is set to @c EINVAL when the input is not
 *          a strict a JSON number.
 * @retval  NAN [EINVAL] The JSON text is invalid or malformed.
 * @retval  HUGE_VAL [ERANGE] The value is too positive.
 * @retval -HUGE_VAL [ERANGE] The value is too negative.
 * @retval  0 [ERANGE] The value is too small and conversion underflowed.
 */
double json_as_double(const __JSON char *json);

/**
 * Converts JSON to a long integer.
 *
 * The conversion depends on the JSON input:
 * <ul>
 * <li>JSON numbers are first converted using @c strtoul().
 *     If this conversion does not end at a JSON delimiting character
 *     (such as whitespace or a comma), it is restarted using
 *     #json_as_double().  If the double result succeeds and exceeds
 *     [@c LONG_MIN, @c LONG_MAX] the value is clamped and @c errno
 *     is set to @c ERANGE.
 *     C truncation (rounding towards zero) is applied to get an integer.
 *     Underflow from @c strtod() is ignored.
 * <li>For all other values, #json_as_double() is called.
 *     A @c NAN result is translated to 0.
 *     the same truncation and clamping rules are applied as above, and
 *     @c errno is always set to @c EINVAL.
 * </ul>
 *
 * @param json  (optional) JSON text
 *
 * @returns the value converted to a long integer, and
 *          @c errno is set to @c EINVAL when the input is not
 *          a strict a JSON number.
 * @retval 0 [EINVAL] The JSON text is invalid, malformed or not a number.
 * @retval LONG_MIN [ERANGE] The number is too large.
 * @retval LONG_MAX [ERANGE] The number is too large.
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
 * @returns the value converted to an integer, and
 *          @c errno is set to @c EINVAL when the input is not
 *          a strict a JSON number.
 * @retval 0 [EINVAL] The JSON text is invalid, malformed or not a number.
 * @retval INT_MIN [ERANGE] The number is too large.
 * @retval INT_MAX [ERANGE] The number is too large.
 */
int json_as_int(const __JSON char *json);

/**
 * Converts JSON to a boolean value.
 *
 * Converts <code>true</code> and <code>false</code> to C values 1 and 0.
 *
 * Converts non-boolean inputs according to
 * Javascript's "falsey" rules, and sets @c errno to @c EINVAL.
 *
 * The following falsey values convert to 0:
 * <code>false</code>,
 * <code>null</code>,
 * <code>""</code>,
 * <code>0</code>,
 * <code>undefined</code>,
 * <code>NaN</code>,
 * and empty or @c NULL input.
 * All other values convert to 1.
 *
 * Note that non-empty quoted JSON strings convert to true [EINVAL], including
 * <code>"false"</code>, <code>"undefined"</code> and <code>"0"</code>.
 *
 * @param json  (optional) JSON text
 *
 * @retval nonzero The value is <code>true</code>.
 * @retval 0 The value is <code>false</code>.
 * @retval 0 [EINVAL] The value is <code>null</code>,
 *                                 <code>""</code>, <code>''</code>,
 *                                 <code>0</code>,
 *                                 <code>undefined</code>,
 *                                 <code>NaN</code>,
 *                                 @c NULL or empty.
 * @retval nonzero [EINVAL] The value is not a boolean.
 */
int json_as_bool(const __JSON char *json);

/**
 * Tests if JSON text is exactly <code>null</code>.
 *
 * @param json  (optional) JSON text
 *
 * @retval 0 The value is not <code>null</code>.
 * @retval nonzero The value is <code>null</code>.
 */
int json_is_null(const __JSON char *json);


/**
 * Converts JSON into a UTF-8 C string.
 *
 * The conversion to string depends on the type of source JSON value:
 * <ul><li>quoted strings: as described below
 *     <li>objects and arrays: empty string [EINVAL]
 *     <li>missing values, @c NULL: empty string [EINVAL]
 *     <li>boolean, numbers, <code>null</code>, other words: as if quoted,
 *         but escape sequences are ignored
 * </ul>
 *
 * JSON quoted strings are always converted into NUL-terminated,
 * shortest-form UTF-8 C strings.
 *
 * Truncation will occur (and @c errno will be set to @c EINVAL), if
 * processing input encounters
 * <ul><li>an invalid or overlong UTF-8 input (eg C0 80 for U+0);
 *     <li>an input sequence (including escapes) that would decode
 *         to a codepoint outside the
 *         safe codepoint set, {U+1..U+D7FF, U+E000..U+10FFFF};
 * </ul>
 *
 * If @a bufsz is zero, the function operates in "size request mode,
 * and computes the minimum buffer size needed to hold the result.
 *
 * If @a bufsz is non-zero and too small to hold the whole result, the
 * output is truncated at the nearest UTF-8 sequence boundary such that
 * a terminating NUL byte can be written.
 *
 * @param json  (optional) input JSON text
 * @param buf   storage for the returned UTF-8 string.
 *              This will always be NUL-terminated if @a bufsz > 0,
 *              regardless of whether an error occurs or not.
 * @param bufsz the size of @a buf, or 0 if only a return value is wanted
 *
 * @returns a non-zero positive number indicating
 *          the minimum buffer size required to convert the input without
 *          buffer truncation.
 *          If this is larger than @a bufsz, then the @a buf will have been
 *          truncated at a UTF-8 boundary nearest the buffer end.
 */
size_t json_as_str(const __JSON char *json, void *buf, size_t bufsz);

/**
 * Converts JSON into a UTF-8B C string.
 *
 * This conversion is the same as #json_as_str(), except that
 * invalid input sequences are not truncated, but instead
 * mapped byte-wise into U+DC00..U+DCFF.
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
 * @see #json_string_from_utf8b()
 *
 * @param json  (optional) input JSON text
 * @param buf   storage for the returned modified UTF-8 string.
 *              This will always be NUL-terminated if @a bufsz > 0,
 *              regardless of whether an error occurs or not.
 * @param bufsz the size of @a buf, or 0 if only a return value is wanted
 *
 * @returns the minimum @a bufsz required to convert the input without
 *          truncation, including terminating NUL
 * @retval 0 [ENOMEM] The buffer size is too small and non-zero.
 * @retval 0 [EINVAL] The input text is not a string.
 */
size_t json_as_utf8b(const __JSON char *json, void *buf, size_t bufsz);

/**
 * Converts JSON into a UTF-8 C string on the heap.
 *
 * This is the same conversion as #json_as_str(), except that on success
 * it returns a pointer to storage allocated by @c malloc(), and on error
 * it returns @c NULL.
 *
 * Note that JSON <code>null</code> will be converted into the
 * C string <code>"null"</code> instead of @c NULL.
 * Use #json_is_null() to test if the JSON value is <code>null</code>.
 *
 * @param json  (optional) JSON text
 *
 * @returns a newly allocated C string
 * @retval NULL [EINVAL] Conversion error, see #json_as_str().
 * @retval NULL [ENOMEM] Allocation failed.
 *
 * The caller is responsible for calling @c free() on the returned pointer.
 */
char *json_as_strdup(const __JSON char *json)
    __attribute__((malloc));

/**
 * Converts JSON into a UTF-8B C string on the heap.
 *
 * This is the same as #json_as_strdup() except that
 * invalid input sequences are byte-mapped into U+DC00..U+DCFF,
 * in the same way as #json_as_utf8b().
 *
 * @param json  (optional) JSON text
 *
 * @returns a newly allocated C string
 * @retval NULL [EINVAL] The input text is not a string.
 * @retval NULL [ENOMEM] Allocation failed.
 *
 * The caller is responsible for calling @c free() on the returned pointer.
 */
char *json_as_utf8b_strdup(const __JSON char *json)
    __attribute__((malloc));

/**
 * Decodes BASE-64 JSON string into an array of bytes.
 *
 * This function recovers binary data from a JSON string containing BASE-64.
 * Whitespace within the BASE-64 input is ignored.
 *
 * The minimum output buffer size can be obtained by calling this function
 * with a zero-sized output buffer size.
 *
 * @see RFC 3548
 *
 * @param json  (optional) input JSON text
 * @param buf   (optional) output storage for the returned binary data
 * @param bufsz the size of the output buffer @a buf, or
 *              0 to request a minimum-size calculation
 *
 * @returns the number of bytes successfuly written to @a buf, or
 *          the minimum output buffer size needed when @a bufsz is 0
 * @retval -1 [ENOMEM] The buffer size is too small.
 * @retval -1 [EINVAL] The JSON text is not a valid BASE-64 JSON string.
 */
int json_as_bytes(const __JSON char *json, void *buf, size_t bufsz);

/**
 * Decode an RFC 3339 string as a time_t.
 *
 * @see RFC 3339, ISO 8601.
 *
 * On error, this function returns -1 and sets #errno to a non-zero
 * value.
 *
 * Leap seconds do not have a representation in #time_t so they are
 * decayed to the closest second. This can be detected if necessary
 * by clearing #errno beforehand, then checking it for EOVERFLOW
 * after the call returns. For many purposes the decay can be ignored,
 * especially if you know that the input time originated from a posix
 * system.
 *
 * @param json  (optional) input JSON text
 *
 * @returns the unix time representing the string.
 * @retval -1 [EINVAL] The value was not decodable as ISO 8601.
 * @retval -1 [errno=0] The value was "1969-12-31T23:59:59Z".
 * @retval [EOVERFLOW] The value was a leap second, and
 *           was decayed to the prior non-leap second.
 */
time_t json_as_time(const __JSON char *json);

/**
 * Encode a time in RFC 3339 (ISO 8601) format.
 *
 * The output string will be in uppercase and use Z (UTC) offset.
 * For example: "1985-04-12T23:20:50Z"
 *
 * @param t     the number of seconds since the Unix epoch
 * @param dst   output buffer for holding double-quoted JSON string.
 *              This will be NUL-terminated on success.
 * @param dstsz size of the output buffer @ dst.
 *              This must be at least @c JSON_FROM_TIME_SZ.
 *
 * @returns the number of non-NUL bytes that were stored in @a dst
 * @retval -1 [ENOMEM] The @a dstsz is too small.
 * @retval -1 [EOVERFLOW] The time cannot be represented.
 */
size_t json_from_time(time_t t, __JSON char *dst, size_t dstsz);

/* Minimum buffer size required by json_from_time(). */
#define JSON_FROM_TIME_SZ (sizeof "\"1985-04-12T23:20:50Z\"")

/**
 * Encodes binary data into a BASE-64, quoted JSON string.
 *
 * Use #JSON_FROM_BYTES_DSTSZ() to determine the dst buffer size required.
 * If the dst buffer is too small, it will be truncated with a single NUL
 * (unless dstsz is zero) and an error will be returned.
 *
 * @see RFC 3548
 *
 * @param src   source bytes
 * @param srcsz length of source in bytes
 * @param dst   output buffer for holding double-quoted JSON string.
 *              This will be NUL-terminated on success.
 * @param dstsz size of the output buffer @ dst.
 *              This must be at least @c JSON_FROM_BYTES_DSTSZ(srcsz).
 *
 * @returns the number of non-NUL bytes that were stored in @a dst
 * @retval -1 [ENOMEM] The @a dstsz is too small.
 */
int json_from_bytes(const void *src, size_t srcsz,
			      __JSON char *dst, size_t dstsz);

/** Calculates the output buffer size for #json_from_bytes().
 *  @param srcsz length of the source in bytes
 *  @returns size of a buffer required to hold a NUL-terminated
 *           JSON string containing the BASE-64 encoding of the
 *           source bytes.
 */
#define JSON_FROM_BYTES_DSTSZ(srcsz)   (3 + (((srcsz) + 2) / 3) * 4)

/**
 * Converts a UTF-8 C string into a quoted JSON string.
 *
 * If the source string contains invalid UTF-8 characters, this
 * function will return 0 and set @c errno to @c EINVAL.
 *
 * If the @a dstsz is 0, then the minimum output buffer size
 * will be computed and returned.
 *
 * If there is insufficient space in the output buffer, this function
 * will place an empty string in the output, and return 0 [ENOMEM].
 *
 * Substrings "</" and "]]>" will have their last character escaped
 * so that they never appear literally in the output buffer.
 *
 * @param src   (optional) NUL-terminated source string in UTF-8
 * @param dst   output buffer, will be NUL terminated.
 * @param dstsz output buffer size, or
 *              0 to indicate a size request
 *
 * @returns the minimum @a dstsz required (@a dstsz was 0), or
 *          the number of bytes stored in @a dst including the NUL.
 * @retval 0 [EINVAL] The source string is @c NULL.
 * @retval 0 [EINVAL] The source string contains invalid UTF-8.
 * @retval 0 [ENOMEM] The output buffer size @a dstz was too small and non-zero.
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
 * @returns the minimum number of bytes of output buffer required, or
 *         the number of bytes stored in @a dst including the NUL.
 * @retval 0 [EINVAL] The source string contains invalid UTF-8.
 * @retval 0 [ENOMEM] The output buffer is too small.
 */
size_t json_string_from_strn(const char *src, int srclen,
				__JSON char *dst, size_t dstsz);

/**
 * Converts from a UTF-8B C string into a quoted JSON string value.
 *
 * A UTF-8B string is one that contains codepoints
 * U+DC00..U+DCFF. This function unwraps these points
 * back into individual bytes.
 *
 * This function is otherwise similar to #json_string_from_str().
 *
 * @param src   (optional) NUL-terminated source string
 * @param dst   output buffer, will be NUL terminated.
 * @param dstsz output buffer size, or 0 to indicate a size request
 *
 * @returns the minimum size of the buffer filled or required
 * @retval 0 [EINVAL] The source string contains invalid UTF-8
 *                    characters, other than U+DC00..U+DCFF
 * @retval 0 [EINVAL] The source string is @c NULL
 * @retval 0 [ENOMEM] The output buffer is too small.
 */
size_t json_string_from_utf8b(const char *src,
				__JSON char *dst, size_t dstsz);

/**
 * @see #json_string_from_utf8b().
 *
 * @param src    source string
 * @param srclen number of bytes in the source string
 * @param dst    output buffer, will be NUL terminated.
 * @param dstsz  output buffer size, or 0 to indicate a size request
 *
 * @returns the minimum size of the buffer filled or required
 * @retval 0 [EINVAL] The source string contains invalid UTF-8
 *                    characters, other than U+DC00..U+DCFF
 * @retval 0 [ENOMEM] The output buffer is too small.
 */
size_t json_string_from_utf8bn(const char *src, int srclen,
				__JSON char *dst, size_t dstsz);

/**
 * Compares a JSON value with a UTF-8 C string.
 *
 * This function is equivalent to converting the first argument with
 * #json_as_utf8b(), and then comparing that to the second argument
 * with @c strcmp().
 *
 * Note that UTF-8B comparison with @c strcmp() is equivalent to a
 * per-unicode-character comparison thanks to a nifty property of UTF-8.
 *
 * <ul>
 * <li>JSON numbers, booleans and null will comared equal to
 *     C strings containing the same JSON representation. e.g.
 *     The JSON value <code>null</code> will compare equal to
 *     the C string <code>"null"</code>, and
 *     the JSON number <code>0.50</code> will compare equal to
 *     the C string <code>"0.50"</code>.
 * <li>Objects, arrays and invalid JSON will only compare equal to
 *     the C string <code>""</code>.
 * </ul>
 *
 * @param json (optional) JSON text
 * @param cstr (required) UTF-8 C string to compare against
 *
 * This function sets @c errno to @c EINVAL if the JSON input is
 * not a quoted JSON string.
 *
 * @retval  0 The JSON value compares equal to the C string.
 * @retval -1 The JSON string is lexicographically 'smaller' than
 *            the C string
 * @retval +1 The JSON string is lexicographically 'larger' than the
 *            C string.
 */
int json_strcmp(const __JSON char *json, const char *cstr);

/**
 * @see #json_strcmp()
 *
 * @param json   (optional) JSON text
 * @param cstr   UTF-8 string segment to compare against
 * @param cstrsz length of the string segment, in bytes
 *
 * @retval  0 The JSON value compares equal to the C string.
 * @retval -1 The JSON string is lexicographically 'smaller' than
 *            the C string
 * @retval +1 The JSON string is lexicographically 'larger' than the
 *            C string.
 */
int json_strcmpn(const __JSON char *json, const char *cstr, size_t cstrsz);

extern const char json_true[];	/**< "true" */
extern const char json_false[];	/**< "false" */
extern const char json_null[];	/**< "null" */

/** Converts a boolean into string constants "true" or "false". */
#define json_from_bool(b) ((b) ? json_true : json_false)

#endif /* REDJSON_H */
