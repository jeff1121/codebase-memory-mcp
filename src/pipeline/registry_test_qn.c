#include "pipeline/registry_test_qn.h"

#include <string.h>

#ifdef CBM_USE_RUST_PIPELINE_REGISTRY_TEST_QN
extern int cbm_rs_registry_is_test_qn_v1(const char *qn);
#endif

bool cbm_pipeline_registry_is_test_qn(const char *qn) {
    if (!qn) {
        return false;
    }

#ifdef CBM_USE_RUST_PIPELINE_REGISTRY_TEST_QN
    int rust_value = cbm_rs_registry_is_test_qn_v1(qn);
    if (rust_value == 0 || rust_value == 1) {
        return rust_value != 0;
    }
#endif

    return (strstr(qn, "Test") != NULL || strstr(qn, "test") != NULL ||
            strstr(qn, "Mock") != NULL || strstr(qn, "mock") != NULL ||
            strstr(qn, "Stub") != NULL || strstr(qn, "stub") != NULL ||
            strstr(qn, "Fake") != NULL || strstr(qn, "fake") != NULL ||
            strstr(qn, "Fixture") != NULL || strstr(qn, "spec") != NULL);
}
