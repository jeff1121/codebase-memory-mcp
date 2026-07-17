/*
 * userconfig_bridge_stub.c — FFI smoke 專用的無狀態 userconfig fallback。
 */
#include "cbm.h"

int cbm_rs_user_language_lookup(const char* ext) {
    (void)ext;
    return CBM_LANG_COUNT;
}
