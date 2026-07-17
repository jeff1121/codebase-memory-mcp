/*
 * test_store_compat.c — Phase 3 Rust 遷移前的 SQLite store contract。
 */
#include "test_framework.h"
#include <foundation/compat.h>
#include <store/store.h>
#include <sqlite3.h>
#include <string.h>
#include <sys/stat.h>

static const char *USER_INDEXES[] = {
    "idx_nodes_label",       "idx_nodes_name",
    "idx_nodes_file",        "idx_edges_source",
    "idx_edges_target",      "idx_edges_type",
    "idx_edges_target_type", "idx_edges_source_type",
    "idx_edges_url_path",    NULL,
};

typedef struct {
    const char *table;
    const char *column;
    const char *type;
    const char *default_sql;
    int ordinal;
    int not_null;
    int pk;
    int hidden;
} expected_column_t;

typedef struct {
    const char *name;
    const char *table;
    const char *columns_csv;
} expected_index_t;

static const expected_column_t STORE_COLUMNS[] = {
    {"projects", "name", "TEXT", "", 0, 0, 1, 0},
    {"projects", "indexed_at", "TEXT", "", 1, 1, 0, 0},
    {"projects", "root_path", "TEXT", "", 2, 1, 0, 0},
    {"file_hashes", "project", "TEXT", "", 0, 1, 1, 0},
    {"file_hashes", "rel_path", "TEXT", "", 1, 1, 2, 0},
    {"file_hashes", "sha256", "TEXT", "", 2, 1, 0, 0},
    {"file_hashes", "mtime_ns", "INTEGER", "0", 3, 1, 0, 0},
    {"file_hashes", "size", "INTEGER", "0", 4, 1, 0, 0},
    {"nodes", "id", "INTEGER", "", 0, 0, 1, 0},
    {"nodes", "project", "TEXT", "", 1, 1, 0, 0},
    {"nodes", "label", "TEXT", "", 2, 1, 0, 0},
    {"nodes", "name", "TEXT", "", 3, 1, 0, 0},
    {"nodes", "qualified_name", "TEXT", "", 4, 1, 0, 0},
    {"nodes", "file_path", "TEXT", "''", 5, 0, 0, 0},
    {"nodes", "start_line", "INTEGER", "0", 6, 0, 0, 0},
    {"nodes", "end_line", "INTEGER", "0", 7, 0, 0, 0},
    {"nodes", "properties", "TEXT", "'{}'", 8, 0, 0, 0},
    {"edges", "id", "INTEGER", "", 0, 0, 1, 0},
    {"edges", "project", "TEXT", "", 1, 1, 0, 0},
    {"edges", "source_id", "INTEGER", "", 2, 1, 0, 0},
    {"edges", "target_id", "INTEGER", "", 3, 1, 0, 0},
    {"edges", "type", "TEXT", "", 4, 1, 0, 0},
    {"edges", "properties", "TEXT", "'{}'", 5, 0, 0, 0},
    {"edges", "url_path_gen", "TEXT", "", 6, 0, 0, 2},
    {"project_summaries", "project", "TEXT", "", 0, 0, 1, 0},
    {"project_summaries", "summary", "TEXT", "", 1, 1, 0, 0},
    {"project_summaries", "source_hash", "TEXT", "", 2, 1, 0, 0},
    {"project_summaries", "created_at", "TEXT", "", 3, 1, 0, 0},
    {"project_summaries", "updated_at", "TEXT", "", 4, 1, 0, 0},
};

static const expected_column_t FTS_COLUMNS[] = {
    {"nodes_fts", "name", "", "", 0, 0, 0, 0},
    {"nodes_fts", "qualified_name", "", "", 1, 0, 0, 0},
    {"nodes_fts", "label", "", "", 2, 0, 0, 0},
    {"nodes_fts", "file_path", "", "", 3, 0, 0, 0},
    {"nodes_fts", "nodes_fts", "", "", 4, 0, 0, 1},
    {"nodes_fts", "rank", "", "", 5, 0, 0, 1},
};

