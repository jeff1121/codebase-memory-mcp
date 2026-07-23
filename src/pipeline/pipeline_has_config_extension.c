#include "pipeline/pipeline_has_config_extension.h"

#include <stddef.h>
#include <string.h>

#if defined(CBM_USE_RUST_PIPELINE_HAS_CONFIG_EXTENSION) || \
    defined(CBM_USE_RUST_PIPELINE_HAS_CONFIG_EXTENSION_ONLY)
extern int cbm_rs_pipeline_has_config_extension_v1(const char *path);
#endif

bool cbm_has_config_extension(const char *path) {
#if defined(CBM_USE_RUST_PIPELINE_HAS_CONFIG_EXTENSION) || \
    defined(CBM_USE_RUST_PIPELINE_HAS_CONFIG_EXTENSION_ONLY)
    return cbm_rs_pipeline_has_config_extension_v1(path) != 0;
#else
    if (!path) {
        return false;
    }

    const char *dot = strrchr(path, '.');
    const char *basename = strrchr(path, '/');
    if (!basename) {
        basename = path;
    } else {
        basename++;
    }

    if (strcmp(basename, ".env") == 0) {
        return true;
    }

    if (!dot) {
        return false;
    }

    static const char *exts[] = {".toml", ".ini", ".yaml", ".yml", ".cfg", ".properties",
                                 ".json", ".xml", ".conf", ".env", NULL};

    for (int i = 0; exts[i]; i++) {
        if (strcmp(dot, exts[i]) == 0) {
            return true;
        }
    }

    return false;
#endif
}
