#include "foundation/constants.h"

#include <string.h>

#ifdef CBM_USE_RUST_PIPELINE_K8S_FILE_CLASSIFIERS
extern int cbm_rs_pipeline_is_helm_chart_file_v1(const char *name);
extern int cbm_rs_pipeline_is_gomod_file_v1(const char *name);
extern int cbm_rs_pipeline_is_requirements_file_v1(const char *name);
#endif

int cbm_pipeline_is_helm_chart_file(const char *name) {
    if (!name) {
        return 0;
    }
#ifdef CBM_USE_RUST_PIPELINE_K8S_FILE_CLASSIFIERS
    return cbm_rs_pipeline_is_helm_chart_file_v1(name);
#else
    return strcmp(name, "Chart.yaml") == 0 || strcmp(name, "Chart.yml") == 0;
#endif
}

int cbm_pipeline_is_gomod_file(const char *name) {
    if (!name) {
        return 0;
    }
#ifdef CBM_USE_RUST_PIPELINE_K8S_FILE_CLASSIFIERS
    return cbm_rs_pipeline_is_gomod_file_v1(name);
#else
    return strcmp(name, "go.mod") == 0;
#endif
}

int cbm_pipeline_is_requirements_file(const char *name) {
    if (!name) {
        return 0;
    }
#ifdef CBM_USE_RUST_PIPELINE_K8S_FILE_CLASSIFIERS
    return cbm_rs_pipeline_is_requirements_file_v1(name);
#else
    return strcmp(name, "requirements.txt") == 0;
#endif
}
