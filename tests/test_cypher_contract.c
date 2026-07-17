/*
 * test_cypher_contract.c — Phase 3 Rust 遷移前的 Cypher contract fixture。
 */
#include "test_framework.h"
#include <cypher/cypher.h>
#include <store/store.h>
#include <stdlib.h>
#include <string.h>

static cbm_store_t *setup_cypher_contract_store(void) {
    cbm_store_t *s = cbm_store_open_memory();
    if (!s) {
        return NULL;
    }
    if (cbm_store_upsert_project(s, "contract", "/tmp/contract") != CBM_STORE_OK) {
        return s;
    }

    cbm_node_t alpha = {.project = "contract",
                        .label = "Function",
                        .name = "Alpha",
                        .qualified_name = "contract.Alpha",
                        .file_path = "alpha.c"};
    cbm_node_t beta = {.project = "contract",
                       .label = "Function",
                       .name = "Beta",
                       .qualified_name = "contract.Beta",
                       .file_path = "beta.c"};
    cbm_node_t gamma = {.project = "contract",
                        .label = "Function",
                        .name = "Gamma",
                        .qualified_name = "contract.Gamma",
                        .file_path = "gamma.c"};

    int64_t alpha_id = cbm_store_upsert_node(s, &alpha);
    int64_t beta_id = cbm_store_upsert_node(s, &beta);
    int64_t gamma_id = cbm_store_upsert_node(s, &gamma);
    if (alpha_id <= 0 || beta_id <= 0 || gamma_id <= 0) {
        return s;
    }

    cbm_edge_t alpha_beta = {.project = "contract",
                             .source_id = alpha_id,
                             .target_id = beta_id,
                             .type = "CALLS",
                             .properties_json = "{\"confidence\":0.95,\"kind\":\"direct\"}"};
    cbm_edge_t alpha_gamma = {.project = "contract",
                              .source_id = alpha_id,
                              .target_id = gamma_id,
                              .type = "CALLS",
                              .properties_json = "{\"confidence\":0.75,\"kind\":\"fallback\"}"};
    (void)cbm_store_insert_edge(s, &alpha_beta);
    (void)cbm_store_insert_edge(s, &alpha_gamma);

    return s;
}

static const char *contract_col(const cbm_cypher_result_t *r, int row, const char *col) {
    for (int c = 0; c < r->col_count; c++) {
        if (strcmp(r->columns[c], col) == 0) {
            return r->rows[row][c];
        }
    }
    return NULL;
}

TEST(cypher_contract_parser_ast_shape) {
    cbm_query_t *q = NULL;
    char *err = NULL;
    int rc = cbm_cypher_parse("MATCH (n:Function|Module)-[r:CALLS|DEFINES*1..2]->(m:Function) "
                              "WHERE n.name = \"main\" OR n.name = \"Alpha\" AND m.name = \"Beta\" "
                              "RETURN n.name",
                              &q, &err);
    ASSERT_EQ(rc, 0);
    ASSERT_NULL(err);
    ASSERT_NOT_NULL(q);

    cbm_pattern_t pat = cbm_query_pattern(q);
    ASSERT_EQ(pat.node_count, 2);
    ASSERT_EQ(pat.rel_count, 1);
    ASSERT_STR_EQ(pat.nodes[0].variable, "n");
    ASSERT_STR_EQ(pat.nodes[0].label, "Function|Module");
    ASSERT_STR_EQ(pat.nodes[1].variable, "m");
    ASSERT_STR_EQ(pat.nodes[1].label, "Function");
    ASSERT_STR_EQ(pat.rels[0].variable, "r");
    ASSERT_EQ(pat.rels[0].type_count, 2);
    ASSERT_STR_EQ(pat.rels[0].types[0], "CALLS");
    ASSERT_STR_EQ(pat.rels[0].types[1], "DEFINES");
    ASSERT_STR_EQ(pat.rels[0].direction, "outbound");
    ASSERT_EQ(pat.rels[0].min_hops, 1);
    ASSERT_EQ(pat.rels[0].max_hops, 2);

    ASSERT_NOT_NULL(q->where);
    ASSERT_NOT_NULL(q->where->root);
    ASSERT_EQ(q->where->root->type, EXPR_OR);
    ASSERT_EQ(q->where->root->left->type, EXPR_CONDITION);
    ASSERT_STR_EQ(q->where->root->left->cond.variable, "n");
    ASSERT_STR_EQ(q->where->root->left->cond.property, "name");
    ASSERT_STR_EQ(q->where->root->left->cond.value, "main");
    ASSERT_EQ(q->where->root->right->type, EXPR_AND);
    ASSERT_EQ(q->ret->count, 1);
    ASSERT_STR_EQ(q->ret->items[0].variable, "n");
    ASSERT_STR_EQ(q->ret->items[0].property, "name");

    cbm_query_free(q);
    PASS();
}

