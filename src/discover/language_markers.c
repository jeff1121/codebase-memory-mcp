#include "discover/language_markers.h"

#include <string.h>

#ifdef CBM_USE_RUST_DISCOVER_LANGUAGE_MARKERS
extern int cbm_rs_discover_language_marker_v1(const char *buffer, int kind);
#else

enum { LANGUAGE_MARKER_SCAN_PASSES = 2 };

static int str_contains(const char *haystack, const char *needle) {
    return strstr(haystack, needle) != NULL;
}

static int is_ascii_alpha(unsigned char value) {
    return (value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z');
}

static int has_objc_markers(const char *buffer) {
    return str_contains(buffer, "@interface") || str_contains(buffer, "@implementation") ||
           str_contains(buffer, "@protocol") || str_contains(buffer, "@property") ||
           str_contains(buffer, "#import") || str_contains(buffer, "@selector") ||
           str_contains(buffer, "@encode") || str_contains(buffer, "@synthesize") ||
           str_contains(buffer, "@dynamic");
}

static int has_magma_end_markers(const char *buffer) {
    return str_contains(buffer, "end function;") || str_contains(buffer, "end procedure;") ||
           str_contains(buffer, "end intrinsic;") || str_contains(buffer, "end if;") ||
           str_contains(buffer, "end for;") || str_contains(buffer, "end while;");
}

static int has_magma_callable_pattern(const char *buffer) {
    const char *markers[] = {"intrinsic ", "procedure "};
    for (int index = 0; index < LANGUAGE_MARKER_SCAN_PASSES; index++) {
        const char *cursor = strstr(buffer, markers[index]);
        if (!cursor) {
            continue;
        }
        cursor += strlen(markers[index]);
        const char *name_start = cursor;
        while (*cursor && is_ascii_alpha((unsigned char)*cursor)) {
            cursor++;
        }
        if (cursor > name_start && *cursor == '(') {
            return 1;
        }
    }
    return 0;
}

static int has_matlab_line_markers(const char *buffer) {
    const char *line = buffer;
    while (*line) {
        const char *cursor = line;
        while (*cursor == ' ' || *cursor == '\t') {
            cursor++;
        }
        if (strncmp(cursor, "function ", sizeof("function ") - 1) == 0 ||
            strncmp(cursor, "function\t", sizeof("function\t") - 1) == 0 ||
            strncmp(cursor, "classdef ", sizeof("classdef ") - 1) == 0 ||
            strncmp(cursor, "classdef\t", sizeof("classdef\t") - 1) == 0 ||
            strncmp(cursor, "%%", 2) == 0 || (*cursor == '%' && cursor[1] != '{')) {
            return 1;
        }
        const char *newline = strchr(line, '\n');
        if (!newline) {
            break;
        }
        line = newline + 1;
    }
    return 0;
}

#endif

int cbm_discover_language_marker(const char *buffer, int kind) {
#ifdef CBM_USE_RUST_DISCOVER_LANGUAGE_MARKERS
    return cbm_rs_discover_language_marker_v1(buffer, kind);
#else
    if (!buffer) {
        return 0;
    }
    switch (kind) {
    case 0:
        return has_objc_markers(buffer);
    case 1:
        return has_magma_end_markers(buffer);
    case 2:
        return has_magma_callable_pattern(buffer);
    case 3:
        return has_matlab_line_markers(buffer);
    default:
        return 0;
    }
#endif
}
