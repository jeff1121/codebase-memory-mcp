//! C store 的 SQLite schema/index manifest contract。
//!
//! 此模組刻意維持 static 且無副作用；它不開啟 SQLite，也不取代任何 production
//! `src/store` 行為。

pub const KIND_CORE_TABLE: i32 = 1;
pub const KIND_FTS_TABLE: i32 = 2;
pub const KIND_USER_INDEX: i32 = 3;
pub const KIND_GENERATED_COLUMN: i32 = 4;
pub const KIND_TABLE_COLUMN: i32 = 5;
pub const KIND_FTS_SHADOW_TABLE: i32 = 6;

pub const FLAG_REQUIRED: u32 = 1 << 0;
pub const FLAG_DROPPABLE_USER_INDEX: u32 = 1 << 1;
pub const FLAG_GENERATED: u32 = 1 << 2;
pub const FLAG_FTS: u32 = 1 << 3;
pub const FLAG_NOT_NULL: u32 = 1 << 4;
pub const FLAG_PRIMARY_KEY: u32 = 1 << 5;
pub const FLAG_UNIQUE: u32 = 1 << 6;
pub const FLAG_AUTOINCREMENT: u32 = 1 << 7;
pub const FLAG_FK_CASCADE: u32 = 1 << 8;

pub const CONNECTION_KIND_OPEN_MODE: i32 = 1;
pub const CONNECTION_KIND_PRAGMA: i32 = 2;
pub const CONNECTION_KIND_READ_PROBE: i32 = 3;
pub const CONNECTION_KIND_FALLBACK: i32 = 4;

pub const CONNECTION_MODE_MEMORY: u32 = 1 << 0;
pub const CONNECTION_MODE_WRITE: u32 = 1 << 1;
pub const CONNECTION_MODE_READ_ONLY: u32 = 1 << 2;
pub const CONNECTION_MODE_BULK_BEGIN: u32 = 1 << 3;
pub const CONNECTION_MODE_BULK_END: u32 = 1 << 4;
pub const CONNECTION_MODE_CHECKPOINT: u32 = 1 << 5;

pub const CONNECTION_FLAG_REQUIRED: u32 = 1 << 0;
pub const CONNECTION_FLAG_NO_CREATE: u32 = 1 << 1;
pub const CONNECTION_FLAG_READ_ONLY: u32 = 1 << 2;
pub const CONNECTION_FLAG_URI: u32 = 1 << 3;
pub const CONNECTION_FLAG_WRITES_DB_OR_WAL: u32 = 1 << 4;
pub const CONNECTION_FLAG_BEST_EFFORT: u32 = 1 << 5;
pub const CONNECTION_FLAG_ENV_BACKED: u32 = 1 << 6;

