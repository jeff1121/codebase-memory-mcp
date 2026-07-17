#ifndef CBM_PIPELINE_INCREMENTAL_REGISTRY_LABEL_H
#define CBM_PIPELINE_INCREMENTAL_REGISTRY_LABEL_H

#include <stdbool.h>

/* 公開 bridge：incremental registry seed 用 label 過濾。
 * NULL → false。接受 Function/Method、type-like、Variable/Field。 */
bool cbm_pipeline_incremental_label_is_registry_symbol(const char *label);

#endif