TEST(cypher_contract_parser_hop_range_shorthand) {
    struct {
        const char *query;
        int min_hops;
        int max_hops;
    } cases[] = {
        {"MATCH (a:Function)-[:CALLS*2]->(b:Function) RETURN b.name", 1, 2},
        {"MATCH (a:Function)-[:CALLS*]->(b:Function) RETURN b.name", 1, 0},
        {"MATCH (a:Function)-[:CALLS*..3]->(b:Function) RETURN b.name", 1, 3},
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        cbm_query_t *q = NULL;
        char *err = NULL;
        int rc = cbm_cypher_parse(cases[i].query, &q, &err);
        ASSERT_EQ(rc, 0);
        ASSERT_NULL(err);
        ASSERT_NOT_NULL(q);
        cbm_pattern_t pat = cbm_query_pattern(q);
        ASSERT_EQ(pat.rel_count, 1);
        ASSERT_EQ(pat.rels[0].min_hops, cases[i].min_hops);
        ASSERT_EQ(pat.rels[0].max_hops, cases[i].max_hops);
        cbm_query_free(q);
    }
    PASS();
}

TEST(cypher_contract_optional_match_ast_shape) {
    cbm_query_t *q = NULL;
    char *err = NULL;
    int rc = cbm_cypher_parse(
        "MATCH (f:Function) OPTIONAL MATCH (f)-[:CALLS]->(g:Function) RETURN f.name, g.name", &q,
        &err);
    ASSERT_EQ(rc, 0);
    ASSERT_NULL(err);
    ASSERT_NOT_NULL(q);

    ASSERT_EQ(q->pattern_count, 2);
    ASSERT_FALSE(q->pattern_optional[0]);
    ASSERT_TRUE(q->pattern_optional[1]);
    ASSERT_EQ(q->patterns[1].node_count, 2);
    ASSERT_EQ(q->patterns[1].rel_count, 1);
    ASSERT_STR_EQ(q->patterns[1].nodes[0].variable, "f");
    ASSERT_STR_EQ(q->patterns[1].rels[0].types[0], "CALLS");
    ASSERT_STR_EQ(q->patterns[1].nodes[1].variable, "g");

    cbm_query_free(q);
    PASS();
}

TEST(cypher_contract_union_ast_shape) {
    cbm_query_t *q = NULL;
    char *err = NULL;
    int rc = cbm_cypher_parse(
        "MATCH (f:Function) RETURN f.name UNION ALL MATCH (g:Function) RETURN g.name", &q, &err);
    ASSERT_EQ(rc, 0);
    ASSERT_NULL(err);
    ASSERT_NOT_NULL(q);

    ASSERT_TRUE(q->union_all);
    ASSERT_NOT_NULL(q->union_next);
    ASSERT_EQ(q->ret->count, 1);
    ASSERT_STR_EQ(q->ret->items[0].variable, "f");
    ASSERT_EQ(q->union_next->ret->count, 1);
    ASSERT_STR_EQ(q->union_next->ret->items[0].variable, "g");

    cbm_query_free(q);
    PASS();
}

