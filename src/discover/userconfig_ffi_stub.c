/*
 * userconfig_ffi_stub.c — userconfig Rust FFI smoke 專用依賴 stub。
 *
 * 正式產品連結使用 platform.c 與 language.c；本檔只提供最小 ABI，
 * 讓獨立 test-rust-ffi 不必連結整個 discover 模組。
 */
#include "cbm.h"

const char *cbm_language_name(int language) {
    return language == CBM_LANG_PHP ? "PHP" : NULL;
}
