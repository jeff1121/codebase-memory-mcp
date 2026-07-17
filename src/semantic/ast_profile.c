/*
 * ast_profile.c — Extraction-time AST structural signals.
 *
 * Walks a function body AST (alongside MinHash) to compute:
 *   Signal 8:  Control flow counts, nesting depth, expression types
 *   Signal 9:  Approximate data flow (params in returns/conditions)
 *   Signal 11: Halstead-lite (unique/total operators/operands)
 *
 * Pure functions — thread-safe, no shared state.
 */
#include "semantic/ast_profile.h"
#include "semantic/ast_profile_classifiers.h"
#include "foundation/constants.h"
#include "tree_sitter/api.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* ── Node type classification ────────────────────────────────────── */

enum {
    WALK_STACK_CAP = 2048,
    PROFILE_FIELD_COUNT = 25,
    DEPTH_SCALE = 10,
    BASE_DECIMAL_AST = 10,
};

typedef struct {
    TSNode node;
    int depth;
} profile_frame_t;

static bool kind_has_flag(const char *kind, uint32_t flag) {
    return (cbm_ast_profile_kind_flags(kind) & flag) != 0;
}

static bool is_control_if(const char *kind) {
    return kind_has_flag(kind, CBM_AST_PROFILE_KIND_IF);
}

static bool is_control_for(const char *kind) {
    return kind_has_flag(kind, CBM_AST_PROFILE_KIND_FOR);
}

static bool is_control_while(const char *kind) {
    return kind_has_flag(kind, CBM_AST_PROFILE_KIND_WHILE);
}

static bool is_control_switch(const char *kind) {
    return kind_has_flag(kind, CBM_AST_PROFILE_KIND_SWITCH);
}

static bool is_control_try(const char *kind) {
    return kind_has_flag(kind, CBM_AST_PROFILE_KIND_TRY);
}

static bool is_return(const char *kind) {
    return kind_has_flag(kind, CBM_AST_PROFILE_KIND_RETURN);
}

static bool is_comparison(const char *kind) {
    return kind_has_flag(kind, CBM_AST_PROFILE_KIND_COMPARISON);
}

static bool is_arithmetic(const char *kind) {
    return kind_has_flag(kind, CBM_AST_PROFILE_KIND_ARITHMETIC);
}

static bool is_assignment(const char *kind) {
    return kind_has_flag(kind, CBM_AST_PROFILE_KIND_ASSIGNMENT);
}

static bool is_string_lit(const char *kind) {
    return kind_has_flag(kind, CBM_AST_PROFILE_KIND_STRING_LITERAL);
}

static bool is_number_lit(const char *kind) {
    return kind_has_flag(kind, CBM_AST_PROFILE_KIND_NUMBER_LITERAL);
}

static bool is_bool_lit(const char *kind) {
    return kind_has_flag(kind, CBM_AST_PROFILE_KIND_BOOL_LITERAL);
}

static bool is_operator_node(const char *kind) {
    return kind_has_flag(kind, CBM_AST_PROFILE_KIND_OPERATOR);
}

static bool is_identifier(const char *kind) {
    return kind_has_flag(kind, CBM_AST_PROFILE_KIND_IDENTIFIER);
}

static bool is_param_name(const char *ident, const char *source, const char **param_names,
                          int param_count) {
    if (!ident || !source || !param_names || param_count <= 0) {
        return false;
    }
    return cbm_ast_profile_is_param_name(ident, param_names, param_count);
}

/* ── Main computation ────────────────────────────────────────────── */

/* Count control-flow statement kinds (if/for/while/switch/try/return). */
static void accumulate_control_flow(const char *kind, cbm_ast_profile_t *out, bool *in_return) {
    if (is_control_if(kind)) {
        out->if_count++;
    }
    if (is_control_for(kind)) {
        out->for_count++;
    }
    if (is_control_while(kind)) {
        out->while_count++;
    }
    if (is_control_switch(kind)) {
        out->switch_count++;
    }
    if (is_control_try(kind)) {
        out->try_count++;
    }
    if (is_return(kind)) {
        out->return_count++;
        *in_return = true;
    }
}

/* Count expression- and literal-class counters for one node. */
static void accumulate_expressions(const char *kind, cbm_ast_profile_t *out) {
    if (is_comparison(kind)) {
        out->comparison_ops++;
    }
    if (is_arithmetic(kind)) {
        out->arithmetic_ops++;
    }
    if (strcmp(kind, "not_operator") == 0 || strcmp(kind, "boolean_operator") == 0) {
        out->logical_ops++;
    }
    if (is_assignment(kind)) {
        out->assignment_count++;
        out->variable_reassigns++;
    }
    if (is_string_lit(kind)) {
        out->string_literals++;
    }
    if (is_number_lit(kind)) {
        out->number_literals++;
    }
    if (is_bool_lit(kind)) {
        out->bool_literals++;
    }
}