TEST(cypher_contract_ordered_rows) {
    cbm_store_t *s = setup_cypher_contract_store();
    ASSERT_NOT_NULL(s);
    cbm_cypher_result_t r = {0};

    int rc = cbm_cypher_execute(s,
                                "MATCH (f:Function)-[:CALLS]->(g:Function) "
                                "WHERE f.name = \"Alpha\" "
                                "RETURN f.name, g.name ORDER BY g.name ASC",
                                "contract", 0, &r);
    ASSERT_EQ(rc, 0);
    ASSERT_NULL(r.error);
    ASSERT_EQ(r.col_count, 2);
    ASSERT_STR_EQ(r.columns[0], "f.name");
    ASSERT_STR_EQ(r.columns[1], "g.name");
    ASSERT_EQ(r.row_count, 2);
    ASSERT_STR_EQ(r.rows[0][0], "Alpha");
    ASSERT_STR_EQ(r.rows[0][1], "Beta");
    ASSERT_STR_EQ(r.rows[1][0], "Alpha");
    ASSERT_STR_EQ(r.rows[1][1], "Gamma");

    cbm_cypher_result_free(&r);
    cbm_store_close(s);
    PASS();
}

TEST(cypher_contract_optional_match_keeps_unmatched_row) {
    cbm_store_t *s = setup_cypher_contract_store();
    ASSERT_NOT_NULL(s);
    cbm_cypher_result_t r = {0};

    int rc = cbm_cypher_execute(
        s,
        "MATCH (f:Function {name: \"Beta\"}) OPTIONAL MATCH (f)-[:CALLS]->(g:Function) "
        "RETURN f.name, g.name",
        "contract", 0, &r);
    ASSERT_EQ(rc, 0);
    ASSERT_NULL(r.error);
    ASSERT_EQ(r.col_count, 2);
    ASSERT_EQ(r.row_count, 1);
    ASSERT_STR_EQ(r.rows[0][0], "Beta");
    ASSERT_STR_EQ(r.rows[0][1], "");

    cbm_cypher_result_free(&r);
    cbm_store_close(s);
    PASS();
}

TEST(cypher_contract_union_dedupes_and_union_all_keeps_duplicates) {
    cbm_store_t *s = setup_cypher_contract_store();
    ASSERT_NOT_NULL(s);
    cbm_cypher_result_t r = {0};

    int rc = cbm_cypher_execute(s,
                                "MATCH (f:Function {name: \"Alpha\"}) RETURN f.name "
                                "UNION MATCH (g:Function {name: \"Alpha\"}) RETURN g.name",
                                "contract", 0, &r);
    ASSERT_EQ(rc, 0);
    ASSERT_NULL(r.error);
    ASSERT_EQ(r.col_count, 1);
    ASSERT_EQ(r.row_count, 1);
    ASSERT_STR_EQ(r.rows[0][0], "Alpha");
    cbm_cypher_result_free(&r);

    memset(&r, 0, sizeof(r));
    rc = cbm_cypher_execute(s,
                            "MATCH (f:Function {name: \"Alpha\"}) RETURN f.name "
                            "UNION ALL MATCH (g:Function {name: \"Alpha\"}) RETURN g.name",
                            "contract", 0, &r);
    ASSERT_EQ(rc, 0);
    ASSERT_NULL(r.error);
    ASSERT_EQ(r.col_count, 1);
    ASSERT_EQ(r.row_count, 2);
    ASSERT_STR_EQ(r.rows[0][0], "Alpha");
    ASSERT_STR_EQ(r.rows[1][0], "Alpha");

    cbm_cypher_result_free(&r);
    cbm_store_close(s);
    PASS();
}

