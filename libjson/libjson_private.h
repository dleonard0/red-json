#ifndef H_LIBJSON_JSON
#define H_LIBJSON_JSON

#include "json.h"

/* Marks as function as exported from the library */
#define __PUBLIC __attribute__ ((visibility ("default")))
#define __PURE   __attribute__ ((pure))

int is_delimiter(__JSON char ch) __PURE;
int is_word_start(__JSON char ch) __PURE;
int is_word_char(__JSON char ch) __PURE;

void skip_white(const __JSON char **json_ptr);
int can_skip_delim(const __JSON char **json_ptr, char ch);
int skip_word_or_string(const __JSON char **json_ptr);
int skip_value(const __JSON char **json_ptr);

int word_strcmpn(const __JSON char *json, const char *str, size_t strsz);
int word_strcmp(const __JSON char *json, const char *str);

#endif /* H_LIBJSON_JSON */
