#include "foundation/log.h"

void cbm_rs_path_alias_warn(const char* message, const char* key, const char* value) {
    cbm_log_warn(message, key, value, "kept", "256_of_more");
}
