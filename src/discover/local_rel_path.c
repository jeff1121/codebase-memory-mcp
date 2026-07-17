#include "discover/local_rel_path.h"

#include <string.h>

size_t cbm_discover_local_rel_path_offset(const char *rel_path, const char *local_prefix) {
    if (!rel_path || !local_prefix || local_prefix[0] == '\0') {
        return 0;
    }

    size_t prefix_len = strlen(local_prefix);
    if (strncmp(rel_path, local_prefix, prefix_len) == 0 && rel_path[prefix_len] == '/') {
        return prefix_len + 1;
    }
    return 0;
}
