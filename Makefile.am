
ACLOCAL_AMFLAGS = -I m4

include_HEADERS = json.h

lib_LTLIBRARIES = libredjson.la
libredjson_la_SOURCES  =
libredjson_la_SOURCES += libjson/array.c
libredjson_la_SOURCES += libjson/base64.c
libredjson_la_SOURCES += libjson/bool.c
libredjson_la_SOURCES += libjson/null.c
libredjson_la_SOURCES += libjson/number.c
libredjson_la_SOURCES += libjson/object.c
libredjson_la_SOURCES += libjson/select.c
libredjson_la_SOURCES += libjson/skip.c
libredjson_la_SOURCES += libjson/span.c
libredjson_la_SOURCES += libjson/string.c
libredjson_la_SOURCES += libjson/type.c
libredjson_la_SOURCES += libjson/utf8.c
libredjson_la_SOURCES += libjson/word.c

check_PROGRAMS =
check_PROGRAMS += libjson/t-array
check_PROGRAMS += libjson/t-base64
check_PROGRAMS += libjson/t-bool
check_PROGRAMS += libjson/t-null
check_PROGRAMS += libjson/t-number
check_PROGRAMS += libjson/t-object
check_PROGRAMS += libjson/t-select
check_PROGRAMS += libjson/t-span
check_PROGRAMS += libjson/t-str-as
check_PROGRAMS += libjson/t-str-from
check_PROGRAMS += libjson/t-strcmp
check_PROGRAMS += libjson/t-type
check_PROGRAMS += libjson/t-word
libjson_t_array_LDADD	= libredjson.la
libjson_t_base64_LDADD	= libredjson.la
libjson_t_bool_LDADD	= libredjson.la
libjson_t_null_LDADD	= libredjson.la
libjson_t_number_LDADD	= libredjson.la
libjson_t_object_LDADD	= libredjson.la
libjson_t_select_LDADD	= libredjson.la
libjson_t_span_LDADD	= libredjson.la
libjson_t_str_as_LDADD	= libredjson.la
libjson_t_str_from_LDADD= libredjson.la
libjson_t_strcmp_LDADD	= libredjson.la
libjson_t_type_LDADD	= libredjson.la
libjson_t_word_LDADD	= libredjson.la

TESTS = $(check_PROGRAMS)

