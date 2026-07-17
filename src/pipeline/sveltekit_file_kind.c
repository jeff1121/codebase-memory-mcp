#include "pipeline/sveltekit_file_kind.h"

#include <string.h>

#if defined(CBM_USE_RUST_PIPELINE_SVELTEKIT_FILE_KIND)
extern int cbm_rs_pipeline_sveltekit_file_kind_v1(const char *file_path);
#endif

int cbm_pipeline_sveltekit_file_kind(const char *file_path) {
#if defined(CBM_USE_RUST_PIPELINE_SVELTEKIT_FILE_KIND)
    return cbm_rs_pipeline_sveltekit_file_kind_v1(file_path);
#else
    if (!file_path) {
        return 0;
    }
    if (!strstr(file_path, "/routes/")) {
        return 0;
    }
    const char *slash = strrchr(file_path, '/');
    const char *base = slash ? slash + 1 : file_path;
    if (strcmp(base, "+server.ts") == 0 || strcmp(base, "+server.js") == 0) {
        return 1;
    }
    if (strcmp(base, "+page.server.ts") == 0 || strcmp(base, "+page.server.js") == 0) {
        return 2;
    }
    if (strcmp(base, "+layout.server.ts") == 0 || strcmp(base, "+layout.server.js") == 0) {
        return 3;
    }
    return 0;
#endif
}
