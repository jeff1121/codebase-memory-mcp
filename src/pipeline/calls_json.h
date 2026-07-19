#ifndef CBM_PIPELINE_CALLS_JSON_H
#define CBM_PIPELINE_CALLS_JSON_H

/* 公開 bridge：從 properties JSON 擷取 "local_name":"..." 的 value。
 *
 * 契約：
 * - props_json == NULL、缺少鍵、空 value → 回傳 NULL
 * - 成功 → 回傳 C malloc 字串（first-NUL 結尾）；caller 必須 free()
 * - 僅 raw byte 掃描，不做 UTF-8／JSON unescape
 * - import map／CALLS edge／graph ownership 仍由 pass_calls 負責
 */
char *cbm_pipeline_calls_extract_local_name(const char *props_json);

#endif
