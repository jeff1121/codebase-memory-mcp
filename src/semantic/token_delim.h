#ifndef CBM_SEMANTIC_TOKEN_DELIM_H
#define CBM_SEMANTIC_TOKEN_DELIM_H

#include <stdbool.h>

/* 僅九個 ASCII token 分隔字元回傳 true；其他所有 byte 回傳 false。 */
bool cbm_semantic_is_token_delim(unsigned char byte);

#endif
