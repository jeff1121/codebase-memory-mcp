#ifndef CBM_PIPELINE_PKGMAP_TEXT_H
#define CBM_PIPELINE_PKGMAP_TEXT_H

#include <stddef.h>

/* 公開 bridge（bounded manifest scan 與 path 文字 helper）：
 * - at_prefix：source/prefix == NULL 或 prefix_len < 0 時回傳 0；比較 explicit bytes
 * - find_line_value：source/prefix == NULL 或 source_len < 0 時回傳 NULL；成功值借用 source
 * - path helper：輸入採 first-NUL raw bytes；回傳 C malloc 字串，caller 必須 free()
 * - path_dirname/strip_extension 的 NULL 輸入回傳配置的空字串；join 的 NULL/空 entry 回傳 NULL
 * - join/build 沿用既有 1023-byte path 組裝上限，並在去副檔名之前截斷
 */
int cbm_pipeline_pkgmap_at_prefix(const char *source, size_t available, const char *prefix,
                                  int prefix_len);
const char *cbm_pipeline_pkgmap_find_line_value(const char *source, int source_len,
                                                const char *prefix);
char *cbm_pipeline_pkgmap_path_dirname(const char *path);
char *cbm_pipeline_pkgmap_strip_extension(const char *path);
char *cbm_pipeline_pkgmap_join_and_strip(const char *dir, const char *entry);
char *cbm_pipeline_pkgmap_build_entry_path(const char *rel_path, const char *suffix);

#endif
