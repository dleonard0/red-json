#ifndef LIBJSON_JSON_PRIVATE_H
#define LIBJSON_JSON_PRIVATE_H

#include "redjson.h"

/* Marks as function as exported from the library */
#define __PUBLIC /* indicates the function is part of public API */
#define __PURE   __attribute__ ((pure))

/* Hide some functions private to the library */
#define is_delimiter		_redjson_is_delimiter
#define is_word_start		_redjson_is_word_start
#define is_word_char		_redjson_is_word_char
#define skip_white		_redjson_skip_white
#define can_skip_delim		_redjson_can_skip_delim
#define skip_value		_redjson_skip_value
#define word_strcmpn		_redjson_word_strcmpn
#define word_strcmp		_redjson_word_strcmp

int is_delimiter(__JSON char ch) __PURE;
int is_word_start(__JSON char ch) __PURE;
int is_word_char(__JSON char ch) __PURE;

void skip_white(const __JSON char **json_ptr);
int can_skip_delim(const __JSON char **json_ptr, char ch);
int skip_value(const __JSON char **json_ptr);

int word_strcmpn(const __JSON char *json, const char *str, size_t strsz);
int word_strcmp(const __JSON char *json, const char *str);

#endif /* LIBJSON_JSON_PRIVATE_H */