const EMPTY: &[u8] = b"\0";

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct ManifestEntry {
    pub kind: i32,
    pub flags: u32,
    pub name: &'static [u8],
    pub table_name: &'static [u8],
    pub column_name: &'static [u8],
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct ManifestEntryV2 {
    pub kind: i32,
    pub flags: u32,
    pub ordinal: i32,
    pub hidden: i32,
    pub name: &'static [u8],
    pub table_name: &'static [u8],
    pub column_name: &'static [u8],
    pub column_type: &'static [u8],
    pub default_sql: &'static [u8],
    pub columns_csv: &'static [u8],
    pub sql_fragment: &'static [u8],
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct ConnectionManifestEntryV1 {
    pub kind: i32,
    pub flags: u32,
    pub mode_mask: u32,
    pub ordinal: i32,
    pub value_i64: i64,
    pub name: &'static [u8],
    pub sql: &'static [u8],
    pub env_name: &'static [u8],
}

const fn core_table(name: &'static [u8]) -> ManifestEntry {
    ManifestEntry {
        kind: KIND_CORE_TABLE,
        flags: FLAG_REQUIRED,
        name,
        table_name: EMPTY,
        column_name: EMPTY,
    }
}

const fn fts_table(name: &'static [u8]) -> ManifestEntry {
    ManifestEntry {
        kind: KIND_FTS_TABLE,
        flags: FLAG_REQUIRED | FLAG_FTS,
        name,
        table_name: EMPTY,
        column_name: EMPTY,
    }
}

const fn user_index(name: &'static [u8], table_name: &'static [u8]) -> ManifestEntry {
    ManifestEntry {
        kind: KIND_USER_INDEX,
        flags: FLAG_REQUIRED | FLAG_DROPPABLE_USER_INDEX,
        name,
        table_name,
        column_name: EMPTY,
    }
}

const fn generated_column(
    name: &'static [u8],
    table_name: &'static [u8],
    column_name: &'static [u8],
) -> ManifestEntry {
    ManifestEntry {
        kind: KIND_GENERATED_COLUMN,
        flags: FLAG_REQUIRED | FLAG_GENERATED,
        name,
        table_name,
        column_name,
    }
}

const fn core_table_v2(
    name: &'static [u8],
    columns_csv: &'static [u8],
    sql_fragment: &'static [u8],
) -> ManifestEntryV2 {
    ManifestEntryV2 {
        kind: KIND_CORE_TABLE,
        flags: FLAG_REQUIRED,
        ordinal: -1,
        hidden: 0,
        name,
        table_name: EMPTY,
        column_name: EMPTY,
        column_type: EMPTY,
        default_sql: EMPTY,
        columns_csv,
        sql_fragment,
    }
}

const fn fts_table_v2(
    name: &'static [u8],
    columns_csv: &'static [u8],
    sql_fragment: &'static [u8],
) -> ManifestEntryV2 {
    ManifestEntryV2 {
        kind: KIND_FTS_TABLE,
        flags: FLAG_REQUIRED | FLAG_FTS,
        ordinal: -1,
        hidden: 0,
        name,
        table_name: EMPTY,
        column_name: EMPTY,
        column_type: EMPTY,
        default_sql: EMPTY,
        columns_csv,
        sql_fragment,
    }
}

const fn fts_shadow_table(name: &'static [u8], table_name: &'static [u8]) -> ManifestEntryV2 {
    ManifestEntryV2 {
        kind: KIND_FTS_SHADOW_TABLE,
        flags: FLAG_REQUIRED | FLAG_FTS,
        ordinal: -1,
        hidden: 0,
        name,
        table_name,
        column_name: EMPTY,
        column_type: EMPTY,
        default_sql: EMPTY,
        columns_csv: EMPTY,
        sql_fragment: EMPTY,
    }
}

const fn table_column(
    name: &'static [u8],
    table_name: &'static [u8],
    column_name: &'static [u8],
    column_type: &'static [u8],
    ordinal: i32,
    flags: u32,
) -> ManifestEntryV2 {
    table_column_default(
        name,
        table_name,
        column_name,
        column_type,
        EMPTY,
        ordinal,
        flags,
    )
}

const fn table_column_default(
    name: &'static [u8],
    table_name: &'static [u8],
    column_name: &'static [u8],
    column_type: &'static [u8],
    default_sql: &'static [u8],
    ordinal: i32,
    flags: u32,
) -> ManifestEntryV2 {
    ManifestEntryV2 {
        kind: KIND_TABLE_COLUMN,
        flags: FLAG_REQUIRED | flags,
        ordinal,
        hidden: 0,
        name,
        table_name,
        column_name,
        column_type,
        default_sql,
        columns_csv: EMPTY,
        sql_fragment: EMPTY,
    }
}

const fn generated_column_v2(
    name: &'static [u8],
    table_name: &'static [u8],
    column_name: &'static [u8],
    column_type: &'static [u8],
    ordinal: i32,
    hidden: i32,
    expression: &'static [u8],
) -> ManifestEntryV2 {
    ManifestEntryV2 {
        kind: KIND_GENERATED_COLUMN,
        flags: FLAG_REQUIRED | FLAG_GENERATED,
        ordinal,
        hidden,
        name,
        table_name,
        column_name,
        column_type,
        default_sql: EMPTY,
        columns_csv: EMPTY,
        sql_fragment: expression,
    }
}

const fn user_index_v2(
    name: &'static [u8],
    table_name: &'static [u8],
    columns_csv: &'static [u8],
) -> ManifestEntryV2 {
    ManifestEntryV2 {
        kind: KIND_USER_INDEX,
        flags: FLAG_REQUIRED | FLAG_DROPPABLE_USER_INDEX,
        ordinal: -1,
        hidden: 0,
        name,
        table_name,
        column_name: EMPTY,
        column_type: EMPTY,
        default_sql: EMPTY,
        columns_csv,
        sql_fragment: EMPTY,
    }
}

const fn connection_entry(
    kind: i32,
    flags: u32,
    mode_mask: u32,
    ordinal: i32,
    value_i64: i64,
    name: &'static [u8],
    sql_and_env: (&'static [u8], &'static [u8]),
) -> ConnectionManifestEntryV1 {
    ConnectionManifestEntryV1 {
        kind,
        flags,
        mode_mask,
        ordinal,
        value_i64,
        name,
        sql: sql_and_env.0,
        env_name: sql_and_env.1,
    }
}

pub static MANIFEST: &[ManifestEntry] = &[
    core_table(b"projects\0"),
    core_table(b"file_hashes\0"),
    core_table(b"nodes\0"),
    core_table(b"edges\0"),
    core_table(b"project_summaries\0"),
    fts_table(b"nodes_fts\0"),
    generated_column(b"edges.url_path_gen\0", b"edges\0", b"url_path_gen\0"),
    user_index(b"idx_nodes_label\0", b"nodes\0"),
    user_index(b"idx_nodes_name\0", b"nodes\0"),
    user_index(b"idx_nodes_file\0", b"nodes\0"),
    user_index(b"idx_edges_source\0", b"edges\0"),
    user_index(b"idx_edges_target\0", b"edges\0"),
    user_index(b"idx_edges_type\0", b"edges\0"),
    user_index(b"idx_edges_target_type\0", b"edges\0"),
    user_index(b"idx_edges_source_type\0", b"edges\0"),
    user_index(b"idx_edges_url_path\0", b"edges\0"),
];

pub static MANIFEST_V2: &[ManifestEntryV2] = &[
    core_table_v2(
        b"projects\0",
        b"name,indexed_at,root_path\0",
        b"PRIMARY KEY(name)\0",
    ),
    core_table_v2(
        b"file_hashes\0",
        b"project,rel_path,sha256,mtime_ns,size\0",
        b"PRIMARY KEY(project,rel_path);FOREIGN KEY(project) REFERENCES projects(name) ON DELETE CASCADE\0",
    ),
    core_table_v2(
        b"nodes\0",
        b"id,project,label,name,qualified_name,file_path,start_line,end_line,properties\0",
        b"UNIQUE(project,qualified_name);FOREIGN KEY(project) REFERENCES projects(name) ON DELETE CASCADE\0",
    ),
    core_table_v2(
        b"edges\0",
        b"id,project,source_id,target_id,type,properties,url_path_gen\0",
        b"UNIQUE(source_id,target_id,type);FOREIGN KEY(project) REFERENCES projects(name) ON DELETE CASCADE\0",
    ),
    core_table_v2(
        b"project_summaries\0",
        b"project,summary,source_hash,created_at,updated_at\0",
        b"PRIMARY KEY(project)\0",
    ),
    fts_table_v2(
        b"nodes_fts\0",
        b"name,qualified_name,label,file_path\0",
        b"content='';tokenize='unicode61 remove_diacritics 2'\0",
    ),
    fts_shadow_table(b"nodes_fts_config\0", b"nodes_fts\0"),
    fts_shadow_table(b"nodes_fts_data\0", b"nodes_fts\0"),
    fts_shadow_table(b"nodes_fts_docsize\0", b"nodes_fts\0"),
    fts_shadow_table(b"nodes_fts_idx\0", b"nodes_fts\0"),
    table_column(
        b"projects.name\0",
        b"projects\0",
        b"name\0",
        b"TEXT\0",
        0,
        FLAG_PRIMARY_KEY,
    ),
    table_column(
        b"projects.indexed_at\0",
        b"projects\0",
        b"indexed_at\0",
        b"TEXT\0",
        1,
        FLAG_NOT_NULL,
    ),
    table_column(
        b"projects.root_path\0",
        b"projects\0",
        b"root_path\0",
        b"TEXT\0",
        2,
        FLAG_NOT_NULL,
    ),
    table_column(
        b"file_hashes.project\0",
        b"file_hashes\0",
        b"project\0",
        b"TEXT\0",
        0,
        FLAG_NOT_NULL | FLAG_PRIMARY_KEY | FLAG_FK_CASCADE,
    ),
    table_column(
        b"file_hashes.rel_path\0",
        b"file_hashes\0",
        b"rel_path\0",
        b"TEXT\0",
        1,
        FLAG_NOT_NULL | FLAG_PRIMARY_KEY,
    ),
    table_column(
        b"file_hashes.sha256\0",
        b"file_hashes\0",
        b"sha256\0",
        b"TEXT\0",
        2,
        FLAG_NOT_NULL,
    ),
    table_column_default(
        b"file_hashes.mtime_ns\0",
        b"file_hashes\0",
        b"mtime_ns\0",
        b"INTEGER\0",
        b"0\0",
        3,
        FLAG_NOT_NULL,
    ),
    table_column_default(
        b"file_hashes.size\0",
        b"file_hashes\0",
        b"size\0",
        b"INTEGER\0",
        b"0\0",
        4,
        FLAG_NOT_NULL,
    ),
    table_column(
        b"nodes.id\0",
        b"nodes\0",
        b"id\0",
        b"INTEGER\0",
        0,
        FLAG_PRIMARY_KEY | FLAG_AUTOINCREMENT,
    ),
    table_column(
        b"nodes.project\0",
        b"nodes\0",
        b"project\0",
        b"TEXT\0",
        1,
        FLAG_NOT_NULL | FLAG_FK_CASCADE,
    ),
    table_column(
        b"nodes.label\0",
        b"nodes\0",
        b"label\0",
        b"TEXT\0",
        2,
        FLAG_NOT_NULL,
    ),
    table_column(
        b"nodes.name\0",
        b"nodes\0",
        b"name\0",
        b"TEXT\0",
        3,
        FLAG_NOT_NULL,
    ),
    table_column(
        b"nodes.qualified_name\0",
        b"nodes\0",
        b"qualified_name\0",
        b"TEXT\0",
        4,
        FLAG_NOT_NULL | FLAG_UNIQUE,
    ),
    table_column_default(
        b"nodes.file_path\0",
        b"nodes\0",
        b"file_path\0",
        b"TEXT\0",
        b"''\0",
        5,
        0,
    ),
    table_column_default(
        b"nodes.start_line\0",
        b"nodes\0",
        b"start_line\0",
        b"INTEGER\0",
        b"0\0",
        6,
        0,
    ),
    table_column_default(
        b"nodes.end_line\0",
        b"nodes\0",
        b"end_line\0",
        b"INTEGER\0",
        b"0\0",
        7,
        0,
    ),
    table_column_default(
        b"nodes.properties\0",
        b"nodes\0",
        b"properties\0",
        b"TEXT\0",
        b"'{}'\0",
        8,
        0,
    ),
    table_column(
        b"edges.id\0",
        b"edges\0",
        b"id\0",
        b"INTEGER\0",
        0,
        FLAG_PRIMARY_KEY | FLAG_AUTOINCREMENT,
    ),
    table_column(
        b"edges.project\0",
        b"edges\0",
        b"project\0",
        b"TEXT\0",
        1,
        FLAG_NOT_NULL | FLAG_FK_CASCADE,
    ),
    table_column(
        b"edges.source_id\0",
        b"edges\0",
        b"source_id\0",
        b"INTEGER\0",
        2,
        FLAG_NOT_NULL | FLAG_FK_CASCADE,
    ),
    table_column(
        b"edges.target_id\0",
        b"edges\0",
        b"target_id\0",
        b"INTEGER\0",
        3,
        FLAG_NOT_NULL | FLAG_FK_CASCADE,
    ),
    table_column(
        b"edges.type\0",
        b"edges\0",
        b"type\0",
        b"TEXT\0",
        4,
        FLAG_NOT_NULL,
    ),
    table_column_default(
        b"edges.properties\0",
        b"edges\0",
        b"properties\0",
        b"TEXT\0",
        b"'{}'\0",
        5,
        0,
    ),
    generated_column_v2(
        b"edges.url_path_gen\0",
        b"edges\0",
        b"url_path_gen\0",
        b"TEXT\0",
        6,
        2,
        b"json_extract(properties,'$.url_path')\0",
    ),
    table_column(
        b"project_summaries.project\0",
        b"project_summaries\0",
        b"project\0",
        b"TEXT\0",
        0,
        FLAG_PRIMARY_KEY,
    ),
    table_column(
        b"project_summaries.summary\0",
        b"project_summaries\0",
        b"summary\0",
        b"TEXT\0",
        1,
        FLAG_NOT_NULL,
    ),
    table_column(
        b"project_summaries.source_hash\0",
        b"project_summaries\0",
        b"source_hash\0",
        b"TEXT\0",
        2,
        FLAG_NOT_NULL,
    ),
    table_column(
        b"project_summaries.created_at\0",
        b"project_summaries\0",
        b"created_at\0",
        b"TEXT\0",
        3,
        FLAG_NOT_NULL,
    ),
    table_column(
        b"project_summaries.updated_at\0",
        b"project_summaries\0",
        b"updated_at\0",
        b"TEXT\0",
        4,
        FLAG_NOT_NULL,
    ),
    user_index_v2(b"idx_nodes_label\0", b"nodes\0", b"project,label\0"),
    user_index_v2(b"idx_nodes_name\0", b"nodes\0", b"project,name\0"),
    user_index_v2(b"idx_nodes_file\0", b"nodes\0", b"project,file_path\0"),
    user_index_v2(b"idx_edges_source\0", b"edges\0", b"source_id,type\0"),
    user_index_v2(b"idx_edges_target\0", b"edges\0", b"target_id,type\0"),
    user_index_v2(b"idx_edges_type\0", b"edges\0", b"project,type\0"),
    user_index_v2(
        b"idx_edges_target_type\0",
        b"edges\0",
        b"project,target_id,type\0",
    ),
    user_index_v2(
        b"idx_edges_source_type\0",
        b"edges\0",
        b"project,source_id,type\0",
    ),
    user_index_v2(
        b"idx_edges_url_path\0",
        b"edges\0",
        b"project,url_path_gen\0",
    ),
];

pub static CONNECTION_MANIFEST_V1: &[ConnectionManifestEntryV1] = &[
    connection_entry(
        CONNECTION_KIND_OPEN_MODE,
        CONNECTION_FLAG_REQUIRED,
        CONNECTION_MODE_MEMORY,
        0,
        0,
        b"open.memory\0",
        (b":memory:\0", EMPTY),
    ),
    connection_entry(
        CONNECTION_KIND_OPEN_MODE,
        CONNECTION_FLAG_REQUIRED,
        CONNECTION_MODE_WRITE,
        0,
        0,
        b"open.path_write\0",
        (b"sqlite3_open\0", EMPTY),
    ),
    connection_entry(
        CONNECTION_KIND_OPEN_MODE,
        CONNECTION_FLAG_REQUIRED | CONNECTION_FLAG_READ_ONLY | CONNECTION_FLAG_NO_CREATE,
        CONNECTION_MODE_READ_ONLY,
        0,
        0,
        b"open.path_query\0",
        (b"SQLITE_OPEN_READONLY\0", EMPTY),
    ),
    connection_entry(
        CONNECTION_KIND_READ_PROBE,
        CONNECTION_FLAG_REQUIRED | CONNECTION_FLAG_READ_ONLY,
        CONNECTION_MODE_READ_ONLY,
        1,
        0,
        b"open.path_query.probe\0",
        (b"SELECT 1 FROM sqlite_master LIMIT 1;\0", EMPTY),
    ),
    connection_entry(
        CONNECTION_KIND_FALLBACK,
        CONNECTION_FLAG_REQUIRED
            | CONNECTION_FLAG_READ_ONLY
            | CONNECTION_FLAG_NO_CREATE
            | CONNECTION_FLAG_URI,
        CONNECTION_MODE_READ_ONLY,
        2,
        0,
        b"open.path_query.immutable_fallback\0",
        (b"SQLITE_OPEN_READONLY|SQLITE_OPEN_URI;immutable=1\0", EMPTY),
    ),
    connection_entry(
        CONNECTION_KIND_PRAGMA,
        CONNECTION_FLAG_REQUIRED,
        CONNECTION_MODE_MEMORY | CONNECTION_MODE_WRITE | CONNECTION_MODE_READ_ONLY,
        10,
        0,
        b"pragma.foreign_keys\0",
        (b"PRAGMA foreign_keys = ON;\0", EMPTY),
    ),
    connection_entry(
        CONNECTION_KIND_PRAGMA,
        CONNECTION_FLAG_REQUIRED,
        CONNECTION_MODE_MEMORY | CONNECTION_MODE_WRITE | CONNECTION_MODE_READ_ONLY,
        20,
        0,
        b"pragma.temp_store\0",
        (b"PRAGMA temp_store = MEMORY;\0", EMPTY),
    ),
    connection_entry(
        CONNECTION_KIND_PRAGMA,
        CONNECTION_FLAG_REQUIRED | CONNECTION_FLAG_WRITES_DB_OR_WAL,
        CONNECTION_MODE_MEMORY | CONNECTION_MODE_BULK_BEGIN,
        30,
        0,
        b"pragma.synchronous.off\0",
        (b"PRAGMA synchronous = OFF;\0", EMPTY),
    ),
    connection_entry(
        CONNECTION_KIND_PRAGMA,
        CONNECTION_FLAG_REQUIRED,
        CONNECTION_MODE_WRITE | CONNECTION_MODE_READ_ONLY,
        30,
        10000,
        b"pragma.busy_timeout\0",
        (b"PRAGMA busy_timeout = 10000;\0", EMPTY),
    ),
    connection_entry(
        CONNECTION_KIND_PRAGMA,
        CONNECTION_FLAG_REQUIRED | CONNECTION_FLAG_WRITES_DB_OR_WAL,
        CONNECTION_MODE_WRITE,
        40,
        0,
        b"pragma.journal_mode.wal\0",
        (b"PRAGMA journal_mode = WAL;\0", EMPTY),
    ),
    connection_entry(
        CONNECTION_KIND_PRAGMA,
        CONNECTION_FLAG_REQUIRED | CONNECTION_FLAG_WRITES_DB_OR_WAL | CONNECTION_FLAG_BEST_EFFORT,
        CONNECTION_MODE_WRITE,
        50,
        0,
        b"pragma.wal_checkpoint.passive\0",
        (b"PRAGMA wal_checkpoint(PASSIVE)\0", EMPTY),
    ),
    connection_entry(
        CONNECTION_KIND_PRAGMA,
        CONNECTION_FLAG_REQUIRED | CONNECTION_FLAG_WRITES_DB_OR_WAL,
        CONNECTION_MODE_WRITE | CONNECTION_MODE_BULK_END,
        60,
        0,
        b"pragma.synchronous.normal\0",
        (b"PRAGMA synchronous = NORMAL;\0", EMPTY),
    ),
    connection_entry(
        CONNECTION_KIND_PRAGMA,
        CONNECTION_FLAG_REQUIRED | CONNECTION_FLAG_ENV_BACKED,
        CONNECTION_MODE_WRITE | CONNECTION_MODE_READ_ONLY,
        70,
        67_108_864,
        b"pragma.mmap_size\0",
        (
            b"PRAGMA mmap_size = <resolved>;\0",
            b"CBM_SQLITE_MMAP_SIZE\0",
        ),
    ),
    connection_entry(
        CONNECTION_KIND_PRAGMA,
        CONNECTION_FLAG_REQUIRED,
        CONNECTION_MODE_BULK_BEGIN,
        80,
        -65_536,
        b"pragma.bulk.cache_size.begin\0",
        (b"PRAGMA cache_size = -65536;\0", EMPTY),
    ),
    connection_entry(
        CONNECTION_KIND_PRAGMA,
        CONNECTION_FLAG_REQUIRED,
        CONNECTION_MODE_BULK_END,
        90,
        -2_000,
        b"pragma.bulk.cache_size.end\0",
        (b"PRAGMA cache_size = -2000;\0", EMPTY),
    ),
    connection_entry(
        CONNECTION_KIND_PRAGMA,
        CONNECTION_FLAG_REQUIRED | CONNECTION_FLAG_BEST_EFFORT,
        CONNECTION_MODE_WRITE,
        100,
        0,
        b"pragma.optimize\0",
        (b"PRAGMA optimize;\0", EMPTY),
    ),
    connection_entry(
        CONNECTION_KIND_PRAGMA,
        CONNECTION_FLAG_REQUIRED | CONNECTION_FLAG_WRITES_DB_OR_WAL,
        CONNECTION_MODE_CHECKPOINT,
        10,
        0,
        b"checkpoint.passive.api\0",
        (b"SQLITE_CHECKPOINT_PASSIVE\0", EMPTY),
    ),
    connection_entry(
        CONNECTION_KIND_PRAGMA,
        CONNECTION_FLAG_REQUIRED,
        CONNECTION_MODE_CHECKPOINT,
        20,
        0,
        b"checkpoint.optimize\0",
        (b"PRAGMA optimize;\0", EMPTY),
    ),
];

pub fn entries() -> &'static [ManifestEntry] {
    MANIFEST
}

pub fn entries_v2() -> &'static [ManifestEntryV2] {
    MANIFEST_V2
}

