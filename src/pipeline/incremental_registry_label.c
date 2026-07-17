#include "pipeline/incremental_registry_label.h"

#include <string.h>

#if defined(CBM_USE_RUST_PIPELINE_INCREMENTAL_REGISTRY_LABEL)
extern int cbm_rs_pipeline_incremental_label_is_registry_symbol_v1(const char *label);
#endif

bool cbm_pipeline_incremental_label_is_registry_symbol(const char *label) {
#if defined(CBM_USE_RUST_PIPELINE_INCREMENTAL_REGISTRY_LABEL)
    return cbm_rs_pipeline_incremental_label_is_registry_symbol_v1(label) != 0;
#else
    /* 與 Rust pipeline_incremental::label_is_registry_symbol 及
     * cbm_label_is_type_like + Function/Method/Variable/Field 等價。 */
    return label && (strcmp(label, "Function") == 0 || strcmp(label, "Method") == 0 ||
                     strcmp(label, "Class") == 0 || strcmp(label, "Struct") == 0 ||
                     strcmp(label, "Interface") == 0 || strcmp(label, "Enum") == 0 ||
                     strcmp(label, "Type") == 0 || strcmp(label, "Trait") == 0 ||
                     strcmp(label, "Variable") == 0 || strcmp(label, "Field") == 0);
#endif
}
