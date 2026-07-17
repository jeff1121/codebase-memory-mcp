#ifndef CBM_SEMANTIC_CAMEL_BREAK_H
#define CBM_SEMANTIC_CAMEL_BREAK_H

#include <stdbool.h>

/*
 * index 必須指向 name 中有效且非 NUL 的位元組；NULL、負值與零 index 回傳 false。
 */
bool cbm_semantic_is_camel_break(const char *name, int index);

#endif
