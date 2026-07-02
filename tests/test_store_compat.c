/*
 * test_store_compat.c — Phase 3 Rust 遷移前的 SQLite store contract。
 */
#include "test_framework.h"
#include <foundation/compat.h>
#include <store/store.h>
#include <sqlite3.h>
#include <string.h>

static const char *USER_INDEXES[] = {
    "idx_nodes_label",       "idx_nodes_name",
    "idx_nodes_file",        "idx_edges_source",
    "idx_edges_target",      "idx_edges_type",
    "idx_edges_target_type", "idx_edges_source_type",
    "idx_edges_url_path",    NULL,
};

static int schema_object_exists(sqlite3 *db, const char *type, const char *name) {
    sqlite3_stmt *stmt = NULL;
    int exists = -1;
    const char *sql = "SELECT count(*) FROM sqlite_master WHERE type = ?1 AND name = ?2;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_text(stmt, 1, type, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, name, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        exists = sqlite3_column_int(stmt, 0) > 0 ? 1 : 0;
    }
    sqlite3_finalize(stmt);
    return exists;
}

static int table_column_exists(sqlite3 *db, const char *table, const char *column) {
    sqlite3_stmt *stmt = NULL;
    char sql[128];
    int exists = 0;
    snprintf(sql, sizeof(sql), "PRAGMA table_xinfo(%s);", table);
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *name = sqlite3_column_text(stmt, 1);
        if (name && strcmp((const char *)name, column) == 0) {
            exists = 1;
            break;
        }
    }
    sqlite3_finalize(stmt);
    return exists;
}

static int sql_scalar_int(sqlite3 *db, const char *sql) {
    sqlite3_stmt *stmt = NULL;
    int value = -1;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        value = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return value;
}

static int sql_scalar_text(sqlite3 *db, const char *sql, char *buf, size_t n) {
    sqlite3_stmt *stmt = NULL;
    if (!buf || n == 0 || sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    int ok = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *value = sqlite3_column_text(stmt, 0);
        snprintf(buf, n, "%s", value ? (const char *)value : "");
        ok = 0;
    }
    sqlite3_finalize(stmt);
    return ok;
}

static int explain_plan_mentions(sqlite3 *db, const char *sql, const char *needle) {
    sqlite3_stmt *stmt = NULL;
    int found = 0;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *detail = sqlite3_column_text(stmt, 3);
        if (detail && strstr((const char *)detail, needle)) {
            found = 1;
            break;
        }
    }
    sqlite3_finalize(stmt);
    return found;
}

static void make_temp_store_path(char *buf, size_t n, const char *suffix) {
    snprintf(buf, n, "%s/cbm_store_compat_%d_%s.db", cbm_tmpdir(), (int)getpid(), suffix);
}

static void cleanup_store_files(const char *path) {
    remove(path);
    char aux[512];
    snprintf(aux, sizeof(aux), "%s-wal", path);
    remove(aux);
    snprintf(aux, sizeof(aux), "%s-shm", path);
    remove(aux);
}

TEST(store_compat_schema_manifest) {
    cbm_store_t *s = cbm_store_open_memory();
    ASSERT_NOT_NULL(s);
    sqlite3 *db = cbm_store_get_db(s);
    ASSERT_NOT_NULL(db);

    ASSERT_EQ(schema_object_exists(db, "table", "projects"), 1);
    ASSERT_EQ(schema_object_exists(db, "table", "file_hashes"), 1);
    ASSERT_EQ(schema_object_exists(db, "table", "nodes"), 1);
    ASSERT_EQ(schema_object_exists(db, "table", "edges"), 1);
    ASSERT_EQ(schema_object_exists(db, "table", "project_summaries"), 1);
    ASSERT_EQ(schema_object_exists(db, "table", "nodes_fts"), 1);
    ASSERT_EQ(table_column_exists(db, "edges", "url_path_gen"), 1);

    cbm_store_close(s);
    PASS();
}

