#include "discover/discover_string_helpers.h"

#include <string.h>

#if defined(CBM_USE_RUST_DISCOVER_STRING_HELPERS)
extern int cbm_rs_discover_str_in_list_v1(const char *value, const char *const *list);
extern int cbm_rs_discover_ends_with_v1(const char *value, const char *suffix);
extern int cbm_rs_discover_str_contains_v1(const char *value, const char *substring);
extern int cbm_rs_discover_ascii_ieq_v1(const char *left, const char *right);
#else
static unsigned char ascii_lower(unsigned char value) {
    if (value >= 'A' && value <= 'Z') {
        return value + ('a' - 'A');
    }
    return value;
}
#endif

bool cbm_discover_str_in_list(const char *value, const char *const *list) {
#if defined(CBM_USE_RUST_DISCOVER_STRING_HELPERS)
    return cbm_rs_discover_str_in_list_v1(value, list) != 0;
#else
    if (!value || !list) {
        return false;
    }
    for (int i = 0; list[i]; i++) {
        if (strcmp(value, list[i]) == 0) {
            return true;
        }
    }
    return false;
#endif
}

bool cbm_discover_ends_with(const char *value, const char *suffix) {
#if defined(CBM_USE_RUST_DISCOVER_STRING_HELPERS)
    return cbm_rs_discover_ends_with_v1(value, suffix) != 0;
#else
    if (!value || !suffix) {
        return false;
    }
    size_t value_len = strlen(value);
    size_t suffix_len = strlen(suffix);
    if (suffix_len > value_len) {
        return false;
    }
    return strcmp(value + value_len - suffix_len, suffix) == 0;
#endif
}

bool cbm_discover_str_contains(const char *value, const char *substring) {
#if defined(CBM_USE_RUST_DISCOVER_STRING_HELPERS)
    return cbm_rs_discover_str_contains_v1(value, substring) != 0;
#else
    return value && substring && strstr(value, substring) != NULL;
#endif
}

bool cbm_discover_ascii_ieq(const char *left, const char *right) {
#if defined(CBM_USE_RUST_DISCOVER_STRING_HELPERS)
    return cbm_rs_discover_ascii_ieq_v1(left, right) != 0;
#else
    if (!left || !right) {
        return false;
    }
    while (*left && *right) {
        if (ascii_lower((unsigned char)*left) != ascii_lower((unsigned char)*right)) {
            return false;
        }
        left++;
        right++;
    }
    return *left == '\0' && *right == '\0';
#endif
}