TEST(cypher_contract_edge_properties_are_projected_and_filtered) {
    cbm_store_t *s = setup_cypher_contract_store();
    ASSERT_NOT_NULL(s);
    cbm_cypher_result_t r = {0};

    int rc = cbm_cypher_execute(s,
                                "MATCH (f:Function)-[r:CALLS]->(g:Function) "
                                "WHERE r.confidence >= 0.9 "
                                "RETURN f.name, g.name, r.type, r.confidence, r.kind",
                                "contract", 0, &r);
    ASSERT_EQ(rc, 0);
    ASSERT_NULL(r.error);
    ASSERT_EQ(r.row_count, 1);
    ASSERT_STR_EQ(contract_col(&r, 0, "f.name"), "Alpha");
    ASSERT_STR_EQ(contract_col(&r, 0, "g.name"), "Beta");
    ASSERT_STR_EQ(contract_col(&r, 0, "r.type"), "CALLS");
    ASSERT_STR_EQ(contract_col(&r, 0, "r.confidence"), "0.95");
    ASSERT_STR_EQ(contract_col(&r, 0, "r.kind"), "direct");

    cbm_cypher_result_free(&r);
    cbm_store_close(s);
    PASS();
}

TEST(cypher_contract_bare_edge_returns_properties_json) {
    cbm_store_t *s = setup_cypher_contract_store();
    ASSERT_NOT_NULL(s);
    cbm_cypher_result_t r = {0};

    int rc = cbm_cypher_execute(
        s, "MATCH (f:Function)-[r:CALLS]->(g:Function) WHERE r.kind = \"direct\" RETURN r",
        "contract", 0, &r);
    ASSERT_EQ(rc, 0);
    ASSERT_NULL(r.error);
    ASSERT_EQ(r.col_count, 1);
    ASSERT_STR_EQ(r.columns[0], "r");
    ASSERT_EQ(r.row_count, 1);
    ASSERT_STR_EQ(r.rows[0][0], "{\"confidence\":0.95,\"kind\":\"direct\"}");

    cbm_cypher_result_free(&r);
    cbm_store_close(s);
    PASS();
}

TEST(cypher_contract_exists_predicate_ast_shape) {
    cbm_query_t *q = NULL;
    char *err = NULL;
    int rc = cbm_cypher_parse(
        "MATCH (f:Function) WHERE NOT EXISTS { (f)<-[:CALLS]-() } RETURN f.name", &q, &err);
    ASSERT_EQ(rc, 0);
    ASSERT_NULL(err);
    ASSERT_NOT_NULL(q);

    ASSERT_NOT_NULL(q->where);
    ASSERT_NOT_NULL(q->where->root);
    ASSERT_EQ(q->where->root->type, EXPR_NOT);
    ASSERT_NOT_NULL(q->where->root->left);
    ASSERT_NULL(q->where->root->right);
    ASSERT_EQ(q->where->root->left->type, EXPR_CONDITION);
    ASSERT_STR_EQ(q->where->root->left->cond.variable, "f");
    ASSERT_NULL(q->where->root->left->cond.property);
    ASSERT_STR_EQ(q->where->root->left->cond.op, "EXISTS");
    ASSERT_STR_EQ(q->where->root->left->cond.value, "CALLS");
    ASSERT_FALSE(q->where->root->left->cond.negated);
    ASSERT_EQ(q->where->root->left->cond.exists_dir, 1);

    cbm_query_free(q);
    PASS();
}

TEST(cypher_contract_exists_predicate_filters_by_direction_and_type) {
    cbm_store_t *s = setup_cypher_contract_store();
    ASSERT_NOT_NULL(s);
    cbm_cypher_result_t r = {0};

    int rc = cbm_cypher_execute(s,
                                "MATCH (f:Function) "
                                "WHERE EXISTS { (f)-[:CALLS]->() } "
                                "RETURN f.name ORDER BY f.name ASC",
                                "contract", 0, &r);
    ASSERT_EQ(rc, 0);
    ASSERT_NULL(r.error);
    ASSERT_EQ(r.row_count, 1);
    ASSERT_STR_EQ(r.rows[0][0], "Alpha");
    cbm_cypher_result_free(&r);

    memset(&r, 0, sizeof(r));
    rc = cbm_cypher_execute(s,
                            "MATCH (f:Function) "
                            "WHERE NOT EXISTS { (f)<-[:CALLS]-() } "
                            "RETURN f.name ORDER BY f.name ASC",
                            "contract", 0, &r);
    ASSERT_EQ(rc, 0);
    ASSERT_NULL(r.error);
    ASSERT_EQ(r.row_count, 1);
    ASSERT_STR_EQ(r.rows[0][0], "Alpha");

    cbm_cypher_result_free(&r);
    cbm_store_close(s);
    PASS();
}

