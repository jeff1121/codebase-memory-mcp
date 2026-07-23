#ifndef CBM_CYPHER_LEX_TWO_CHAR_H
#define CBM_CYPHER_LEX_TWO_CHAR_H

#include "cypher/cypher.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 公開 bridge：Two-character token classifier。
 *
 * 契約：
 * - 傳入連續兩字元 `c1`, `c2`
 * - 回傳對應的 `cbm_token_type_t` 符號（如 `TOK_NEQ`, `TOK_EQTILDE`, `TOK_GTE`, `TOK_LTE`,
 * `TOK_DOTDOT` 等）
 * - 未匹配時回傳 `TOK_EOF`
 * - 無 heap、無 I/O
 */
cbm_token_type_t cbm_cypher_lex_two_char(char c1, char c2);

#ifdef __cplusplus
}
#endif

#endif /* CBM_CYPHER_LEX_TWO_CHAR_H */
