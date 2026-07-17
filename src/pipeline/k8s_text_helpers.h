#ifndef CBM_PIPELINE_K8S_TEXT_HELPERS_H
#define CBM_PIPELINE_K8S_TEXT_HELPERS_H

#include <stddef.h>

/* 公開 bridge（pure text helpers；無 I/O／graph）：
 * - cbm_pipeline_k8s_basename：path NULL → NULL；否則回傳 path 內 basename 指標（不配置）。
 * - cbm_pipeline_k8s_indent：line NULL → 0；僅計前導空白（tab 不算）。
 * - cbm_pipeline_k8s_split_kv：已去 indent 的 "key: value"；成功回 1，否則 0。
 */
const char *cbm_pipeline_k8s_basename(const char *path);
int cbm_pipeline_k8s_indent(const char *line);
int cbm_pipeline_k8s_split_kv(const char *text, char *key, size_t key_sz, char *val,
                              size_t val_sz);

#endif