TEST(cypher_contract_exists_predicate_any_direction_filters) {
    cbm_store_t *s = setup_cypher_contract_store();
    ASSERT_NOT_NULL(s);
    cbm_cypher_result_t r = {0};

    int rc = cbm_cypher_execute(s,
                                "MATCH (f:Function) "
                                "WHERE EXISTS { (f)-[:CALLS]-() } "
                                "RETURN f.name ORDER BY f.name ASC",
                                "contract", 0, &r);
    ASSERT_EQ(rc, 0);
    ASSERT_NULL(r.error);
    ASSERT_EQ(r.row_count, 3);
    ASSERT_STR_EQ(r.rows[0][0], "Alpha");
    ASSERT_STR_EQ(r.rows[1][0], "Beta");
    ASSERT_STR_EQ(r.rows[2][0], "Gamma");

    cbm_cypher_result_free(&r);
    cbm_store_close(s);
    PASS();
}

TEST(cypher_contract_aggregation_count_distinct) {
    cbm_store_t *s = setup_cypher_contract_store();
    ASSERT_NOT_NULL(s);
    cbm_cypher_result_t r = {0};

    int rc = cbm_cypher_execute(
        s,
        "MATCH (f:Function) "
        "RETURN COUNT(f.name) AS funcs, COUNT(DISTINCT f.label) AS labels",
        "contract", 0, &r);
    ASSERT_EQ(rc, 0);
    ASSERT_NULL(r.error);
    ASSERT_EQ(r.col_count, 2);
    ASSERT_EQ(r.row_count, 1);
    ASSERT_STR_EQ(contract_col(&r, 0, "funcs"), "3");
    ASSERT_STR_EQ(contract_col(&r, 0, "labels"), "1");

    cbm_cypher_result_free(&r);
    cbm_store_close(s);
    PASS();
}

TEST(cypher_contract_aggregation_edge_properties_numeric) {
    cbm_store_t *s = setup_cypher_contract_store();
    ASSERT_NOT_NULL(s);
    cbm_cypher_result_t r = {0};

    int rc = cbm_cypher_execute(s,
                                "MATCH ()-[r:CALLS]->() "
                                "RETURN SUM(r.confidence) AS sum_conf, "
                                "AVG(r.confidence) AS avg_conf, MIN(r.confidence) AS min_conf, "
                                "MAX(r.confidence) AS max_conf",
                                "contract", 0, &r);
    ASSERT_EQ(rc, 0);
    ASSERT_NULL(r.error);
    ASSERT_EQ(r.col_count, 4);
    ASSERT_EQ(r.row_count, 1);
    ASSERT_STR_EQ(contract_col(&r, 0, "sum_conf"), "1.7");
    ASSERT_STR_EQ(contract_col(&r, 0, "avg_conf"), "0.85");
    ASSERT_STR_EQ(contract_col(&r, 0, "min_conf"), "0.75");
    ASSERT_STR_EQ(contract_col(&r, 0, "max_conf"), "0.95");

    cbm_cypher_result_free(&r);
    cbm_store_close(s);
    PASS();
}