pub fn user_index_count() -> usize {
    MANIFEST
        .iter()
        .filter(|entry| entry.kind == KIND_USER_INDEX)
        .count()
}

pub fn table_column_count_v2() -> usize {
    MANIFEST_V2
        .iter()
        .filter(|entry| entry.kind == KIND_TABLE_COLUMN || entry.kind == KIND_GENERATED_COLUMN)
        .count()
}

pub fn fts_shadow_table_count_v2() -> usize {
    MANIFEST_V2
        .iter()
        .filter(|entry| entry.kind == KIND_FTS_SHADOW_TABLE)
        .count()
}

pub fn connection_entries_v1() -> &'static [ConnectionManifestEntryV1] {
    CONNECTION_MANIFEST_V1
}

pub fn connection_write_pragma_count_v1() -> usize {
    CONNECTION_MANIFEST_V1
        .iter()
        .filter(|entry| {
            entry.kind == CONNECTION_KIND_PRAGMA
                && (entry.flags & CONNECTION_FLAG_WRITES_DB_OR_WAL) != 0
        })
        .count()
}

pub fn connection_entries_for_mode_v1(
    mode_mask: u32,
) -> impl Iterator<Item = &'static ConnectionManifestEntryV1> {
    CONNECTION_MANIFEST_V1
        .iter()
        .filter(move |entry| (entry.mode_mask & mode_mask) != 0)
}

