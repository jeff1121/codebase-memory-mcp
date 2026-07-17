#include "pipeline/incremental_edge_type.h"

#include <string.h>

#if defined(CBM_USE_RUST_PIPELINE_INCREMENTAL_EDGE)
extern int cbm_rs_pipeline_incremental_edge_type_is_recomputed_v1(const char *edge_type);
#endif

bool cbm_pipeline_incremental_edge_type_is_recomputed(const char *edge_type) {
#if defined(CBM_USE_RUST_PIPELINE_INCREMENTAL_EDGE)
    return cbm_rs_pipeline_incremental_edge_type_is_recomputed_v1(edge_type) != 0;
#else
    return edge_type &&
           (strcmp(edge_type, "SIMILAR_TO") == 0 ||
            strcmp(edge_type, "SEMANTICALLY_RELATED") == 0 ||
            strcmp(edge_type, "FILE_CHANGES_WITH") == 0 || strcmp(edge_type, "DATA_FLOWS") == 0);
#endif
}