/* Accumulate Halstead operator/operand counts.  Leaf identifiers, literals,
 * and booleans are "operands"; recognized statement/expression kinds are
 * "operators".  Uses an open-addressed hash set for unique counting. */
static void accumulate_halstead(const char *kind, uint32_t child_count, uint32_t *op_set,
                                uint32_t *operand_set, cbm_ast_profile_t *out) {
    if (is_operator_node(kind)) {
        out->total_operators++;
        if (cbm_ast_profile_halstead_insert(op_set, kind)) {
            out->unique_operators++;
        }
    }
    if (child_count == 0 &&
        (is_identifier(kind) || is_string_lit(kind) || is_number_lit(kind) || is_bool_lit(kind))) {
        out->total_operands++;
        if (cbm_ast_profile_halstead_insert(operand_set, kind)) {
            out->unique_operands++;
        }
        out->body_tokens++;
    }
}

/* Data-flow counter update: bump params_in_returns / params_in_conditions
 * when the current leaf identifier names a parameter and we're inside the
 * corresponding syntactic scope. */
static void accumulate_data_flow(TSNode node, const char *kind, uint32_t child_count,
                                 const char *source, const char **param_names, int param_count,
                                 bool in_return, bool in_condition, cbm_ast_profile_t *out) {
    if (!(child_count == 0 && is_identifier(kind) && source)) {
        return;
    }
    uint32_t start = ts_node_start_byte(node);
    uint32_t end = ts_node_end_byte(node);
    if (end <= start || (end - start) >= CBM_SZ_128) {
        return;
    }
    char ident_buf[CBM_SZ_128];
    int ilen = (int)(end - start);
    memcpy(ident_buf, source + start, (size_t)ilen);
    ident_buf[ilen] = '\0';
    if (!is_param_name(ident_buf, source, param_names, param_count)) {
        return;
    }
    if (in_return) {
        out->params_in_returns++;
    }
    if (in_condition) {
        out->params_in_conditions++;
    }
}

bool cbm_ast_profile_compute(TSNode func_body, const char *source, const char **param_names,
                             int param_count, cbm_ast_profile_t *out) {
    if (ts_node_is_null(func_body)) {
        return false;
    }

    memset(out, 0, sizeof(*out));
    out->param_count = (uint16_t)param_count;

    uint32_t op_set[CBM_AST_PROFILE_HALSTEAD_SET_SIZE];
    uint32_t operand_set[CBM_AST_PROFILE_HALSTEAD_SET_SIZE];
    memset(op_set, 0, sizeof(op_set));
    memset(operand_set, 0, sizeof(operand_set));

    int total_depth = 0;
    int node_count = 0;
    bool in_return = false;
    bool in_condition = false;

    profile_frame_t stack[WALK_STACK_CAP];
    int top = 0;
    stack[top++] = (profile_frame_t){func_body, 0};

    while (top > 0) {
        profile_frame_t frame = stack[--top];
        TSNode node = frame.node;
        int depth = frame.depth;
        uint32_t child_count = ts_node_child_count(node);
        const char *kind = ts_node_type(node);

        if (!ts_node_is_named(node) && child_count == 0) {
            /* Anonymous leaf (punctuation, keywords) — skip. */
            goto push_children;
        }

        node_count++;
        total_depth += depth;

        if ((uint16_t)depth > out->max_nesting_depth) {
            out->max_nesting_depth = (uint16_t)depth;
        }

        accumulate_control_flow(kind, out, &in_return);
        accumulate_expressions(kind, out);
        accumulate_halstead(kind, child_count, op_set, operand_set, out);
        accumulate_data_flow(node, kind, child_count, source, param_names, param_count, in_return,
                             in_condition, out);

        /* Track context for data flow: are we inside a condition? */
        if (is_control_if(kind) || is_control_while(kind)) {
            in_condition = true;
        }

    push_children:
        /* Reset context flags when leaving return/condition scope */
        if (is_return(kind)) {
            in_return = false;
        }
        if (child_count > 0 && (is_control_if(kind) || is_control_while(kind))) {
            in_condition = false;
        }

        /* Push children in reverse order */
        for (int i = (int)child_count - SKIP_ONE; i >= 0 && top < WALK_STACK_CAP; i--) {
            stack[top++] = (profile_frame_t){ts_node_child(node, (uint32_t)i), depth + SKIP_ONE};
        }
    }

    /* Compute averages */
    if (node_count > 0) {
        out->avg_nesting_depth_x10 = (uint16_t)((total_depth * DEPTH_SCALE) / node_count);
    }

    return node_count > 0;
}

/* Rust direct slice 只取代純 codec；Tree-sitter traversal 仍保留 C fallback。 */
#ifndef CBM_USE_RUST_AST_PROFILE_CODEC_ONLY

/* ── Serialization ───────────────────────────────────────────────── */