pub fn connection_entry_count_for_mode_v1(mode_mask: u32) -> usize {
    connection_entries_for_mode_v1(mode_mask).count()
}

pub fn connection_write_entry_count_for_mode_v1(mode_mask: u32) -> usize {
    connection_entries_for_mode_v1(mode_mask)
        .filter(|entry| (entry.flags & CONNECTION_FLAG_WRITES_DB_OR_WAL) != 0)
        .count()
}

#[cfg(test)]
mod tests {
    use super::*;

    fn text(bytes: &[u8]) -> &str {
        std::str::from_utf8(bytes.strip_suffix(b"\0").unwrap()).unwrap()
    }

    #[test]
    fn manifest_contains_store_compat_contract_objects() {
        let names: Vec<&str> = entries().iter().map(|entry| text(entry.name)).collect();
        assert!(names.contains(&"projects"));
        assert!(names.contains(&"file_hashes"));
        assert!(names.contains(&"nodes"));
        assert!(names.contains(&"edges"));
        assert!(names.contains(&"project_summaries"));
        assert!(names.contains(&"nodes_fts"));
        assert!(names.contains(&"edges.url_path_gen"));
        assert!(names.contains(&"idx_edges_url_path"));
        assert_eq!(user_index_count(), 9);
    }

    #[test]
    fn manifest_strings_are_nul_terminated() {
        for entry in entries() {
            assert_eq!(entry.name.last(), Some(&0));
            assert_eq!(entry.table_name.last(), Some(&0));
            assert_eq!(entry.column_name.last(), Some(&0));
        }
        for entry in entries_v2() {
            assert_eq!(entry.name.last(), Some(&0));
            assert_eq!(entry.table_name.last(), Some(&0));
            assert_eq!(entry.column_name.last(), Some(&0));
            assert_eq!(entry.column_type.last(), Some(&0));
            assert_eq!(entry.default_sql.last(), Some(&0));
            assert_eq!(entry.columns_csv.last(), Some(&0));
            assert_eq!(entry.sql_fragment.last(), Some(&0));
        }
    }

