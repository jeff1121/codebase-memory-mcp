/*
 * userconfig_bridge.c — 為 Rust language direct slice 提供可選的 userconfig 邊界。
 */
#include "discover/userconfig.h"

int cbm_rs_user_language_lookup(const char* ext) {
    if (!ext) {
        return CBM_LANG_COUNT;
    }
    const cbm_userconfig_t* cfg = cbm_get_user_lang_config();
    if (!cfg) {
        return CBM_LANG_COUNT;
    }
    return cbm_userconfig_lookup(cfg, ext);
}
