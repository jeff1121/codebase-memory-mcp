#ifndef CBM_DISCOVER_STRING_HELPERS_H
#define CBM_DISCOVER_STRING_HELPERS_H

#include <stdbool.h>

bool cbm_discover_str_in_list(const char *value, const char *const *list);
bool cbm_discover_ends_with(const char *value, const char *suffix);
bool cbm_discover_str_contains(const char *value, const char *substring);
bool cbm_discover_ascii_ieq(const char *left, const char *right);

#endif
