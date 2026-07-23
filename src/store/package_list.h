#ifndef CBM_STORE_PACKAGE_LIST_H
#define CBM_STORE_PACKAGE_LIST_H

#include <stdbool.h>

/* 依序以大小寫敏感的 C 字串比較判定 package 是否存在於陣列。 */
bool cbm_store_package_list_contains(const char *pkg, char **list, int count);

/* Rust opt-in wrapper 使用的 ABI v1。 */
bool cbm_rs_store_package_list_contains_v1(const char *pkg, char **list, int count);

#endif