TEST(cypher_contract_with_post_where_ast_shape) {
    cbm_query_t *q = NULL;
    char *err = NULL;
    int rc =
        cbm_cypher_parse("MATCH (f:Function)-[:CALLS]->(g:Function) "
                         "WITH f.name AS caller, COUNT(g) AS cnt WHERE cnt > \"1\" RETURN caller",
                         &q, &err);
    ASSERT_EQ(rc, 0);
    ASSERT_NULL(err);
    ASSERT_NOT_NULL(q);

    ASSERT_NOT_NULL(q->with_clause);
    ASSERT_EQ(q->with_clause->count, 2);
    ASSERT_STR_EQ(q->with_clause->items[0].variable, "f");
    ASSERT_STR_EQ(q->with_clause->items[0].property, "name");
    ASSERT_STR_EQ(q->with_clause->items[0].alias, "caller");
    ASSERT_STR_EQ(q->with_clause->items[1].func, "COUNT");
    ASSERT_STR_EQ(q->with_clause->items[1].variable, "g");
    ASSERT_STR_EQ(q->with_clause->items[1].alias, "cnt");

    ASSERT_NOT_NULL(q->post_with_where);
    ASSERT_NOT_NULL(q->post_with_where->root);
    ASSERT_EQ(q->post_with_where->root->type, EXPR_CONDITION);
    ASSERT_STR_EQ(q->post_with_where->root->cond.variable, "cnt");
    ASSERT_NULL(q->post_with_where->root->cond.property);
    ASSERT_STR_EQ(q->post_with_where->root->cond.op, ">");
    ASSERT_STR_EQ(q->post_with_where->root->cond.value, "1");

    ASSERT_NOT_NULL(q->ret);
    ASSERT_EQ(q->ret->count, 1);
    ASSERT_STR_EQ(q->ret->items[0].variable, "caller");
    ASSERT_NULL(q->ret->items[0].property);

    cbm_query_free(q);
    PASS();
}

TEST(cypher_contract_multiarg_function_projection) {
    cbm_store_t *s = setup_cypher_contract_store();
    ASSERT_NOT_NULL(s);
    cbm_cypher_result_t r = {0};

    int rc = cbm_cypher_execute(
        s,
        "MATCH (f:Function) WHERE f.name = \"Alpha\" "
        "RETURN substring(f.name, 0, 2), left(f.name, 3), "
        "right(f.name, 2), replace(f.name, \"Al\", \"Om\"), "
        "coalesce(f.missing, \"fallback\")",
        "contract", 0, &r);
    ASSERT_EQ(rc, 0);
    ASSERT_NULL(r.error);
    ASSERT_EQ(r.row_count, 1);
    ASSERT_EQ(r.col_count, 5);
    ASSERT_STR_EQ(r.rows[0][0], "Al");
    ASSERT_STR_EQ(r.rows[0][1], "Alp");
    ASSERT_STR_EQ(r.rows[0][2], "ha");
    ASSERT_STR_EQ(r.rows[0][3], "Ompha");
    ASSERT_STR_EQ(r.rows[0][4], "fallback");

    cbm_cypher_result_free(&r);
    cbm_store_close(s);
    PASS();
}

TEST(cypher_contract_limit_zero_keeps_columns) {
    cbm_store_t *s = setup_cypher_contract_store();
    ASSERT_NOT_NULL(s);
    cbm_cypher_result_t r = {0};

    int rc = cbm_cypher_execute(s, "MATCH (f:Function) RETURN f.name ORDER BY f.name ASC LIMIT 0",
                                "contract", 0, &r);
    ASSERT_EQ(rc, 0);
    ASSERT_NULL(r.error);
    ASSERT_EQ(r.col_count, 1);
    ASSERT_STR_EQ(r.columns[0], "f.name");
    ASSERT_EQ(r.row_count, 0);

    cbm_cypher_result_free(&r);
    cbm_store_close(s);
    PASS();
}

TEST(cypher_contract_exact_unknown_function_error) {
    cbm_query_t *q = NULL;
    char *err = NULL;
    int rc = cbm_cypher_parse("MATCH (f:Function) RETURN nosuchfunc(f.name)", &q, &err);
    ASSERT_NEQ(rc, 0);
    ASSERT_NULL(q);
    ASSERT_NOT_NULL(err);
    ASSERT_STR_EQ(err, "unsupported function 'nosuchfunc' (supported: count, sum, avg, min, max, "
                       "collect, toLower, toUpper, toString, toInteger, toFloat, toBoolean, size, "
                       "length, trim, ltrim, rtrim, reverse, labels, type, id, keys, properties)");

    free(err);
    PASS();
}

