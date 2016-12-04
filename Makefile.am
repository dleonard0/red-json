
include_HEADERS = json.h

lib_LIBRARIES = libjson.a
libjson_a_SOURCES  =
libjson_a_SOURCES += libjson/array.c
libjson_a_SOURCES += libjson/bool.c
libjson_a_SOURCES += libjson/null.c
libjson_a_SOURCES += libjson/number.c
libjson_a_SOURCES += libjson/object.c
libjson_a_SOURCES += libjson/select.c
libjson_a_SOURCES += libjson/skip.c
libjson_a_SOURCES += libjson/span.c
libjson_a_SOURCES += libjson/string.c
libjson_a_SOURCES += libjson/type.c
libjson_a_SOURCES += libjson/utf8.c
libjson_a_SOURCES += libjson/word.c

check_PROGRAMS =
check_PROGRAMS += libjson/t-array
check_PROGRAMS += libjson/t-null
check_PROGRAMS += libjson/t-number
check_PROGRAMS += libjson/t-object
check_PROGRAMS += libjson/t-select
check_PROGRAMS += libjson/t-span
check_PROGRAMS += libjson/t-str
check_PROGRAMS += libjson/t-strcmp
check_PROGRAMS += libjson/t-type
libjson_t_array_LDADD	= libjson.a
libjson_t_null_LDADD	= libjson.a
libjson_t_number_LDADD	= libjson.a
libjson_t_object_LDADD	= libjson.a
libjson_t_select_LDADD	= libjson.a
libjson_t_span_LDADD	= libjson.a
libjson_t_str_LDADD	= libjson.a
libjson_t_strcmp_LDADD	= libjson.a
libjson_t_type_LDADD	= libjson.a

TESTS = $(check_PROGRAMS)

