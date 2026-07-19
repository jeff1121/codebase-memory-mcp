#ifndef CBM_PIPELINE_ENRICHMENT_TOKENS_H
#define CBM_PIPELINE_ENRICHMENT_TOKENS_H

/* 將 camelCase 依 ASCII 小寫→大寫邊界切為 token。
 *
 * s == NULL、out == NULL 或 max_out <= 0 時回傳 0 且不寫入 out。
 * 成功 token 由 C malloc 配置，caller 必須逐一 free()。輸入採 first-NUL raw bytes 語意。
 * 正常資源條件以外（例如 OOM）不屬 C/Rust parity 契約。 */
int cbm_split_camel_case(const char *s, char **out, int max_out);

/* 將 decorator 轉為 ASCII lowercase token，並排除既有 stopword。
 *
 * s == NULL、out == NULL 或 max_out <= 0 時回傳 0 且不寫入 out。
 * decorator 最多處理前 255 bytes；成功 token 由 C malloc 配置，caller 必須逐一 free()。
 * 非 ASCII bytes 維持原樣，輸入採 first-NUL raw bytes 語意。 */
int cbm_tokenize_decorator(const char *dec, char **out, int max_out);

#endif
