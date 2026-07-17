#include <string.h>

#ifdef CBM_USE_RUST_PIPELINE_CHECKED_EXCEPTION
extern int cbm_rs_pipeline_is_checked_exception_v1(const char *name);
#endif

int cbm_pipeline_is_checked_exception(const char *name) {
    if (!name) {
        return 0;
    }
#ifdef CBM_USE_RUST_PIPELINE_CHECKED_EXCEPTION
    return cbm_rs_pipeline_is_checked_exception_v1(name);
#endif
    if (strstr(name, "Error") || strstr(name, "Panic") || strstr(name, "error") ||
        strstr(name, "panic")) {
        return 0;
    }
    return 1;
}
