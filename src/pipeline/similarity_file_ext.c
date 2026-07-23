#include "pipeline/similarity_file_ext.h"

#include <string.h>

#ifndef CBM_USE_RUST_PIPELINE_SIMILARITY_FILE_EXT_ONLY
static const char empty_file_ext[] = "";

static const char *similarity_file_ext_c(const char *path) {
    if (!path) {
        return empty_file_ext;
    }

    const char *dot = strrchr(path, '.');
    return dot ? dot : empty_file_ext;
}

const char *cbm_pipeline_similarity_file_ext(const char *path) {
#ifdef CBM_USE_RUST_PIPELINE_SIMILARITY_FILE_EXT
    const char *result = cbm_rs_pipeline_similarity_file_ext_v1(path);
    if (result) {
        return result;
    }
#endif
    return similarity_file_ext_c(path);
}
#endif
