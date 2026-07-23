#include "pipeline/pipeline_is_env_var_name.h"

#include <ctype.h>
#include <stddef.h>
#include <string.h>

#if defined(CBM_USE_RUST_PIPELINE_IS_ENV_VAR_NAME) || \
    defined(CBM_USE_RUST_PIPELINE_IS_ENV_VAR_NAME_ONLY)
extern int cbm_rs_pipeline_is_env_var_name_v1(const char *s);
#endif

bool cbm_is_env_var_name(const char *s) {
#if defined(CBM_USE_RUST_PIPELINE_IS_ENV_VAR_NAME) || \
    defined(CBM_USE_RUST_PIPELINE_IS_ENV_VAR_NAME_ONLY)
    return cbm_rs_pipeline_is_env_var_name_v1(s) != 0;
#else
    if (!s) {
        return false;
    }
    size_t len = strlen(s);
    if (len < 2) {
        return false;
    }

    bool has_upper = false;
    for (size_t i = 0; i < len; i++) {
        char c = s[i];
        if (c >= 'A' && c <= 'Z') {
            has_upper = true;
        } else if (c == '_' || (c >= '0' && c <= '9')) {
            /* ok */
        } else {
            return false;
        }
    }
    return has_upper;
#endif
}