TEST(store_compat_user_index_manifest_and_drop_symmetry) {
    cbm_store_t *s = cbm_store_open_memory();
    ASSERT_NOT_NULL(s);
    sqlite3 *db = cbm_store_get_db(s);
    ASSERT_NOT_NULL(db);

    for (int i = 0; USER_INDEXES[i]; i++) {
        ASSERT_EQ(schema_object_exists(db, "index", USER_INDEXES[i]), 1);
    }

    ASSERT_EQ(cbm_store_drop_indexes(s), CBM_STORE_OK);
    for (int i = 0; USER_INDEXES[i]; i++) {
        ASSERT_EQ(schema_object_exists(db, "index", USER_INDEXES[i]), 0);
    }

    ASSERT_EQ(cbm_store_create_indexes(s), CBM_STORE_OK);
    for (int i = 0; USER_INDEXES[i]; i++) {
        ASSERT_EQ(schema_object_exists(db, "index", USER_INDEXES[i]), 1);
    }

    cbm_store_close(s);
    PASS();
}

TEST(store_compat_query_open_is_readonly_and_no_create) {
    char db_path[512];
    make_temp_store_path(db_path, sizeof(db_path), "query_open");
    cleanup_store_files(db_path);

    cbm_store_t *missing = cbm_store_open_path_query(db_path);
    ASSERT_NULL(missing);
    ASSERT_NEQ(access(db_path, F_OK), 0);

    cbm_store_t *writer = cbm_store_open_path(db_path);
    ASSERT_NOT_NULL(writer);
    ASSERT_EQ(cbm_store_upsert_project(writer, "test", "/tmp/test"), CBM_STORE_OK);
    cbm_store_close(writer);

    cbm_store_t *reader = cbm_store_open_path_query(db_path);
    ASSERT_NOT_NULL(reader);
    sqlite3 *db = cbm_store_get_db(reader);
    ASSERT_NOT_NULL(db);
    ASSERT_EQ(sqlite3_db_readonly(db, "main"), 1);

    cbm_project_t project = {0};
    ASSERT_EQ(cbm_store_get_project(reader, "test", &project), CBM_STORE_OK);
    ASSERT_STR_EQ(project.name, "test");
    ASSERT_STR_EQ(project.root_path, "/tmp/test");
    cbm_project_free_fields(&project);

    ASSERT_EQ(cbm_store_exec(reader, "INSERT INTO projects(name,indexed_at,root_path) "
                                     "VALUES('write','now','/tmp/write');"),
              CBM_STORE_ERR);
    cbm_store_close(reader);
    cleanup_store_files(db_path);
    PASS();
}

TEST(store_compat_path_open_uses_wal_journal_mode) {
    char db_path[512];
    make_temp_store_path(db_path, sizeof(db_path), "wal_mode");
    cleanup_store_files(db_path);

    cbm_store_t *writer = cbm_store_open_path(db_path);
    ASSERT_NOT_NULL(writer);
    sqlite3 *db = cbm_store_get_db(writer);
    ASSERT_NOT_NULL(db);

    char mode[32];
    ASSERT_EQ(sql_scalar_text(db, "PRAGMA journal_mode;", mode, sizeof(mode)), 0);
    ASSERT_STR_EQ(mode, "wal");
    ASSERT_EQ(cbm_store_upsert_project(writer, "wal-test", "/tmp/wal-test"), CBM_STORE_OK);

    cbm_store_close(writer);
    cleanup_store_files(db_path);
    PASS();
}

TEST(store_compat_query_open_reads_existing_wal_and_rejects_write) {
    char db_path[512];
    make_temp_store_path(db_path, sizeof(db_path), "query_wal");
    cleanup_store_files(db_path);

    cbm_store_t *writer = cbm_store_open_path(db_path);
    ASSERT_NOT_NULL(writer);
    sqlite3 *writer_db = cbm_store_get_db(writer);
    ASSERT_NOT_NULL(writer_db);

    char mode[32];
    ASSERT_EQ(sql_scalar_text(writer_db, "PRAGMA journal_mode;", mode, sizeof(mode)), 0);
    ASSERT_STR_EQ(mode, "wal");
    ASSERT_EQ(cbm_store_upsert_project(writer, "wal-query", "/tmp/wal-query"), CBM_STORE_OK);

    cbm_store_t *reader = cbm_store_open_path_query(db_path);
    ASSERT_NOT_NULL(reader);
    sqlite3 *reader_db = cbm_store_get_db(reader);
    ASSERT_NOT_NULL(reader_db);
    ASSERT_EQ(sqlite3_db_readonly(reader_db, "main"), 1);

    cbm_project_t project = {0};
    ASSERT_EQ(cbm_store_get_project(reader, "wal-query", &project), CBM_STORE_OK);
    ASSERT_STR_EQ(project.name, "wal-query");
    ASSERT_STR_EQ(project.root_path, "/tmp/wal-query");
    cbm_project_free_fields(&project);

    ASSERT_EQ(cbm_store_exec(reader, "INSERT INTO projects(name,indexed_at,root_path) "
                                     "VALUES('readonly-write','now','/tmp/readonly-write');"),
              CBM_STORE_ERR);

    cbm_store_close(reader);
    cbm_store_close(writer);
    cleanup_store_files(db_path);
    PASS();
}

