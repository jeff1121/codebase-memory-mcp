#include "foundation/constants.h"
#include "pipeline/artifact.h"
#include "pipeline/artifact_path.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef CBM_USE_RUST_PIPELINE_ARTIFACT_PATH
extern size_t cbm_rs_pipeline_artifact_path_v1(char *buf, size_t bufsz, const char *repo_path,
                                               const char *name);
#endif

bool cbm_pipeline_artifact_path(char *buf, size_t bufsz, const char *repo_path, const char *name) {
    if (!buf || bufsz == 0 || !repo_path || !name) {
        return false;
    }
#ifdef CBM_USE_RUST_PIPELINE_ARTIFACT_PATH
    return cbm_rs_pipeline_artifact_path_v1(buf, bufsz, repo_path, name) != SIZE_MAX;
#else
    int n = snprintf(buf, bufsz, "%s/%s/%s", repo_path, CBM_ARTIFACT_DIR, name);
    return n >= 0 && (size_t)n < bufsz;
#endif
}
