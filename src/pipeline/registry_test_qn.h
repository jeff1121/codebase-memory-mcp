#ifndef CBM_PIPELINE_REGISTRY_TEST_QN_H
#define CBM_PIPELINE_REGISTRY_TEST_QN_H

#include <stdbool.h>

/* NULL 回 false；非 NULL 必須是有效 NUL 結尾的 qualified name。 */
bool cbm_pipeline_registry_is_test_qn(const char *qn);

#endif