TEST(store_compat_url_path_project_scoped_substring) {
    cbm_store_t *s = cbm_store_open_memory();
    ASSERT_NOT_NULL(s);
    ASSERT_EQ(cbm_store_upsert_project(s, "test", "/tmp/test"), CBM_STORE_OK);
    ASSERT_EQ(cbm_store_upsert_project(s, "other", "/tmp/other"), CBM_STORE_OK);

    cbm_node_t a = {
        .project = "test", .label = "Function", .name = "caller", .qualified_name = "test.caller"};
    cbm_node_t b = {.project = "test",
                    .label = "Function",
                    .name = "handler",
                    .qualified_name = "test.handler"};
    cbm_node_t c = {.project = "other",
                    .label = "Function",
                    .name = "hidden",
                    .qualified_name = "other.hidden"};
    int64_t caller = cbm_store_upsert_node(s, &a);
    int64_t handler = cbm_store_upsert_node(s, &b);
    int64_t hidden = cbm_store_upsert_node(s, &c);
    ASSERT_GT(caller, 0);
    ASSERT_GT(handler, 0);
    ASSERT_GT(hidden, 0);

    cbm_edge_t orders = {.project = "test",
                         .source_id = caller,
                         .target_id = handler,
                         .type = "HTTP_CALLS",
                         .properties_json =
                             "{\"url_path\":\"/api/orders/create\",\"confidence\":0.8}"};
    cbm_edge_t invoices = {.project = "test",
                           .source_id = handler,
                           .target_id = caller,
                           .type = "HTTP_CALLS",
                           .properties_json =
                               "{\"url_path\":\"/api/invoices/create\",\"confidence\":0.7}"};
    cbm_edge_t other = {.project = "other",
                        .source_id = hidden,
                        .target_id = hidden,
                        .type = "HTTP_CALLS",
                        .properties_json =
                            "{\"url_path\":\"/api/orders/hidden\",\"confidence\":0.1}"};
    cbm_edge_t no_url = {.project = "test",
                         .source_id = caller,
                         .target_id = hidden,
                         .type = "CALLS",
                         .properties_json = "{\"note\":\"orders without url_path\"}"};
    ASSERT_GT(cbm_store_insert_edge(s, &orders), 0);
    ASSERT_GT(cbm_store_insert_edge(s, &invoices), 0);
    ASSERT_GT(cbm_store_insert_edge(s, &other), 0);
    ASSERT_GT(cbm_store_insert_edge(s, &no_url), 0);

    cbm_edge_t *edges = NULL;
    int count = 0;
    ASSERT_EQ(cbm_store_find_edges_by_url_path(s, "test", "orders", &edges, &count), CBM_STORE_OK);
    ASSERT_EQ(count, 1);
    ASSERT_STR_EQ(edges[0].project, "test");
    ASSERT_NOT_NULL(strstr(edges[0].properties_json, "/api/orders/create"));
    cbm_store_free_edges(edges, count);

    edges = NULL;
    count = 0;
    ASSERT_EQ(cbm_store_find_edges_by_url_path(s, "test", "api", &edges, &count), CBM_STORE_OK);
    ASSERT_EQ(count, 2);
    cbm_store_free_edges(edges, count);

    edges = NULL;
    count = 0;
    ASSERT_EQ(cbm_store_find_edges_by_url_path(s, "other", "orders", &edges, &count), CBM_STORE_OK);
    ASSERT_EQ(count, 1);
    ASSERT_STR_EQ(edges[0].project, "other");
    cbm_store_free_edges(edges, count);

    cbm_store_close(s);
    PASS();
}

