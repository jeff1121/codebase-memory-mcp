#include "pipeline/configlink_path_basename.h"

#include <string.h>

#if defined(CBM_USE_RUST_PIPELINE_CONFIGLINK_PATH_BASENAME)
extern const char *cbm_rs_pipeline_configlink_path_basename_v1(const char *path);

const char *cbm_pipeline_configlink_path_basename(const char *path) {
    return cbm_rs_pipeline_configlink_path_basename_v1(path);
}
#elif !defined(CBM_USE_RUST_PIPELINE_CONFIGLINK_PATH_BASENAME_ONLY)
static const char cbm_pipeline_configlink_path_basename_empty[] = "";

const char *cbm_pipeline_configlink_path_basename(const char *path) {
    if (!path) {
        return cbm_pipeline_configlink_path_basename_empty;
    }

    const char *slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}
#endif