void cbm_ast_profile_to_str(const cbm_ast_profile_t *p, char *buf, int bufsize) {
    if (!p || !buf || bufsize < SKIP_ONE) {
        if (buf && bufsize > 0) {
            buf[0] = '\0';
        }
        return;
    }
    snprintf(buf, (size_t)bufsize,
             "%u,%u,%u,%u,%u,%u,%u,%u,"
             "%u,%u,%u,%u,"
             "%u,%u,%u,"
             "%u,%u,%u,%u,"
             "%u,%u,%u,%u,"
             "%u,%u",
             p->if_count, p->for_count, p->while_count, p->switch_count, p->try_count,
             p->return_count, p->max_nesting_depth, p->avg_nesting_depth_x10, p->comparison_ops,
             p->arithmetic_ops, p->logical_ops, p->assignment_count, p->string_literals,
             p->number_literals, p->bool_literals, p->param_count, p->params_in_returns,
             p->params_in_conditions, p->variable_reassigns, p->unique_operators,
             p->unique_operands, p->total_operators, p->total_operands, p->body_lines,
             p->body_tokens);
}

bool cbm_ast_profile_from_str(const char *str, cbm_ast_profile_t *out) {
    if (!str || !out) {
        return false;
    }
    memset(out, 0, sizeof(*out));
    /* Field layout must match cbm_ast_profile_to_str() exactly. */
    uint16_t *fields[PROFILE_FIELD_COUNT] = {
        &out->if_count,           &out->for_count,
        &out->while_count,        &out->switch_count,
        &out->try_count,          &out->return_count,
        &out->max_nesting_depth,  &out->avg_nesting_depth_x10,
        &out->comparison_ops,     &out->arithmetic_ops,
        &out->logical_ops,        &out->assignment_count,
        &out->string_literals,    &out->number_literals,
        &out->bool_literals,      &out->param_count,
        &out->params_in_returns,  &out->params_in_conditions,
        &out->variable_reassigns, &out->unique_operators,
        &out->unique_operands,    &out->total_operators,
        &out->total_operands,     &out->body_lines,
        &out->body_tokens,
    };
    const char *p = str;
    for (int i = 0; i < PROFILE_FIELD_COUNT; i++) {
        char *end = NULL;
        unsigned long parsed = strtoul(p, &end, BASE_DECIMAL_AST);
        if (end == p) {
            return false;
        }
        *fields[i] = (uint16_t)parsed;
        p = end;
        /* Skip the comma separator between fields. */
        if (i + SKIP_ONE < PROFILE_FIELD_COUNT) {
            if (*p != ',') {
                return false;
            }
            p++;
        }
    }
    return true;
}

void cbm_ast_profile_to_vector(const cbm_ast_profile_t *p, float *out) {
    if (!p || !out) {
        return;
    }
    /* Normalize each field to [0,1] range using reasonable maximums. */
    enum { MAX_COUNT = 100, MAX_DEPTH = 20, MAX_HALSTEAD = 200, MAX_TOKENS = 2000 };
    int i = 0;
    out[i++] = (float)p->if_count / MAX_COUNT;
    out[i++] = (float)p->for_count / MAX_COUNT;
    out[i++] = (float)p->while_count / MAX_COUNT;
    out[i++] = (float)p->switch_count / MAX_COUNT;
    out[i++] = (float)p->try_count / MAX_COUNT;
    out[i++] = (float)p->return_count / MAX_COUNT;
    out[i++] = (float)p->max_nesting_depth / MAX_DEPTH;
    out[i++] = (float)p->avg_nesting_depth_x10 / (MAX_DEPTH * DEPTH_SCALE);
    out[i++] = (float)p->comparison_ops / MAX_COUNT;
    out[i++] = (float)p->arithmetic_ops / MAX_COUNT;
    out[i++] = (float)p->logical_ops / MAX_COUNT;
    out[i++] = (float)p->assignment_count / MAX_COUNT;
    out[i++] = (float)p->string_literals / MAX_COUNT;
    out[i++] = (float)p->number_literals / MAX_COUNT;
    out[i++] = (float)p->bool_literals / MAX_COUNT;
    out[i++] = (float)p->param_count / MAX_DEPTH;
    out[i++] = (float)p->params_in_returns / MAX_COUNT;
    out[i++] = (float)p->params_in_conditions / MAX_COUNT;
    out[i++] = (float)p->variable_reassigns / MAX_COUNT;
    out[i++] = (float)p->unique_operators / MAX_HALSTEAD;
    out[i++] = (float)p->unique_operands / MAX_HALSTEAD;
    out[i++] = (float)p->total_operators / MAX_HALSTEAD;
    out[i++] = (float)p->total_operands / MAX_HALSTEAD;
    out[i++] = (float)p->body_lines / MAX_TOKENS;
    out[i++] = (float)p->body_tokens / MAX_TOKENS;
}

#endif
