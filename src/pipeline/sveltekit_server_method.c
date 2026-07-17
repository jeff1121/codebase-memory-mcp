#include "pipeline/sveltekit_server_method.h"

#include <string.h>

#if defined(CBM_USE_RUST_PIPELINE_SVELTEKIT_SERVER_METHOD)
extern const char *cbm_rs_pipeline_sveltekit_server_method_v1(const char *name);
#endif

const char *cbm_pipeline_sveltekit_server_method(const char *name) {
#if defined(CBM_USE_RUST_PIPELINE_SVELTEKIT_SERVER_METHOD)
    return cbm_rs_pipeline_sveltekit_server_method_v1(name);
#else
    if (!name) {
        return NULL;
    }
    static const char *const verbs[] = {
        "GET", "POST", "PUT", "PATCH", "DELETE", "OPTIONS", "HEAD",
    };
    for (size_t i = 0; i < sizeof(verbs) / sizeof(verbs[0]); i++) {
        if (strcmp(name, verbs[i]) == 0) {
            return verbs[i];
        }
    }
    if (strcmp(name, "fallback") == 0) {
        return "ANY";
    }
    return NULL;
#endif
}