    #[test]
    fn manifest_v2_captures_store_schema_layout() {
        assert_eq!(entries_v2().len(), 48);
        assert_eq!(table_column_count_v2(), 29);
        assert_eq!(fts_shadow_table_count_v2(), 4);

        let edges_url = entries_v2()
            .iter()
            .find(|entry| text(entry.name) == "edges.url_path_gen")
            .unwrap();
        assert_eq!(edges_url.kind, KIND_GENERATED_COLUMN);
        assert_eq!(edges_url.ordinal, 6);
        assert_eq!(edges_url.hidden, 2);
        assert_eq!(
            text(edges_url.sql_fragment),
            "json_extract(properties,'$.url_path')"
        );

        let idx = entries_v2()
            .iter()
            .find(|entry| text(entry.name) == "idx_edges_url_path")
            .unwrap();
        assert_eq!(idx.kind, KIND_USER_INDEX);
        assert_eq!(text(idx.columns_csv), "project,url_path_gen");

        let fts = entries_v2()
            .iter()
            .find(|entry| text(entry.name) == "nodes_fts")
            .unwrap();
        assert_eq!(text(fts.columns_csv), "name,qualified_name,label,file_path");
        assert_eq!(
            text(fts.sql_fragment),
            "content='';tokenize='unicode61 remove_diacritics 2'"
        );
    }

