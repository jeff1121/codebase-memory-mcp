#ifndef CBM_DISCOVER_TRAILING_SEP_H
#define CBM_DISCOVER_TRAILING_SEP_H

#include <stdbool.h>

/* value 可為 NULL 或 NUL 結尾字串。
 * 公開 bridge 契約：NULL 與空字串回 false；尾端 '/' 或 '\\' 回 true。
 * 這是將原 private helper 公開化時凍結的防禦性契約，不是舊 C static
 * helper 對 NULL 呼叫 strlen 的未定義行為等價宣稱。 */
bool cbm_discover_has_trailing_sep(const char *value);

#endif
