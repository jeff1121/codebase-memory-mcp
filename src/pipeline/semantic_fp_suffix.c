#include "pipeline/semantic_fp_suffix.h"

#include <string.h>

#if defined(CBM_USE_RUST_PIPELINE_SEMANTIC_FP_SUFFIX)
extern int cbm_rs_pipeline_semantic_fp_ends_with_v1(const char *file_path, const char *suffix);
#endif

bool cbm_pipeline_semantic_fp_ends_with(const char *file_path, const char *suffix) {
#if defined(CBM_USE_RUST_PIPELINE_SEMANTIC_FP_SUFFIX)
    return cbm_rs_pipeline_semantic_fp_ends_with_v1(file_path, suffix) != 0;
#else
    if (!file_path || !suffix) {
        return false;
    }
    size_t fplen = strlen(file_path);
    size_t sflen = strlen(suffix);
    return fplen >= sflen && strcmp(file_path + fplen - sflen, suffix) == 0;
#endif
}