TEST(cypher_contract_exact_create_error) {
    cbm_query_t *q = NULL;
    char *err = NULL;
    int rc = cbm_cypher_parse("CREATE (n:Node {name: \"X\"})", &q, &err);
    ASSERT_NEQ(rc, 0);
    ASSERT_NULL(q);
    ASSERT_NOT_NULL(err);
    ASSERT_STR_EQ(err,
                  "unsupported Cypher feature: CREATE clause (write operations not supported)");

    free(err);
    PASS();
}

TEST(cypher_contract_exact_call_error) {
    cbm_query_t *q = NULL;
    char *err = NULL;
    int rc = cbm_cypher_parse("CALL db.labels()", &q, &err);
    ASSERT_NEQ(rc, 0);
    ASSERT_NULL(q);
    ASSERT_NOT_NULL(err);
    ASSERT_STR_EQ(err, "unsupported Cypher feature: CALL clause (stored procedures not supported)");

    free(err);
    PASS();
}

TEST(cypher_contract_exact_exists_brace_error) {
    cbm_query_t *q = NULL;
    char *err = NULL;
    int rc = cbm_cypher_parse("MATCH (f:Function) WHERE EXISTS (f) RETURN f.name", &q, &err);
    ASSERT_NEQ(rc, 0);
    ASSERT_NULL(q);
    ASSERT_NOT_NULL(err);
    ASSERT_STR_EQ(err, "expected '{' after EXISTS at pos 32");

    free(err);
    PASS();
}

TEST(cypher_contract_exact_list_indexing_error) {
    cbm_query_t *q = NULL;
    char *err = NULL;
    int rc = cbm_cypher_parse("MATCH (f:Function) RETURN f.name[0]", &q, &err);
    ASSERT_NEQ(rc, 0);
    ASSERT_NULL(q);
    ASSERT_NOT_NULL(err);
    ASSERT_STR_EQ(err, "unsupported expression: list indexing/slicing '[...]' is not supported");

    free(err);
    PASS();
}

SUITE(cypher_contract) {
    RUN_TEST(cypher_contract_parser_ast_shape);
    RUN_TEST(cypher_contract_parser_hop_range_shorthand);
    RUN_TEST(cypher_contract_optional_match_ast_shape);
    RUN_TEST(cypher_contract_union_ast_shape);
    RUN_TEST(cypher_contract_ordered_rows);
    RUN_TEST(cypher_contract_optional_match_keeps_unmatched_row);
    RUN_TEST(cypher_contract_union_dedupes_and_union_all_keeps_duplicates);
    RUN_TEST(cypher_contract_edge_properties_are_projected_and_filtered);
    RUN_TEST(cypher_contract_bare_edge_returns_properties_json);
    RUN_TEST(cypher_contract_exists_predicate_ast_shape);
    RUN_TEST(cypher_contract_exists_predicate_filters_by_direction_and_type);
    RUN_TEST(cypher_contract_exists_predicate_any_direction_filters);
    RUN_TEST(cypher_contract_aggregation_count_distinct);
    RUN_TEST(cypher_contract_aggregation_edge_properties_numeric);
    RUN_TEST(cypher_contract_with_post_where_ast_shape);
    RUN_TEST(cypher_contract_multiarg_function_projection);
    RUN_TEST(cypher_contract_limit_zero_keeps_columns);
    RUN_TEST(cypher_contract_exact_unknown_function_error);
    RUN_TEST(cypher_contract_exact_create_error);
    RUN_TEST(cypher_contract_exact_call_error);
    RUN_TEST(cypher_contract_exact_exists_brace_error);
    RUN_TEST(cypher_contract_exact_list_indexing_error);
}
