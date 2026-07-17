/*
 * language_name.h - CBMLanguage 顯示名稱介面。
 */
#ifndef CBM_DISCOVER_LANGUAGE_NAME_H
#define CBM_DISCOVER_LANGUAGE_NAME_H

#include "cbm.h"

/* 取得語言 enum 的人類可讀名稱。
 *
 * 回傳值永不為 NULL，且指向 process lifetime 內有效、NUL 結尾且唯讀的 static 字串。
 * 呼叫端不得釋放或修改該字串；不保證不同呼叫的指標位址相同。
 * CBM_LANG_COUNT、負值、超出範圍或沒有名稱的 enum 值均回傳 "Unknown"。 */
const char *cbm_language_name(CBMLanguage lang);

#endif
