#include "pipeline/lsp_cross_classifiers.h"

#include <string.h>

#if defined(CBM_USE_RUST_PIPELINE_LSP_CLASSIFIERS)
extern int cbm_rs_pipeline_lsp_ts_modes_v1(int language, const char *rel_path);
extern int cbm_rs_pipeline_lsp_has_cross_lsp_v1(int language);
extern int cbm_rs_pipeline_lsp_map_label_allowed_v1(const char *label);
#endif

void cbm_pxc_ts_modes(CBMLanguage lang, const char *rel_path, bool *out_js, bool *out_jsx,
                      bool *out_dts) {
#if defined(CBM_USE_RUST_PIPELINE_LSP_CLASSIFIERS)
    int modes = cbm_rs_pipeline_lsp_ts_modes_v1((int)lang, rel_path);
    *out_js = (modes & 1) != 0;
    *out_jsx = (modes & 2) != 0;
    *out_dts = (modes & 4) != 0;
#else
    *out_js = (lang == CBM_LANG_JAVASCRIPT);
    *out_jsx = (lang == CBM_LANG_TSX);
    *out_dts = false;
    if (!rel_path)
        return;
    size_t rl = strlen(rel_path);
    if (lang == CBM_LANG_JAVASCRIPT && rl >= 4 && strcmp(rel_path + rl - 4, ".jsx") == 0) {
        *out_jsx = true;
    }
    if (lang == CBM_LANG_TYPESCRIPT && rl >= 5 && strcmp(rel_path + rl - 5, ".d.ts") == 0) {
        *out_dts = true;
    }
#endif
}

bool cbm_pxc_has_cross_lsp(CBMLanguage lang) {
#if defined(CBM_USE_RUST_PIPELINE_LSP_CLASSIFIERS)
    return cbm_rs_pipeline_lsp_has_cross_lsp_v1((int)lang) != 0;
#else
    switch (lang) {
    case CBM_LANG_GO:
    case CBM_LANG_C:
    case CBM_LANG_CPP:
    case CBM_LANG_CUDA:
    case CBM_LANG_PYTHON:
    case CBM_LANG_JAVASCRIPT:
    case CBM_LANG_TYPESCRIPT:
    case CBM_LANG_TSX:
    case CBM_LANG_PHP:
    case CBM_LANG_CSHARP:
    case CBM_LANG_JAVA:
    case CBM_LANG_KOTLIN:
    case CBM_LANG_RUST:
        return true;
    default:
        return false;
    }
#endif
}

const char *cbm_pxc_map_label(const char *label) {
#if defined(CBM_USE_RUST_PIPELINE_LSP_CLASSIFIERS)
    return cbm_rs_pipeline_lsp_map_label_allowed_v1(label) != 0 ? label : NULL;
#else
    if (!label)
        return NULL;
    if (strcmp(label, "Class") == 0 || strcmp(label, "Struct") == 0 ||
        strcmp(label, "Interface") == 0 || strcmp(label, "Enum") == 0 ||
        strcmp(label, "Type") == 0 || strcmp(label, "Trait") == 0 ||
        strcmp(label, "Protocol") == 0 || strcmp(label, "Function") == 0 ||
        strcmp(label, "Method") == 0) {
        return label;
    }
    return NULL;
#endif
}
