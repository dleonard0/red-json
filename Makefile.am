
ACLOCAL_AMFLAGS = -I m4

include_HEADERS = redjson.h

lib_LTLIBRARIES = libredjson.la
libredjson_la_SOURCES  =
libredjson_la_SOURCES += lib/array.c
libredjson_la_SOURCES += lib/base64.c
libredjson_la_SOURCES += lib/bool.c
libredjson_la_SOURCES += lib/null.c
libredjson_la_SOURCES += lib/number.c
libredjson_la_SOURCES += lib/object.c
libredjson_la_SOURCES += lib/select.c
libredjson_la_SOURCES += lib/skip.c
libredjson_la_SOURCES += lib/span.c
libredjson_la_SOURCES += lib/stras.c
libredjson_la_SOURCES += lib/strfrom.c
libredjson_la_SOURCES += lib/type.c
libredjson_la_SOURCES += lib/utf8.c
libredjson_la_SOURCES += lib/word.c
libredjson_la_SOURCES += lib/version.c

check_PROGRAMS =
check_PROGRAMS += lib/t-array
check_PROGRAMS += lib/t-base64
check_PROGRAMS += lib/t-bool
check_PROGRAMS += lib/t-null
check_PROGRAMS += lib/t-number
check_PROGRAMS += lib/t-object
check_PROGRAMS += lib/t-select
check_PROGRAMS += lib/t-span
check_PROGRAMS += lib/t-str-as
check_PROGRAMS += lib/t-str-from
check_PROGRAMS += lib/t-strcmp
check_PROGRAMS += lib/t-type
check_PROGRAMS += lib/t-word
lib_t_array_LDADD	= libredjson.la
lib_t_base64_LDADD	= libredjson.la
lib_t_bool_LDADD	= libredjson.la
lib_t_null_LDADD	= libredjson.la
lib_t_number_LDADD	= libredjson.la
lib_t_object_LDADD	= libredjson.la
lib_t_select_LDADD	= libredjson.la
lib_t_span_LDADD	= libredjson.la
lib_t_str_as_LDADD	= libredjson.la
lib_t_str_from_LDADD	= libredjson.la
lib_t_strcmp_LDADD	= libredjson.la
lib_t_type_LDADD	= libredjson.la
lib_t_word_LDADD	= libredjson.la

TESTS = $(check_PROGRAMS)

