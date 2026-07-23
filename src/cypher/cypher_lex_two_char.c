#include "cypher/cypher_lex_two_char.h"

#if defined(CBM_USE_RUST_CYPHER_LEX_TWO_CHAR) || defined(CBM_USE_RUST_CYPHER_LEX_TWO_CHAR_ONLY)
extern int cbm_rs_cypher_two_char_kind_v1(int first, int second);
#endif

cbm_token_type_t cbm_cypher_lex_two_char(char c1, char c2) {
#if defined(CBM_USE_RUST_CYPHER_LEX_TWO_CHAR) || defined(CBM_USE_RUST_CYPHER_LEX_TWO_CHAR_ONLY)
    int rust_type = cbm_rs_cypher_two_char_kind_v1((unsigned char)c1, (unsigned char)c2);
    if (rust_type == TOK_NEQ || rust_type == TOK_EQTILDE || rust_type == TOK_GTE ||
        rust_type == TOK_LTE || rust_type == TOK_DOTDOT) {
        return (cbm_token_type_t)rust_type;
    }
#endif
    if (c1 == '!' && c2 == '=')
        return TOK_NEQ;
    if (c1 == '<' && c2 == '>')
        return TOK_NEQ;
    if (c1 == '=' && c2 == '~')
        return TOK_EQTILDE;
    if (c1 == '>' && c2 == '=')
        return TOK_GTE;
    if (c1 == '<' && c2 == '=')
        return TOK_LTE;
    if (c1 == '.' && c2 == '.')
        return TOK_DOTDOT;
    return TOK_EOF;
}