    #[test]
    fn connection_manifest_captures_open_and_pragma_contract() {
        assert_eq!(connection_entries_v1().len(), 18);
        assert_eq!(connection_write_pragma_count_v1(), 5);

        let readonly = connection_entries_v1()
            .iter()
            .find(|entry| text(entry.name) == "open.path_query")
            .unwrap();
        assert_eq!(readonly.kind, CONNECTION_KIND_OPEN_MODE);
        assert_eq!(readonly.mode_mask, CONNECTION_MODE_READ_ONLY);
        assert_eq!(
            readonly.flags,
            CONNECTION_FLAG_REQUIRED | CONNECTION_FLAG_READ_ONLY | CONNECTION_FLAG_NO_CREATE
        );
        assert_eq!(text(readonly.sql), "SQLITE_OPEN_READONLY");

        let wal = connection_entries_v1()
            .iter()
            .find(|entry| text(entry.name) == "pragma.journal_mode.wal")
            .unwrap();
        assert_eq!(wal.mode_mask, CONNECTION_MODE_WRITE);
        assert_ne!(wal.flags & CONNECTION_FLAG_WRITES_DB_OR_WAL, 0);

        let mmap = connection_entries_v1()
            .iter()
            .find(|entry| text(entry.name) == "pragma.mmap_size")
            .unwrap();
        assert_eq!(mmap.value_i64, 67_108_864);
        assert_eq!(text(mmap.env_name), "CBM_SQLITE_MMAP_SIZE");
        assert_eq!(
            mmap.mode_mask,
            CONNECTION_MODE_WRITE | CONNECTION_MODE_READ_ONLY
        );

        let bulk = connection_entries_v1()
            .iter()
            .find(|entry| text(entry.name) == "pragma.bulk.cache_size.begin")
            .unwrap();
        assert_eq!(bulk.value_i64, -65_536);
        assert_eq!(bulk.mode_mask, CONNECTION_MODE_BULK_BEGIN);
    }

