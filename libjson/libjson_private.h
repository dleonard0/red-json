#ifndef H_LIBJSON_JSON
#define H_LIBJSON_JSON

#include "json.h"

/* Marks as function as exported from the library */
#define __PUBLIC /* indicates the function is part of public API */
#define __PURE   __attribute__ ((pure))

/* Hide some functions private to the library */
#define is_delimiter		__libjson_is_delimiter
#define is_word_start		__libjson_is_word_start
#define is_word_char		__libjson_is_word_char
#define skip_white		__libjson_skip_white
#define can_skip_delim		__libjson_can_skip_delim
#define skip_value		__libjson_skip_value
#define word_strcmpn		__libjson_word_strcmpn
#define word_strcmp		__libjson_word_strcmp

int is_delimiter(__JSON char ch) __PURE;
int is_word_start(__JSON char ch) __PURE;
int is_word_char(__JSON char ch) __PURE;

void skip_white(const __JSON char **json_ptr);
int can_skip_delim(const __JSON char **json_ptr, char ch);
int skip_value(const __JSON char **json_ptr);

int word_strcmpn(const __JSON char *json, const char *str, size_t strsz);
int word_strcmp(const __JSON char *json, const char *str);

#endif /* H_LIBJSON_JSON */