static const expected_index_t USER_INDEX_LAYOUTS[] = {
    {"idx_nodes_label", "nodes", "project,label"},
    {"idx_nodes_name", "nodes", "project,name"},
    {"idx_nodes_file", "nodes", "project,file_path"},
    {"idx_edges_source", "edges", "source_id,type"},
    {"idx_edges_target", "edges", "target_id,type"},
    {"idx_edges_type", "edges", "project,type"},
    {"idx_edges_target_type", "edges", "project,target_id,type"},
    {"idx_edges_source_type", "edges", "project,source_id,type"},
    {"idx_edges_url_path", "edges", "project,url_path_gen"},
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

static int schema_sql_contains(sqlite3 *db, const char *type, const char *name,
                               const char *needle) {
    sqlite3_stmt *stmt = NULL;
    const char *sql = "SELECT sql FROM sqlite_schema WHERE type = ?1 AND name = ?2;";
    int found = 0;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_text(stmt, 1, type, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, name, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *value = sqlite3_column_text(stmt, 0);
        found = value && strstr((const char *)value, needle) ? 1 : 0;
    }
    sqlite3_finalize(stmt);
    return found;
}

static int table_xinfo_column_matches(sqlite3 *db, const expected_column_t *expected) {
    sqlite3_stmt *stmt = NULL;
    const char *sql =
        "SELECT cid, type, \"notnull\", coalesce(dflt_value, ''), pk, hidden "
        "FROM pragma_table_xinfo(?1) WHERE name = ?2;";
    int ok = 0;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_text(stmt, 1, expected->table, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, expected->column, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *type = sqlite3_column_text(stmt, 1);
        const unsigned char *default_sql = sqlite3_column_text(stmt, 3);
        ok = sqlite3_column_int(stmt, 0) == expected->ordinal &&
             type && strcmp((const char *)type, expected->type) == 0 &&
             sqlite3_column_int(stmt, 2) == expected->not_null && default_sql &&
             strcmp((const char *)default_sql, expected->default_sql) == 0 &&
             sqlite3_column_int(stmt, 4) == expected->pk &&
             sqlite3_column_int(stmt, 5) == expected->hidden;
        if (!ok) {
            fprintf(stderr,
                    "table_xinfo mismatch: %s.%s got cid=%d type=%s notnull=%d default=%s "
                    "pk=%d hidden=%d\n",
                    expected->table, expected->column, sqlite3_column_int(stmt, 0),
                    type ? (const char *)type : "(null)", sqlite3_column_int(stmt, 2),
                    default_sql ? (const char *)default_sql : "(null)",
                    sqlite3_column_int(stmt, 4), sqlite3_column_int(stmt, 5));
        }
    }
    sqlite3_finalize(stmt);
    return ok;
}

static int index_xinfo_key_columns_match(sqlite3 *db, const expected_index_t *expected) {
    sqlite3_stmt *stmt = NULL;
    char actual[256] = "";
    const char *table_sql = "SELECT tbl_name FROM sqlite_schema WHERE type = 'index' AND name = ?1;";
    if (sqlite3_prepare_v2(db, table_sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_text(stmt, 1, expected->name, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return 0;
    }
    const unsigned char *table = sqlite3_column_text(stmt, 0);
    int table_ok = table && strcmp((const char *)table, expected->table) == 0;
    sqlite3_finalize(stmt);
    if (!table_ok) {
        return 0;
    }

    const char *index_sql =
        "SELECT name FROM pragma_index_xinfo(?1) WHERE \"key\" = 1 ORDER BY seqno;";
    if (sqlite3_prepare_v2(db, index_sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_text(stmt, 1, expected->name, -1, SQLITE_STATIC);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *name = sqlite3_column_text(stmt, 0);
        if (!name) {
            continue;
        }
        if (actual[0] != '\0') {
            strncat(actual, ",", sizeof(actual) - strlen(actual) - 1);
        }
        strncat(actual, (const char *)name, sizeof(actual) - strlen(actual) - 1);
    }
    sqlite3_finalize(stmt);
    if (strcmp(actual, expected->columns_csv) != 0) {
        fprintf(stderr, "index_xinfo mismatch: %s got %s expected %s\n", expected->name, actual,
                expected->columns_csv);
        return 0;
    }
    return 1;
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

static int sql_scalar_text_bound(sqlite3 *db, const char *sql, const char *value, char *buf,
                                 size_t n) {
    sqlite3_stmt *stmt = NULL;
    if (!buf || n == 0 || sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_text(stmt, 1, value, -1, SQLITE_STATIC);
    int ok = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *text = sqlite3_column_text(stmt, 0);
        snprintf(buf, n, "%s", text ? (const char *)text : "");
        ok = 0;
    }
    sqlite3_finalize(stmt);
    return ok;
}

static void store_compat_db_path(char *buf, size_t n, const char *suffix) {
    snprintf(buf, n, "%s/cbm_store_compat_%d_%s.db", cbm_tmpdir(), (int)getpid(), suffix);
}

TEST(store_compat_dump_to_file_side_effect_contract) {
    char db_path[256];
    char tmp_path[300];
    char wal_path[300];
    char shm_path[300];
    store_compat_db_path(db_path, sizeof(db_path), "dump_side_effect");
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", db_path);
    snprintf(wal_path, sizeof(wal_path), "%s-wal", db_path);
    snprintf(shm_path, sizeof(shm_path), "%s-shm", db_path);
    unlink(db_path);
    unlink(tmp_path);
    unlink(wal_path);
    unlink(shm_path);

    cbm_store_t *memory = cbm_store_open_memory();
    ASSERT_NOT_NULL(memory);
    ASSERT_EQ(cbm_store_upsert_project(memory, "dump-side-effect", "/tmp/dump-side-effect"),
              CBM_STORE_OK);

    cbm_node_t node = {.project = "dump-side-effect",
                       .label = "Function",
                       .name = "HelloDump",
                       .qualified_name = "dump-side-effect.HelloDump",
                       .file_path = "main.go",
                       .start_line = 1,
                       .end_line = 7,
                       .properties_json = "{\"source\":\"contract\"}"};
    ASSERT_GT(cbm_store_upsert_node(memory, &node), 0);
    ASSERT_EQ(cbm_store_dump_to_file(memory, db_path), CBM_STORE_OK);
    cbm_store_close(memory);

    struct stat st;
    ASSERT_TRUE(stat(tmp_path, &st) != 0);

    sqlite3 *db = NULL;
    ASSERT_EQ(sqlite3_open(db_path, &db), SQLITE_OK);
    char mode[32];
    ASSERT_EQ(sql_scalar_text(db, "PRAGMA journal_mode;", mode, sizeof(mode)), 0);
    ASSERT_STR_EQ(mode, "wal");
    sqlite3_close(db);

    cbm_store_t *reader = cbm_store_open_path_query(db_path);
    ASSERT_NOT_NULL(reader);
    cbm_node_t found = {0};
    ASSERT_EQ(cbm_store_find_node_by_qn(reader, "dump-side-effect",
                                        "dump-side-effect.HelloDump", &found),
              CBM_STORE_OK);
    ASSERT_STR_EQ(found.name, "HelloDump");
    cbm_node_free_fields(&found);
    ASSERT_EQ(cbm_store_exec(reader,
                             "INSERT INTO projects(name,indexed_at,root_path) "
                             "VALUES('readonly-dump','now','/tmp/readonly-dump');"),
              CBM_STORE_ERR);
    cbm_store_close(reader);

    unlink(db_path);
    unlink(wal_path);
    unlink(shm_path);
    PASS();
}

TEST(store_compat_delete_nodes_by_file_cascades_edges) {
    cbm_store_t *s = cbm_store_open_memory();
    ASSERT_NOT_NULL(s);
    ASSERT_EQ(cbm_store_upsert_project(s, "cascade-contract", "/tmp/cascade-contract"),
              CBM_STORE_OK);

    cbm_node_t source = {.project = "cascade-contract",
                         .label = "Function",
                         .name = "Source",
                         .qualified_name = "cascade.Source",
                         .file_path = "a.go",
                         .start_line = 1,
                         .end_line = 3,
                         .properties_json = "{}"};
    cbm_node_t target = {.project = "cascade-contract",
                         .label = "Function",
                         .name = "Target",
                         .qualified_name = "cascade.Target",
                         .file_path = "b.go",
                         .start_line = 5,
                         .end_line = 7,
                         .properties_json = "{}"};
    int64_t source_id = cbm_store_upsert_node(s, &source);
    int64_t target_id = cbm_store_upsert_node(s, &target);
    ASSERT_GT(source_id, 0);
    ASSERT_GT(target_id, 0);

    cbm_edge_t outbound = {.project = "cascade-contract",
                           .source_id = source_id,
                           .target_id = target_id,
                           .type = "CALLS",
                           .properties_json = "{}"};
    cbm_edge_t inbound = {.project = "cascade-contract",
                          .source_id = target_id,
                          .target_id = source_id,
                          .type = "USAGE",
                          .properties_json = "{}"};
    ASSERT_GT(cbm_store_insert_edge(s, &outbound), 0);
    ASSERT_GT(cbm_store_insert_edge(s, &inbound), 0);
    ASSERT_EQ(cbm_store_count_edges(s, "cascade-contract"), 2);

    ASSERT_EQ(cbm_store_delete_nodes_by_file(s, "cascade-contract", "a.go"), CBM_STORE_OK);
    ASSERT_EQ(cbm_store_count_nodes(s, "cascade-contract"), 1);
    ASSERT_EQ(cbm_store_count_edges(s, "cascade-contract"), 0);

    cbm_node_t remaining = {0};
    ASSERT_EQ(cbm_store_find_node_by_qn(s, "cascade-contract", "cascade.Target", &remaining),
              CBM_STORE_OK);
    ASSERT_STR_EQ(remaining.file_path, "b.go");
    cbm_node_free_fields(&remaining);

    cbm_store_close(s);
    PASS();
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

TEST(store_compat_table_column_layout_manifest) {
    cbm_store_t *s = cbm_store_open_memory();
    ASSERT_NOT_NULL(s);
    sqlite3 *db = cbm_store_get_db(s);
    ASSERT_NOT_NULL(db);

    for (size_t i = 0; i < sizeof(STORE_COLUMNS) / sizeof(STORE_COLUMNS[0]); i++) {
        ASSERT_EQ(table_xinfo_column_matches(db, &STORE_COLUMNS[i]), 1);
    }
    ASSERT_EQ(schema_sql_contains(db, "table", "edges",
                                  "url_path_gen TEXT GENERATED ALWAYS AS "
                                  "(json_extract(properties,'$.url_path'))"),
              1);

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
    for (size_t i = 0; i < sizeof(USER_INDEX_LAYOUTS) / sizeof(USER_INDEX_LAYOUTS[0]); i++) {
        ASSERT_EQ(index_xinfo_key_columns_match(db, &USER_INDEX_LAYOUTS[i]), 1);
    }

    ASSERT_EQ(cbm_store_drop_indexes(s), CBM_STORE_OK);
    for (int i = 0; USER_INDEXES[i]; i++) {
        ASSERT_EQ(schema_object_exists(db, "index", USER_INDEXES[i]), 0);
    }

    ASSERT_EQ(cbm_store_create_indexes(s), CBM_STORE_OK);
    for (int i = 0; USER_INDEXES[i]; i++) {
        ASSERT_EQ(schema_object_exists(db, "index", USER_INDEXES[i]), 1);
    }
    for (size_t i = 0; i < sizeof(USER_INDEX_LAYOUTS) / sizeof(USER_INDEX_LAYOUTS[0]); i++) {
        ASSERT_EQ(index_xinfo_key_columns_match(db, &USER_INDEX_LAYOUTS[i]), 1);
    }

    cbm_store_close(s);
    PASS();
}

TEST(store_compat_fts_metadata_manifest) {
    cbm_store_t *s = cbm_store_open_memory();
    ASSERT_NOT_NULL(s);
    sqlite3 *db = cbm_store_get_db(s);
    ASSERT_NOT_NULL(db);

    for (size_t i = 0; i < sizeof(FTS_COLUMNS) / sizeof(FTS_COLUMNS[0]); i++) {
        ASSERT_EQ(table_xinfo_column_matches(db, &FTS_COLUMNS[i]), 1);
    }
    ASSERT_EQ(schema_sql_contains(db, "table", "nodes_fts", "USING fts5"), 1);
    ASSERT_EQ(schema_sql_contains(db, "table", "nodes_fts",
                                  "name, qualified_name, label, file_path"),
              1);
    ASSERT_EQ(schema_sql_contains(db, "table", "nodes_fts", "content=''"), 1);
    ASSERT_EQ(schema_sql_contains(db, "table", "nodes_fts",
                                  "tokenize='unicode61 remove_diacritics 2'"),
              1);
    ASSERT_EQ(schema_object_exists(db, "table", "nodes_fts_config"), 1);
    ASSERT_EQ(schema_object_exists(db, "table", "nodes_fts_data"), 1);
    ASSERT_EQ(schema_object_exists(db, "table", "nodes_fts_docsize"), 1);
    ASSERT_EQ(schema_object_exists(db, "table", "nodes_fts_idx"), 1);

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

TEST(store_compat_bulk_pragmas_preserve_wal_contract) {
    char db_path[512];
    make_temp_store_path(db_path, sizeof(db_path), "bulk_pragmas");
    cleanup_store_files(db_path);

    cbm_store_t *s = cbm_store_open_path(db_path);
    ASSERT_NOT_NULL(s);
    sqlite3 *db = cbm_store_get_db(s);
    ASSERT_NOT_NULL(db);

    char mode[32];
    ASSERT_EQ(sql_scalar_text(db, "PRAGMA journal_mode;", mode, sizeof(mode)), 0);
    ASSERT_STR_EQ(mode, "wal");

    ASSERT_EQ(cbm_store_begin_bulk(s), CBM_STORE_OK);
    ASSERT_EQ(sql_scalar_int(db, "PRAGMA synchronous;"), 0);
    ASSERT_EQ(sql_scalar_int(db, "PRAGMA cache_size;"), -65536);
    ASSERT_EQ(sql_scalar_text(db, "PRAGMA journal_mode;", mode, sizeof(mode)), 0);
    ASSERT_STR_EQ(mode, "wal");

    ASSERT_EQ(cbm_store_end_bulk(s), CBM_STORE_OK);
    ASSERT_EQ(sql_scalar_int(db, "PRAGMA synchronous;"), 1);
    ASSERT_EQ(sql_scalar_int(db, "PRAGMA cache_size;"), -2000);
    ASSERT_EQ(sql_scalar_text(db, "PRAGMA journal_mode;", mode, sizeof(mode)), 0);
    ASSERT_STR_EQ(mode, "wal");

    cbm_store_close(s);
    cleanup_store_files(db_path);
    PASS();
}

TEST(store_compat_checkpoint_preserves_wal_size_contract) {
    enum { N_ROWS = 80 };
    char db_path[512];
    char wal_path[560];
    make_temp_store_path(db_path, sizeof(db_path), "checkpoint_wal");
    snprintf(wal_path, sizeof(wal_path), "%s-wal", db_path);
    cleanup_store_files(db_path);

    cbm_store_t *s = cbm_store_open_path(db_path);
    ASSERT_NOT_NULL(s);
    ASSERT_EQ(cbm_store_upsert_project(s, "checkpoint-contract", "/tmp/checkpoint-contract"),
              CBM_STORE_OK);

    for (int i = 0; i < N_ROWS; i++) {
        char sql[256];
        snprintf(sql, sizeof(sql),
                 "INSERT INTO nodes(project, label, name, qualified_name, file_path) "
                 "VALUES('checkpoint-contract', 'Function', 'fn', "
                 "'checkpoint.fn_%d', 'checkpoint.c');",
                 i);
        ASSERT_EQ(cbm_store_exec(s, sql), CBM_STORE_OK);
    }

    struct stat st_before;
    ASSERT_EQ(stat(wal_path, &st_before), 0);
    ASSERT_TRUE(st_before.st_size > 0);

    ASSERT_EQ(cbm_store_checkpoint(s), CBM_STORE_OK);

    struct stat st_after;
    ASSERT_EQ(stat(wal_path, &st_after), 0);
    ASSERT_TRUE(st_after.st_size > 0);

    cbm_store_close(s);
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

TEST(store_compat_camel_split_exact_contract) {
    cbm_store_t *s = cbm_store_open_memory();
    ASSERT_NOT_NULL(s);
    sqlite3 *db = cbm_store_get_db(s);
    ASSERT_NOT_NULL(db);

    char out[4096];
    ASSERT_EQ(sql_scalar_text(db, "SELECT cbm_camel_split(NULL);", out, sizeof(out)), 0);
    ASSERT_STR_EQ(out, "");
    ASSERT_EQ(sql_scalar_text(db, "SELECT cbm_camel_split('');", out, sizeof(out)), 0);
    ASSERT_STR_EQ(out, "");
    ASSERT_EQ(sql_scalar_text(db, "SELECT cbm_camel_split('updateCloudClient');", out,
                              sizeof(out)),
              0);
    ASSERT_STR_EQ(out, "updateCloudClient update Cloud Client");
    ASSERT_EQ(sql_scalar_text(db, "SELECT cbm_camel_split('XMLParser');", out, sizeof(out)), 0);
    ASSERT_STR_EQ(out, "XMLParser XML Parser");
    ASSERT_EQ(sql_scalar_text(db, "SELECT cbm_camel_split('snake_case');", out, sizeof(out)), 0);
    ASSERT_STR_EQ(out, "snake_case snake_case");
    ASSERT_EQ(sql_scalar_text_bound(db, "SELECT cbm_camel_split(?1);", "caf\303\251HTTP", out,
                                    sizeof(out)),
              0);
    ASSERT_STR_EQ(out, "caf\303\251HTTP caf\303\251HTTP");

    char fallback[2048];
    memset(fallback, 'a', sizeof(fallback) - 1);
    fallback[sizeof(fallback) - 1] = '\0';
    ASSERT_EQ(sql_scalar_text_bound(db, "SELECT cbm_camel_split(?1);", fallback, out, sizeof(out)),
              0);
    ASSERT_STR_EQ(out, fallback);

    char near_limit[2047];
    memset(near_limit, 'a', sizeof(near_limit) - 1);
    near_limit[sizeof(near_limit) - 1] = '\0';
    ASSERT_EQ(
        sql_scalar_text_bound(db, "SELECT cbm_camel_split(?1);", near_limit, out, sizeof(out)),
        0);
    ASSERT_EQ(strlen(out), sizeof(near_limit));
    ASSERT_EQ(out[sizeof(near_limit) - 1], ' ');
    ASSERT_EQ(out[sizeof(near_limit)], '\0');

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
    RUN_TEST(store_compat_table_column_layout_manifest);
    RUN_TEST(store_compat_user_index_manifest_and_drop_symmetry);
    RUN_TEST(store_compat_fts_metadata_manifest);
    RUN_TEST(store_compat_query_open_is_readonly_and_no_create);
    RUN_TEST(store_compat_path_open_uses_wal_journal_mode);
    RUN_TEST(store_compat_query_open_reads_existing_wal_and_rejects_write);
    RUN_TEST(store_compat_bulk_pragmas_preserve_wal_contract);
    RUN_TEST(store_compat_checkpoint_preserves_wal_size_contract);
    RUN_TEST(store_compat_dump_to_file_side_effect_contract);
    RUN_TEST(store_compat_delete_nodes_by_file_cascades_edges);
    RUN_TEST(store_compat_url_path_project_scoped_substring);
    RUN_TEST(store_compat_url_path_generated_column_index_plan);
    RUN_TEST(store_compat_camel_split_exact_contract);
    RUN_TEST(store_compat_fts_manual_rebuild_and_camel_split);
}
