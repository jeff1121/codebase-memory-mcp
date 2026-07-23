#ifndef CBM_CYPHER_LEX_SINGLE_CHAR_H
#define CBM_CYPHER_LEX_SINGLE_CHAR_H

#include "cypher/cypher.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 公開 bridge：Single-character token classifier。
 *
 * 契約：
 * - 傳入單一字元 `c`
 * - 回傳對應的 `cbm_token_type_t` 符號（如 `TOK_LPAREN`, `TOK_RPAREN`, `TOK_EQ`, `TOK_PIPE` 等）
 * - 未匹配時回傳 `TOK_EOF`
 * - 無 heap、無 I/O
 */
cbm_token_type_t cbm_cypher_lex_single_char(char c);

#ifdef __cplusplus
}
#endif

#endif /* CBM_CYPHER_LEX_SINGLE_CHAR_H */
