#include "semantic/ast_profile_classifiers.h"

#include <string.h>

#ifdef CBM_USE_RUST_AST_PROFILE_CLASSIFIERS
extern uint32_t cbm_rs_ast_profile_kind_flags_v1(const char *kind);
extern int cbm_rs_ast_profile_halstead_insert_v1(uint32_t *set, const char *key);
extern int cbm_rs_ast_profile_is_param_name_v1(const char *ident, const char *const *param_names,
                                               int param_count);

uint32_t cbm_ast_profile_kind_flags(const char *kind) {
    return cbm_rs_ast_profile_kind_flags_v1(kind);
}

bool cbm_ast_profile_halstead_insert(uint32_t *set, const char *key) {
    return cbm_rs_ast_profile_halstead_insert_v1(set, key) != 0;
}

bool cbm_ast_profile_is_param_name(const char *ident, const char **param_names, int param_count) {
    return cbm_rs_ast_profile_is_param_name_v1(ident, param_names, param_count) != 0;
}
#else
uint32_t cbm_ast_profile_kind_flags(const char *kind) {
    if (!kind) {
        return 0;
    }

    uint32_t flags = 0;
    const uint32_t operator_flags = CBM_AST_PROFILE_KIND_IF | CBM_AST_PROFILE_KIND_FOR |
                                    CBM_AST_PROFILE_KIND_WHILE | CBM_AST_PROFILE_KIND_SWITCH |
                                    CBM_AST_PROFILE_KIND_TRY | CBM_AST_PROFILE_KIND_RETURN |
                                    CBM_AST_PROFILE_KIND_COMPARISON |
                                    CBM_AST_PROFILE_KIND_ARITHMETIC |
                                    CBM_AST_PROFILE_KIND_ASSIGNMENT;

    if (strcmp(kind, "if_statement") == 0 || strcmp(kind, "if_expression") == 0 ||
        strcmp(kind, "elif_clause") == 0) {
        flags |= CBM_AST_PROFILE_KIND_IF;
    }
    if (strcmp(kind, "for_statement") == 0 || strcmp(kind, "for_range_loop") == 0 ||
        strcmp(kind, "for_expression") == 0 || strcmp(kind, "for_in_clause") == 0) {
        flags |= CBM_AST_PROFILE_KIND_FOR;
    }
    if (strcmp(kind, "while_statement") == 0 || strcmp(kind, "while_expression") == 0 ||
        strcmp(kind, "do_statement") == 0) {
        flags |= CBM_AST_PROFILE_KIND_WHILE;
    }
    if (strcmp(kind, "switch_statement") == 0 || strcmp(kind, "switch_expression") == 0 ||
        strcmp(kind, "match_expression") == 0 || strcmp(kind, "type_switch_statement") == 0) {
        flags |= CBM_AST_PROFILE_KIND_SWITCH;
    }
    if (strcmp(kind, "try_statement") == 0 || strcmp(kind, "try_expression") == 0 ||
        strcmp(kind, "catch_clause") == 0 || strcmp(kind, "except_clause") == 0) {
        flags |= CBM_AST_PROFILE_KIND_TRY;
    }
    if (strcmp(kind, "return_statement") == 0 || strcmp(kind, "return_expression") == 0) {
        flags |= CBM_AST_PROFILE_KIND_RETURN;
    }
    if (strcmp(kind, "binary_expression") == 0 || strcmp(kind, "comparison_operator") == 0 ||
        strcmp(kind, "boolean_operator") == 0) {
        flags |= CBM_AST_PROFILE_KIND_COMPARISON;
    }
    if (strcmp(kind, "unary_expression") == 0 || strcmp(kind, "update_expression") == 0) {
        flags |= CBM_AST_PROFILE_KIND_ARITHMETIC;
    }
    if (strcmp(kind, "assignment_expression") == 0 || strcmp(kind, "assignment_statement") == 0 ||
        strcmp(kind, "augmented_assignment") == 0 || strcmp(kind, "short_var_declaration") == 0) {
        flags |= CBM_AST_PROFILE_KIND_ASSIGNMENT;
    }
    if (strcmp(kind, "string") == 0 || strcmp(kind, "string_literal") == 0 ||
        strcmp(kind, "interpreted_string_literal") == 0 || strcmp(kind, "raw_string_literal") == 0) {
        flags |= CBM_AST_PROFILE_KIND_STRING_LITERAL;
    }
    if (strcmp(kind, "number") == 0 || strcmp(kind, "integer") == 0 || strcmp(kind, "float") == 0 ||
        strcmp(kind, "integer_literal") == 0 || strcmp(kind, "float_literal") == 0) {
        flags |= CBM_AST_PROFILE_KIND_NUMBER_LITERAL;
    }
    if (strcmp(kind, "true") == 0 || strcmp(kind, "false") == 0) {
        flags |= CBM_AST_PROFILE_KIND_BOOL_LITERAL;
    }
    if (strcmp(kind, "identifier") == 0 || strcmp(kind, "field_identifier") == 0 ||
        strcmp(kind, "property_identifier") == 0 || strcmp(kind, "type_identifier") == 0) {
        flags |= CBM_AST_PROFILE_KIND_IDENTIFIER;
    }
    if ((flags & operator_flags) != 0 || strcmp(kind, "call_expression") == 0 ||
        strcmp(kind, "member_expression") == 0 || strcmp(kind, "subscript_expression") == 0) {
        flags |= CBM_AST_PROFILE_KIND_OPERATOR;
    }
    return flags;
}

bool cbm_ast_profile_halstead_insert(uint32_t *set, const char *key) {
    if (!set || !key) {
        return false;
    }

    uint32_t hash = 0;
    const unsigned char *cursor = (const unsigned char *)key;
    while (*cursor) {
        hash = (hash * 31u) + (uint32_t)*cursor;
        cursor++;
    }

    uint32_t index = hash & (CBM_AST_PROFILE_HALSTEAD_SET_SIZE - 1u);
    uint32_t value = hash | 1u;
    for (uint32_t probe = 0; probe < CBM_AST_PROFILE_HALSTEAD_SET_SIZE; probe++) {
        uint32_t slot = (index + probe) & (CBM_AST_PROFILE_HALSTEAD_SET_SIZE - 1u);
        if (set[slot] == 0) {
            set[slot] = value;
            return true;
        }
        if (set[slot] == value) {
            return false;
        }
    }
    return false;
}

bool cbm_ast_profile_is_param_name(const char *ident, const char **param_names, int param_count) {
    if (!ident || !param_names || param_count <= 0) {
        return false;
    }
    for (int i = 0; i < param_count; i++) {
        if (param_names[i] && strcmp(ident, param_names[i]) == 0) {
            return true;
        }
    }
    return false;
}
#endif