TEST(store_compat_url_path_generated_column_index_plan) {
    cbm_store_t *s = cbm_store_open_memory();
    ASSERT_NOT_NULL(s);
    sqlite3 *db = cbm_store_get_db(s);
    ASSERT_NOT_NULL(db);
    ASSERT_EQ(cbm_store_upsert_project(s, "test", "/tmp/test"), CBM_STORE_OK);

    cbm_node_t caller = {
        .project = "test", .label = "Function", .name = "caller", .qualified_name = "test.caller"};
    cbm_node_t handler = {.project = "test",
                          .label = "Function",
                          .name = "handler",
                          .qualified_name = "test.handler"};
    int64_t caller_id = cbm_store_upsert_node(s, &caller);
    int64_t handler_id = cbm_store_upsert_node(s, &handler);
    ASSERT_GT(caller_id, 0);
    ASSERT_GT(handler_id, 0);

    cbm_edge_t edge = {.project = "test",
                       .source_id = caller_id,
                       .target_id = handler_id,
                       .type = "HTTP_CALLS",
                       .properties_json =
                           "{\"url_path\":\"/api/orders/create\",\"confidence\":0.8}"};
    ASSERT_GT(cbm_store_insert_edge(s, &edge), 0);

    ASSERT_EQ(sql_scalar_int(db, "SELECT count(*) FROM edges "
                                 "WHERE project = 'test' "
                                 "AND url_path_gen = '/api/orders/create';"),
              1);
    ASSERT_EQ(explain_plan_mentions(db,
                                    "EXPLAIN QUERY PLAN "
                                    "SELECT id FROM edges "
                                    "WHERE project = 'test' "
                                    "AND url_path_gen = '/api/orders/create';",
                                    "idx_edges_url_path"),
              1);

    cbm_store_close(s);
    PASS();
}

TEST(store_compat_fts_manual_rebuild_and_camel_split) {
    cbm_store_t *s = cbm_store_open_memory();
    ASSERT_NOT_NULL(s);
    sqlite3 *db = cbm_store_get_db(s);
    ASSERT_NOT_NULL(db);

    ASSERT_EQ(cbm_store_upsert_project(s, "test", "/tmp/test"), CBM_STORE_OK);
    cbm_node_t n1 = {.project = "test",
                     .label = "Function",
                     .name = "updateCloudClient",
                     .qualified_name = "test.updateCloudClient",
                     .file_path = "src/cloud.c"};
    cbm_node_t n2 = {.project = "test",
                     .label = "Function",
                     .name = "cloudAdapter",
                     .qualified_name = "test.cloudAdapter",
                     .file_path = "src/cloud.c"};
    cbm_node_t n3 = {.project = "test",
                     .label = "Function",
                     .name = "renderPage",
                     .qualified_name = "test.renderPage",
                     .file_path = "src/render.c"};
    ASSERT_GT(cbm_store_upsert_node(s, &n1), 0);
    ASSERT_GT(cbm_store_upsert_node(s, &n2), 0);
    ASSERT_GT(cbm_store_upsert_node(s, &n3), 0);

    ASSERT_EQ(cbm_store_exec(s, "INSERT INTO nodes_fts(nodes_fts) VALUES('delete-all');"),
              CBM_STORE_OK);
    ASSERT_EQ(cbm_store_exec(s,
                             "INSERT INTO nodes_fts(rowid, name, qualified_name, label, "
                             "file_path) "
                             "SELECT id, cbm_camel_split(name), qualified_name, label, file_path "
                             "FROM nodes;"),
              CBM_STORE_OK);

    ASSERT_EQ(sql_scalar_int(db, "SELECT count(*) FROM nodes_fts WHERE nodes_fts MATCH 'cloud';"),
              2);
    ASSERT_EQ(sql_scalar_int(db, "SELECT count(*) FROM nodes_fts WHERE nodes_fts MATCH 'client';"),
              1);
    ASSERT_EQ(sql_scalar_int(db, "SELECT count(*) FROM nodes_fts "
                                 "WHERE nodes_fts MATCH 'updateCloudClient';"),
              1);

    cbm_store_close(s);
    PASS();
}

SUITE(store_compat) {
    RUN_TEST(store_compat_schema_manifest);
    RUN_TEST(store_compat_user_index_manifest_and_drop_symmetry);
    RUN_TEST(store_compat_query_open_is_readonly_and_no_create);
    RUN_TEST(store_compat_path_open_uses_wal_journal_mode);
    RUN_TEST(store_compat_query_open_reads_existing_wal_and_rejects_write);
    RUN_TEST(store_compat_url_path_project_scoped_substring);
    RUN_TEST(store_compat_url_path_generated_column_index_plan);
    RUN_TEST(store_compat_fts_manual_rebuild_and_camel_split);
}
