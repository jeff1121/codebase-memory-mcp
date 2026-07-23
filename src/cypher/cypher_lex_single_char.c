#include "cypher/cypher_lex_single_char.h"

#if defined(CBM_USE_RUST_CYPHER_LEX_SINGLE_CHAR) || \
    defined(CBM_USE_RUST_CYPHER_LEX_SINGLE_CHAR_ONLY)
extern int cbm_rs_cypher_single_char_kind_v1(int c);
#endif

cbm_token_type_t cbm_cypher_lex_single_char(char c) {
#if defined(CBM_USE_RUST_CYPHER_LEX_SINGLE_CHAR) || \
    defined(CBM_USE_RUST_CYPHER_LEX_SINGLE_CHAR_ONLY)
    int rust_type = cbm_rs_cypher_single_char_kind_v1((unsigned char)c);
    if (rust_type == TOK_EOF || (rust_type >= TOK_LPAREN && rust_type <= TOK_EQ) ||
        rust_type == TOK_PIPE) {
        return (cbm_token_type_t)rust_type;
    }
#endif
    switch (c) {
    case '(':
        return TOK_LPAREN;
    case ')':
        return TOK_RPAREN;
    case '[':
        return TOK_LBRACKET;
    case ']':
        return TOK_RBRACKET;
    case '-':
        return TOK_DASH;
    case '>':
        return TOK_GT;
    case '<':
        return TOK_LT;
    case ':':
        return TOK_COLON;
    case '.':
        return TOK_DOT;
    case '{':
        return TOK_LBRACE;
    case '}':
        return TOK_RBRACE;
    case '*':
        return TOK_STAR;
    case ',':
        return TOK_COMMA;
    case '=':
        return TOK_EQ;
    case '|':
        return TOK_PIPE;
    default:
        return TOK_EOF;
    }
}
