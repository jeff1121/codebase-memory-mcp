#ifndef CBM_PIPELINE_LSP_CROSS_CLASSIFIERS_H
#define CBM_PIPELINE_LSP_CROSS_CLASSIFIERS_H

#include "cbm.h"

#include <stdbool.h>

_Static_assert(sizeof(CBMLanguage) == sizeof(int), "CBMLanguage 必須與 Rust c_int 的 ABI 寬度一致");

/* 偵測 TypeScript 方言旗標。三個 output 指標必須非 NULL。 */
void cbm_pxc_ts_modes(CBMLanguage lang, const char *rel_path, bool *out_js, bool *out_jsx,
                      bool *out_dts);

/* 回傳此語言是否具備 pipeline cross-LSP eligibility。 */
bool cbm_pxc_has_cross_lsp(CBMLanguage lang);

/* 允許時回傳原始 borrowed label 指標；NULL 或拒絕時回傳 NULL。 */
const char *cbm_pxc_map_label(const char *label);

#endif /* CBM_PIPELINE_LSP_CROSS_CLASSIFIERS_H */