    #[test]
    fn connection_manifest_filters_by_operation_mode() {
        assert_eq!(
            connection_entry_count_for_mode_v1(CONNECTION_MODE_MEMORY),
            4
        );
        assert_eq!(
            connection_entry_count_for_mode_v1(CONNECTION_MODE_READ_ONLY),
            7
        );
        assert_eq!(
            connection_write_entry_count_for_mode_v1(CONNECTION_MODE_READ_ONLY),
            0
        );
        assert_eq!(
            connection_entry_count_for_mode_v1(CONNECTION_MODE_BULK_BEGIN),
            2
        );
        assert_eq!(
            connection_entry_count_for_mode_v1(CONNECTION_MODE_BULK_END),
            2
        );
        assert_eq!(
            connection_entry_count_for_mode_v1(CONNECTION_MODE_CHECKPOINT),
            2
        );
        assert_eq!(
            connection_write_entry_count_for_mode_v1(CONNECTION_MODE_CHECKPOINT),
            1
        );

        let checkpoint: Vec<&str> = connection_entries_for_mode_v1(CONNECTION_MODE_CHECKPOINT)
            .map(|entry| text(entry.name))
            .collect();
        assert_eq!(
            checkpoint,
            ["checkpoint.passive.api", "checkpoint.optimize"]
        );
        let passive = connection_entries_for_mode_v1(CONNECTION_MODE_CHECKPOINT)
            .find(|entry| text(entry.name) == "checkpoint.passive.api")
            .unwrap();
        assert_ne!(passive.flags & CONNECTION_FLAG_WRITES_DB_OR_WAL, 0);
        assert_eq!(passive.flags & CONNECTION_FLAG_BEST_EFFORT, 0);

        let open_time = connection_entries_v1()
            .iter()
            .find(|entry| text(entry.name) == "pragma.wal_checkpoint.passive")
            .unwrap();
        assert_ne!(open_time.flags & CONNECTION_FLAG_BEST_EFFORT, 0);
    }
}
