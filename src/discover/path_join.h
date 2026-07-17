#ifndef CBM_DISCOVER_PATH_JOIN_H
#define CBM_DISCOVER_PATH_JOIN_H

#include <stddef.h>

/* 公開 bridge 契約（caller-owned buffer）：
 * - out 為 NULL 或 out_sz==0：no-op，不寫入。
 * - base/rel 可為 NULL，視為空字串。
 * - base 空：複製 rel（或 ""）；rel 空：複製 base。
 * - base 尾端為 '/' 或 '\\'：直接串接；否則插入 '/'。
 * - 寫入後正規化 '\\' → '/'（並經 cbm_normalize_path_sep 的 drive 規則）。
 * - 容量不足時靜默截斷並保證 NUL 結尾。
 * API 參數順序為 (base, rel, out, out_sz)，與 stable Rust v1 / direct ABI 一致。 */
void cbm_discover_path_join(const char *base, const char *rel, char *out, size_t out_sz);

#endif
