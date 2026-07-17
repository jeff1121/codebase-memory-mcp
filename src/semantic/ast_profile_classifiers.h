/*
 * AST profile 分類器的純 C ABI。
 */
#ifndef CBM_AST_PROFILE_CLASSIFIERS_H
#define CBM_AST_PROFILE_CLASSIFIERS_H

#include <stdbool.h>
#include <stdint.h>

enum {
    CBM_AST_PROFILE_KIND_IF = 1u << 0,
    CBM_AST_PROFILE_KIND_FOR = 1u << 1,
    CBM_AST_PROFILE_KIND_WHILE = 1u << 2,
    CBM_AST_PROFILE_KIND_SWITCH = 1u << 3,
    CBM_AST_PROFILE_KIND_TRY = 1u << 4,
    CBM_AST_PROFILE_KIND_RETURN = 1u << 5,
    CBM_AST_PROFILE_KIND_COMPARISON = 1u << 6,
    CBM_AST_PROFILE_KIND_ARITHMETIC = 1u << 7,
    CBM_AST_PROFILE_KIND_ASSIGNMENT = 1u << 8,
    CBM_AST_PROFILE_KIND_STRING_LITERAL = 1u << 9,
    CBM_AST_PROFILE_KIND_NUMBER_LITERAL = 1u << 10,
    CBM_AST_PROFILE_KIND_BOOL_LITERAL = 1u << 11,
    CBM_AST_PROFILE_KIND_OPERATOR = 1u << 12,
    CBM_AST_PROFILE_KIND_IDENTIFIER = 1u << 13,
    CBM_AST_PROFILE_HALSTEAD_SET_SIZE = 512,
};

uint32_t cbm_ast_profile_kind_flags(const char *kind);
bool cbm_ast_profile_halstead_insert(uint32_t *set, const char *key);
bool cbm_ast_profile_is_param_name(const char *ident, const char **param_names, int param_count);

#endif
