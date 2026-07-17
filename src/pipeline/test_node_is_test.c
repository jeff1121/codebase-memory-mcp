#include "pipeline/test_node_is_test.h"

#include <string.h>

#if defined(CBM_USE_RUST_PIPELINE_TEST_NODE_IS_TEST) || \
    defined(CBM_USE_RUST_PIPELINE_TEST_NODE_IS_TEST_ONLY)
extern int cbm_rs_pipeline_test_node_is_test_v1(const char *properties_json);
#endif

bool cbm_pipeline_test_node_is_test(const char *properties_json) {
    if (!properties_json) {
        return false;
    }

#if defined(CBM_USE_RUST_PIPELINE_TEST_NODE_IS_TEST) || \
    defined(CBM_USE_RUST_PIPELINE_TEST_NODE_IS_TEST_ONLY)
    return cbm_rs_pipeline_test_node_is_test_v1(properties_json) != 0;
#else
    return strstr(properties_json, "\"is_test\":true") != NULL;
#endif
}
