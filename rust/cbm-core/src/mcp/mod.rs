//! MCP JSON-RPC codec 的 test-only parity helper。
//!
//! 這裡逐步固定 MCP JSON-RPC codec 的 byte-level contract；production MCP
//! dispatch、tool handlers 與 server lifecycle 仍由 C 負責。

use crate::store::search_pattern as store_search_pattern;

#[derive(Debug, Clone, PartialEq, Eq)]
enum IdSummary {
    None,
    Int(i64),
    String(Vec<u8>),
    Other,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum ValueKind {
    Object,
    Array,
    String,
    Int,
    Number,
    Bool,
    Null,
}

#[derive(Debug, Clone, PartialEq, Eq)]
struct RequestSummary {
    jsonrpc: Vec<u8>,
    method: Vec<u8>,
    id: IdSummary,
    params: Option<ParamSummary>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
struct ParamSummary {
    kind: ValueKind,
    raw: Vec<u8>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct JsonRpcRequest {
    pub jsonrpc: Vec<u8>,
    pub method: Vec<u8>,
    pub id: ParsedId,
    pub params_raw: Option<Vec<u8>>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ParsedId {
    None,
    Int(i64),
    String(Vec<u8>),
    Other,
}

const SUPPORTED_PROTOCOL_VERSIONS: [&[u8]; 4] =
    [b"2025-11-25", b"2025-06-18", b"2025-03-26", b"2024-11-05"];

const TOOL_NAMES: [&[u8]; 14] = [
    b"index_repository",
    b"search_graph",
    b"query_graph",
    b"trace_path",
    b"get_code_snippet",
    b"get_graph_schema",
    b"get_architecture",
    b"search_code",
    b"list_projects",
    b"delete_project",
    b"index_status",
    b"detect_changes",
    b"manage_adr",
    b"ingest_traces",
];

const TOOL_TITLES: [&[u8]; 14] = [
    b"Index repository",
    b"Search graph",
    b"Query graph",
    b"Trace path",
    b"Get code snippet",
    b"Get graph schema",
    b"Get architecture",
    b"Search code",
    b"List projects",
    b"Delete project",
    b"Index status",
    b"Detect changes",
    b"Manage ADR",
    b"Ingest traces",
];

const TOOL_DESCRIPTIONS: [&str; 14] = [
    r#"Index a repository into the knowledge graph. Special mode 'cross-repo-intelligence': skip extraction, only match Routes/Channels across projects to create CROSS_HTTP_CALLS/CROSS_ASYNC_CALLS/CROSS_CHANNEL edges. Requires target_projects param. Ensure target projects have fresh indexes first."#,
    r#"Search the code knowledge graph for functions, classes, routes, and variables. Use INSTEAD OF grep/glob when finding code definitions, implementations, or relationships. Three search modes: (1) query='update settings' for BM25 ranked full-text search with camelCase splitting and structural label boosting — recommended for natural-language discovery; (2) name_pattern='.*regex.*' for exact pattern matching; (3) semantic_query=[...] for vector cosine search that bridges vocabulary (finds 'publish' when you search 'send'). The three modes are independent and can be combined in a single call. PAGINATION: results are capped at limit (default 200) — broader queries are silently truncated. The response always includes 'total' (full match count before limit) and 'has_more' (true when total > offset+returned). Detect truncation with has_more, then page by re-calling with offset=offset+limit until has_more is false. Narrow first via label/file_pattern/min_degree before paginating large result sets."#,
    r#"Execute a Cypher query against the knowledge graph for complex multi-hop patterns, aggregations, and cross-service analysis. The response includes 'total' (returned row count). There is a hard 100k row ceiling — for broad queries add LIMIT in the Cypher itself or use search_graph + offset/limit pagination instead. COMPLEXITY / BOTTLENECKS: every Function and Method node carries queryable complexity properties — cyclomatic (complexity), cognitive, loop_count, loop_depth (max nested-loop depth, a polynomial-degree proxy), plus interprocedural transitive_loop_depth (worst-case nested-loop degree propagated along CALLS edges) and a recursive flag. Additional hot-path signals: linear_scan_in_loop (count of find/contains/indexOf-style scans inside a loop — the hidden O(n^2) that loop_depth misses), alloc_in_loop (allocations/appends inside a loop), recursion_in_loop (a self-call inside a loop), unguarded_recursion (recursion with no conditionally-guarded base case), param_count and max_access_depth (structure smells). Find all hot-path candidates in one query, e.g. MATCH (f:Function) WHERE f.transitive_loop_depth >= 3 OR f.linear_scan_in_loop >= 1 RETURN f.qualified_name, f.transitive_loop_depth, f.linear_scan_in_loop ORDER BY f.transitive_loop_depth DESC."#,
    r#"Trace paths through the code graph. Modes: calls (callers/callees), data_flow (value propagation with args at each hop), cross_service (through HTTP/async Route nodes). Use INSTEAD OF grep for callers, dependencies, impact analysis, or data flow tracing."#,
    r#"Read source code for a function/class/symbol. IMPORTANT: First call search_graph to find the exact qualified_name, then pass it here. This is a read tool, not a search tool. Accepts full qualified_name (exact match) or short function name (returns suggestions if ambiguous)."#,
    r#"Get the schema of the knowledge graph (node labels, edge types)"#,
    r#"Get high-level architecture overview — packages, services, dependencies, and project structure at a glance. Includes 'clusters': Leiden community detection over the call/import graph, surfacing the de-facto modules (each with a label, member count, cohesion score, representative top_nodes, and the packages/edge_types that bind it) — use these to grasp the real architectural seams, which often cut across the folder layout. Optional path scopes analysis to nodes under that directory prefix (file_path)."#,
    r#"Graph-augmented code search. Finds text patterns via grep, then enriches results with the knowledge graph: deduplicates matches into containing functions, ranks by structural importance (definitions first, popular functions next, tests last). Modes: compact (default, signatures only — token efficient), full (with source), files (just file paths). Use path_filter regex to scope results. TRUNCATION: enriched results are capped at limit (default 10). Response carries 'total_grep_matches' (raw grep hit count) and 'total_results' (deduplicated function count) — compare to limit to detect truncation. There is no offset parameter; to see more, raise limit or narrow the query with file_pattern / path_filter."#,
    r#"List all indexed projects"#,
    r#"Delete a project from the index"#,
    r#"Get the indexing status of a project"#,
    r#"Detect code changes and their impact"#,
    r#"Create or update Architecture Decision Records"#,
    r#"Ingest runtime traces to enhance the knowledge graph"#,
];

const TOOL_INPUT_SCHEMAS: [&str; 14] = [
    r#"{"type":"object","properties":{"repo_path":{"type":"string","description":"Path to the repository"},"mode":{"type":"string","enum":["full","moderate","fast","cross-repo-intelligence"],"default":"full","description":"All modes run type-aware LSP call/usage resolution (per-file + cross-file). full: all files + similarity/semantic edges. moderate: filtered files + similarity/semantic. fast: filtered files, no similarity/semantic. cross-repo-intelligence: match Routes/Channels across projects."},"target_projects":{"type":"array","items":{"type":"string"},"description":"Projects to search for cross-repo links (cross-repo-intelligence mode). Use [\"*\"] for all indexed projects. Run list_projects to see available projects."},"name":{"type":"string","description":"Override the derived project name. Non-ASCII bytes are encoded and unsafe path characters are normalized."},"persistence":{"type":"boolean","default":false,"description":"Write compressed artifact to .codebase-memory/graph.db.zst for team sharing. Teammates can bootstrap from the artifact instead of full re-indexing."}},"required":["repo_path"]}"#,
    r#"{"type":"object","properties":{"project":{"type":"string"},"query":{"type":"string","description":"Natural-language or keyword full-text search using BM25 ranking. Tokens are split on whitespace; camelCase identifiers are indexed as individual words (updateCloudClient → update, cloud, client). Results are ranked with structural boosting: Functions/Methods +10, Routes +8, Classes/Interfaces +5. Noise labels (File/Folder/Module/Variable) are filtered out. When provided, name_pattern is ignored."},"label":{"type":"string"},"name_pattern":{"type":"string"},"qn_pattern":{"type":"string"},"file_pattern":{"type":"string"},"relationship":{"type":"string"},"min_degree":{"type":"integer"},"max_degree":{"type":"integer"},"exclude_entry_points":{"type":"boolean"},"include_connected":{"type":"boolean"},"semantic_query":{"type":"array","items":{"type":"string"},"description":"MUST be an ARRAY of keyword strings (e.g. [\"send\",\"pubsub\",\"publish\"]) — NOT a single string. Each keyword is scored independently via per-keyword min-cosine; results reflect functions that score well on ALL keywords. Requires moderate/full index mode. Results appear in the 'semantic_results' field (separate from 'results')."},"limit":{"type":"integer","description":"Max results per call. Default 200. Response carries 'total' (full match count) and 'has_more' (true if truncated) so callers can detect the limit and paginate."},"offset":{"type":"integer","default":0,"description":"Skip the first N matching nodes. Combine with 'limit' to page: increment offset by limit and re-call while has_more is true."}},"required":["project"]}"#,
    r#"{"type":"object","properties":{"query":{"type":"string","description":"Cypher query"},"project":{"type":"string"},"max_rows":{"type":"integer","description":"Optional row limit. Default: unlimited up to a 100k row ceiling. No offset support — use search_graph for paginated browsing."}},"required":["query","project"]}"#,
    r#"{"type":"object","properties":{"function_name":{"type":"string"},"project":{"type":"string"},"direction":{"type":"string","enum":["inbound","outbound","both"],"default":"both"},"depth":{"type":"integer","default":3},"mode":{"type":"string","enum":["calls","data_flow","cross_service"],"default":"calls","description":"calls: follow CALLS edges. data_flow: follow CALLS+DATA_FLOWS with arg expressions. cross_service: follow HTTP_CALLS+ASYNC_CALLS+DATA_FLOWS through Routes, plus CROSS_* cross-repo edges (CROSS_HTTP_CALLS/ASYNC_CALLS/CHANNEL/GRPC_CALLS/GRAPHQL_CALLS/TRPC_CALLS) to hop into other services."},"parameter_name":{"type":"string","description":"For data_flow mode: scope trace to a specific parameter name"},"edge_types":{"type":"array","items":{"type":"string"}},"risk_labels":{"type":"boolean","default":false,"description":"Add risk classification (CRITICAL/HIGH/MEDIUM/LOW) based on hop distance"},"include_tests":{"type":"boolean","default":false,"description":"Include test files in results. When false (default), test files are filtered out. When true, test nodes are included with is_test=true marker."}},"required":["function_name","project"]}"#,
    r#"{"type":"object","properties":{"qualified_name":{"type":"string","description":"Full qualified_name from search_graph, or short function name"},"project":{"type":"string"},"include_neighbors":{"type":"boolean","default":false}},"required":["qualified_name","project"]}"#,
    r#"{"type":"object","properties":{"project":{"type":"string"}},"required":["project"]}"#,
    r#"{"type":"object","properties":{"project":{"type":"string"},"path":{"type":"string","description":"Optional directory prefix to scope architecture (e.g. apps/hoa)"},"aspects":{"type":"array","items":{"type":"string"}}},"required":["project"]}"#,
    r#"{"type":"object","properties":{"pattern":{"type":"string"},"project":{"type":"string"},"file_pattern":{"type":"string","description":"Glob for grep --include (e.g. *.go)"},"path_filter":{"type":"string","description":"Regex filter on result file paths (e.g. ^src/ or \\.(go|ts)$)"},"mode":{"type":"string","enum":["compact","full","files"],"default":"compact","description":"compact: signatures+metadata (default). full: with source. files: just file list."},"context":{"type":"integer","description":"Lines of context around each match (like grep -C). Only used in compact mode."},"regex":{"type":"boolean","default":false},"limit":{"type":"integer","description":"Max enriched results per call. Default 10. Response includes 'total_grep_matches' and 'total_results' so callers can detect truncation. No offset parameter — raise limit or narrow with file_pattern / path_filter to see more.","default":10}},"required":["pattern","project"]}"#,
    r#"{"type":"object","properties":{}}"#,
    r#"{"type":"object","properties":{"project":{"type":"string"}},"required":["project"]}"#,
    r#"{"type":"object","properties":{"project":{"type":"string"}},"required":["project"]}"#,
    r#"{"type":"object","properties":{"project":{"type":"string"},"scope":{"type":"string"},"depth":{"type":"integer","default":2},"base_branch":{"type":"string","default":"main"},"since":{"type":"string","description":"Git ref or tag to compare from (e.g. HEAD~5, v0.5.0). Diffs <ref>...HEAD."}},"required":["project"]}"#,
    r#"{"type":"object","properties":{"project":{"type":"string"},"mode":{"type":"string","enum":["get","update","sections"]},"content":{"type":"string"},"sections":{"type":"array","items":{"type":"string"}}},"required":["project"]}"#,
    r#"{"type":"object","properties":{"traces":{"type":"array","items":{"type":"object"}},"project":{"type":"string"}},"required":["traces","project"]}"#,
];

const TOOL_OUTPUT_SCHEMA: &[u8] = br#"{"type":"object","additionalProperties":true}"#;

#[must_use]
pub fn initialize_protocol_version(params_json: Option<&[u8]>) -> Vec<u8> {
    let latest = SUPPORTED_PROTOCOL_VERSIONS[0];
    let Some(bytes) = params_json else {
        return latest.to_vec();
    };
    let mut parser = Parser::new(bytes);
    let Some(requested) = parser.parse_root_protocol_version() else {
        return latest.to_vec();
    };
    parser.skip_ws();
    if !parser.is_eof() {
        return latest.to_vec();
    }
    match requested {
        Some(requested) => SUPPORTED_PROTOCOL_VERSIONS
            .iter()
            .find(|version| **version == requested.as_slice())
            .copied()
            .unwrap_or(latest)
            .to_vec(),
        None => latest.to_vec(),
    }
}

#[must_use]
pub fn initialize_response(params_json: Option<&[u8]>) -> Vec<u8> {
    let version = initialize_protocol_version(params_json);
    let mut out = Vec::new();
    out.extend_from_slice(br#"{"protocolVersion":""#);
    out.extend_from_slice(&version);
    out.extend_from_slice(
        br#"","serverInfo":{"name":"codebase-memory-mcp","version":"0.10.0"},"capabilities":{"tools":{"listChanged":false}}}"#,
    );
    out
}

#[must_use]
pub fn jsonrpc_parse(input: &[u8]) -> Option<JsonRpcRequest> {
    let mut parser = Parser::new(input);
    let summary = parser.parse_root_request()?;
    parser.skip_ws();
    if !parser.is_eof() {
        return None;
    }
    Some(JsonRpcRequest {
        jsonrpc: summary.jsonrpc,
        method: summary.method,
        id: match summary.id {
            IdSummary::None => ParsedId::None,
            IdSummary::Int(value) => ParsedId::Int(value),
            IdSummary::String(value) => ParsedId::String(value),
            IdSummary::Other => ParsedId::Other,
        },
        params_raw: summary.params.map(|params| params.raw),
    })
}

#[must_use]
pub fn jsonrpc_request_summary(input: &[u8]) -> Option<Vec<u8>> {
    let mut parser = Parser::new(input);
    let summary = parser.parse_root_request()?;
    parser.skip_ws();
    if !parser.is_eof() {
        return None;
    }
    Some(render_summary(&summary))
}

/// 依 `src/mcp/mcp.c` `TOOLS[]` 順序查找 tool name。
///
/// 這只固定 name -> index contract；`tools/list` serialization 與 tool handlers
/// 仍由 C 負責。
#[must_use]
pub fn tool_index(name: Option<&[u8]>) -> Option<usize> {
    let name = name?;
    TOOL_NAMES.iter().position(|tool_name| *tool_name == name)
}

#[must_use]
pub fn tool_dispatch_index(name: Option<&[u8]>) -> Option<usize> {
    match name? {
        b"trace_call_path" => tool_index(Some(b"trace_path")),
        value => tool_index(Some(value)),
    }
}

#[must_use]
pub fn tool_count() -> usize {
    TOOL_NAMES.len()
}

#[must_use]
pub fn tool_name(index: i64) -> Option<&'static [u8]> {
    let index = usize::try_from(index).ok()?;
    TOOL_NAMES.get(index).copied()
}

#[must_use]
pub fn tool_title(index: i64) -> Option<&'static [u8]> {
    let index = usize::try_from(index).ok()?;
    TOOL_TITLES.get(index).copied()
}

#[must_use]
pub fn tool_description(index: i64) -> Option<&'static [u8]> {
    let index = usize::try_from(index).ok()?;
    TOOL_DESCRIPTIONS
        .get(index)
        .map(|description| description.as_bytes())
}

#[must_use]
pub fn tool_input_schema(index: i64) -> Option<&'static [u8]> {
    let index = usize::try_from(index).ok()?;
    TOOL_INPUT_SCHEMAS
        .get(index)
        .map(|schema| schema.as_bytes())
}

#[must_use]
pub fn tool_output_schema() -> &'static [u8] {
    TOOL_OUTPUT_SCHEMA
}

#[must_use]
pub fn tools_list_json(offset: i64, limit: i64, include_next_cursor: bool) -> Vec<u8> {
    let bounds = tools_page_bounds(offset, limit, include_next_cursor, tool_count() as i64);
    let mut out = Vec::new();
    out.extend_from_slice(br#"{"tools":["#);
    for index in bounds.start..bounds.end {
        if index > bounds.start {
            out.push(b',');
        }
        append_tool_definition(&mut out, index as usize);
    }
    out.push(b']');
    if bounds.has_next {
        out.extend_from_slice(br#","nextCursor":"#);
        push_json_string(&mut out, bounds.next_cursor.to_string().as_bytes());
    }
    out.push(b'}');
    out
}

fn append_tool_definition(out: &mut Vec<u8>, index: usize) {
    out.extend_from_slice(br#"{"name":"#);
    push_json_string(out, TOOL_NAMES[index]);
    out.extend_from_slice(br#","title":"#);
    push_json_string(out, TOOL_TITLES[index]);
    out.extend_from_slice(br#","description":"#);
    push_json_string(out, TOOL_DESCRIPTIONS[index].as_bytes());
    out.extend_from_slice(br#","inputSchema":"#);
    out.extend_from_slice(TOOL_INPUT_SCHEMAS[index].as_bytes());
    out.extend_from_slice(br#","outputSchema":"#);
    out.extend_from_slice(TOOL_OUTPUT_SCHEMA);
    out.push(b'}');
}

#[must_use]
pub fn content_length_header_matches(line: Option<&[u8]>) -> bool {
    line.is_some_and(|line| line.starts_with(b"Content-Length:"))
}

#[must_use]
pub fn content_length_value(line: Option<&[u8]>, max_len: i64) -> i64 {
    let Some(line) = line else {
        return 0;
    };
    let Some(rest) = line.strip_prefix(b"Content-Length:") else {
        return 0;
    };
    let Some(parsed) = strtol_prefix_decimal(rest) else {
        return 0;
    };
    if parsed > 0 && parsed <= max_len {
        parsed
    } else {
        0
    }
}

#[must_use]
pub fn content_length_raw_value(line: Option<&[u8]>) -> i64 {
    let Some(line) = line else {
        return -1;
    };
    let Some(rest) = line.strip_prefix(b"Content-Length:") else {
        return -1;
    };
    strtol_prefix_decimal(rest).unwrap_or(-1)
}

#[must_use]
pub fn content_length_header_is_blank(line: Option<&[u8]>) -> bool {
    let Some(mut line) = line else {
        return false;
    };
    while matches!(line.last(), Some(b'\n' | b'\r')) {
        line = &line[..line.len() - 1];
    }
    line.is_empty()
}

#[must_use]
pub fn content_length_response(resp: Option<&[u8]>) -> Option<Vec<u8>> {
    let resp = resp?;
    let mut out = Vec::new();
    out.extend_from_slice(b"Content-Length: ");
    out.extend_from_slice(resp.len().to_string().as_bytes());
    out.extend_from_slice(b"\r\n\r\n");
    out.extend_from_slice(resp);
    Some(out)
}

#[must_use]
pub fn parse_file_uri(uri: Option<&[u8]>) -> Option<&[u8]> {
    let uri = uri?;
    let mut path = uri.strip_prefix(b"file://")?;
    if path.len() >= 3 && path[0] == b'/' && path[2] == b':' && path[1].is_ascii_alphabetic() {
        path = &path[1..];
    }
    Some(path)
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum MethodKind {
    Unknown = 0,
    Initialize = 1,
    Ping = 2,
    ToolsList = 3,
    ToolsCall = 4,
    NotificationsCancelled = 5,
}

#[must_use]
pub fn method_kind(method: Option<&[u8]>) -> MethodKind {
    match method {
        Some(b"initialize") => MethodKind::Initialize,
        Some(b"ping") => MethodKind::Ping,
        Some(b"tools/list") => MethodKind::ToolsList,
        Some(b"tools/call") => MethodKind::ToolsCall,
        Some(b"notifications/cancelled") => MethodKind::NotificationsCancelled,
        _ => MethodKind::Unknown,
    }
}

#[must_use]
pub fn method_not_found_error() -> &'static [u8] {
    br#"{"code":-32601,"message":"Method not found"}"#
}

#[must_use]
pub fn parse_error_message() -> &'static [u8] {
    b"Parse error"
}

#[must_use]
pub fn ping_result() -> &'static [u8] {
    b"{}"
}

#[must_use]
pub fn tools_call_default_arguments() -> &'static [u8] {
    b"{}"
}

#[must_use]
pub fn missing_tool_name_message() -> &'static [u8] {
    b"missing tool name"
}

#[must_use]
pub fn missing_project_error() -> &'static [u8] {
    br#"{"error":"missing required argument: project","hint":"Pass the project as the \"project\" argument, e.g. {\"project\":\"<name from list_projects>\"}. Run list_projects to see indexed projects."}"#
}

#[must_use]
pub fn project_not_found_message() -> &'static [u8] {
    b"project not found"
}

#[must_use]
pub fn project_list_error(
    reason: Option<&[u8]>,
    projects_csv: Option<&[u8]>,
    count: i32,
) -> Option<Vec<u8>> {
    let reason = reason?;
    if count > 0 {
        let projects = projects_csv?;
        let mut out = Vec::with_capacity(reason.len() + projects.len() + 196);
        out.extend_from_slice(br#"{"error":""#);
        out.extend_from_slice(reason);
        out.extend_from_slice(br#"","hint":"Use list_projects to see all indexed projects, then pass it as the \"project\" argument.","available_projects":["#);
        out.extend_from_slice(projects);
        out.extend_from_slice(br#"],"count":"#);
        out.extend_from_slice(count.to_string().as_bytes());
        out.push(b'}');
        return Some(out);
    }

    let mut out = Vec::with_capacity(reason.len() + 96);
    out.extend_from_slice(br#"{"error":""#);
    out.extend_from_slice(reason);
    out.extend_from_slice(br#"","hint":"No projects indexed yet. Call index_repository first."}"#);
    Some(out)
}

#[must_use]
pub fn unknown_tool_message(tool_name: Option<&[u8]>) -> Option<Vec<u8>> {
    let name = tool_name?;
    let mut out = Vec::with_capacity(b"unknown tool: ".len() + name.len());
    out.extend_from_slice(b"unknown tool: ");
    out.extend_from_slice(name);
    Some(out)
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct ToolsPageBounds {
    pub start: i64,
    pub end: i64,
    pub has_next: bool,
    pub next_cursor: i64,
}

/// 計算 `tools/list` page range，對齊 C `cbm_mcp_tools_list_range()` clamp 規則。
///
/// Rust 只回傳 page bounds；tool metadata、schema JSON 與 serialization 仍由 C 負責。
#[must_use]
pub fn tools_page_bounds(
    offset: i64,
    limit: i64,
    include_next_cursor: bool,
    tool_count: i64,
) -> ToolsPageBounds {
    let tool_count = tool_count.max(0);
    let start = offset.clamp(0, tool_count);
    let limit = if limit < 0 || limit > tool_count {
        tool_count
    } else {
        limit
    };
    let end = start.saturating_add(limit).min(tool_count);
    let has_next = include_next_cursor && end < tool_count;
    ToolsPageBounds {
        start,
        end,
        has_next,
        next_cursor: if has_next { end } else { 0 },
    }
}

/// tools/list 分頁 cursor offset，對齊 `src/mcp/mcp.c` `mcp_tools_cursor_offset`。
///
/// `params_json` 為 tools/list 請求的 params 物件字串（可為 null）。回傳語意：
/// - params_json 為 null，或非合法 JSON（對齊 yyjson_read 失敗，含尾端殘留）→ 0。
/// - 合法 JSON 但 root 非物件、或物件無 `cursor` 鍵 → 0。
/// - 有 `cursor` 鍵：baseline 為 `tool_count`；僅當 cursor 為非空字串且以 base-10
///   `strtol` 完整解析為非負整數（無尾端字元、無溢位）時，clamp 到 `tool_count`。
#[must_use]
pub fn tools_cursor_offset(params_json: Option<&[u8]>, tool_count: i64) -> i64 {
    let Some(bytes) = params_json else {
        return 0;
    };
    let mut parser = Parser::new(bytes);
    let Some(cursor) = parser.parse_root_cursor() else {
        return 0;
    };
    match cursor {
        Some(JsonValue::String(value)) if !value.is_empty() => match strtol_full_nonneg(&value) {
            Some(parsed) => parsed.min(tool_count),
            None => tool_count,
        },
        Some(_) => tool_count,
        None => 0,
    }
}

/// 擷取 `tools/call` params.name。此 helper 刻意忽略 `arguments` 與所有
/// handler-specific 欄位，讓 opt-in dispatch 可獨立於 tool argument parsing 遷移。
#[must_use]
pub fn tools_call_name(params_json: &[u8]) -> Option<Vec<u8>> {
    let mut parser = Parser::new(params_json);
    let name = parser.parse_root_tool_name()?;
    parser.skip_ws();
    if !parser.is_eof() {
        return None;
    }
    name
}

/// 擷取 `tools/call` params.arguments。此 helper 只回傳首個 `arguments` JSON value
/// 的原始 slice；缺少 `arguments` 時對齊 C wrapper 回 `{}`。handler-specific
/// argument parsing、validation 與 ownership 仍由 C 負責。
#[must_use]
pub fn tools_call_arguments(params_json: &[u8]) -> Option<Vec<u8>> {
    let mut parser = Parser::new(params_json);
    let arguments = parser.parse_root_tool_arguments()?;
    parser.skip_ws();
    if !parser.is_eof() {
        return None;
    }
    Some(arguments.unwrap_or_else(|| b"{}".to_vec()))
}

/// 擷取 MCP tool args object 中指定 key 的 string value。key 不存在、value 非
/// string、root 非 object 或 JSON 不合法時回傳 `None`。若有重複 key，對齊
/// 既有 request parser helper 的 first-wins contract。
#[must_use]
pub fn string_arg(args_json: &[u8], key: &[u8]) -> Option<Vec<u8>> {
    if key.is_empty() {
        return None;
    }
    let mut parser = Parser::new(args_json);
    let value = match parser.parse_root_field(key)? {
        Some(JsonValue::String(value)) => Some(value),
        Some(JsonValue::Int(_) | JsonValue::Bool(_) | JsonValue::Other(_)) | None => None,
    };
    parser.skip_ws();
    if !parser.is_eof() {
        return None;
    }
    value
}

/// 擷取 MCP tool args object 中指定 key 的 int value。JSON 不合法或 value 非 int
/// 時回傳 caller 提供的 default。此 helper 只處理 shared argument extraction；
/// 範圍檢查與 handler-specific validation 仍由 C handler 負責。
#[must_use]
pub fn int_arg(args_json: &[u8], key: &[u8], default_value: i32) -> i32 {
    if key.is_empty() {
        return default_value;
    }
    let mut parser = Parser::new(args_json);
    let value = match parser.parse_root_field(key) {
        Some(Some(JsonValue::Int(value))) => value as i32,
        Some(Some(JsonValue::String(_) | JsonValue::Bool(_) | JsonValue::Other(_)))
        | Some(None)
        | None => default_value,
    };
    parser.skip_ws();
    if !parser.is_eof() {
        return default_value;
    }
    value
}

/// 擷取 MCP tool args object 中指定 key 的 bool value。JSON 不合法或 value 非 bool
/// 時回傳 false，對齊 C helper 的缺省語意。
#[must_use]
pub fn bool_arg(args_json: &[u8], key: &[u8]) -> bool {
    if key.is_empty() {
        return false;
    }
    let mut parser = Parser::new(args_json);
    let value = match parser.parse_root_field(key) {
        Some(Some(JsonValue::Bool(value))) => value,
        Some(Some(JsonValue::String(_) | JsonValue::Int(_) | JsonValue::Other(_)))
        | Some(None)
        | None => false,
    };
    parser.skip_ws();
    if !parser.is_eof() {
        return false;
    }
    value
}

/// 驗證 MCP `search_graph.relationship` edge type：只允許 ASCII 大寫字母與
/// underscore，長度最多 64 bytes。空字串對齊 C helper 目前會通過。
#[must_use]
pub fn edge_type_valid(input: Option<&[u8]>) -> bool {
    let Some(value) = input else {
        return false;
    };
    if value.len() > 64 {
        return false;
    }
    value.iter().all(|&b| b == b'_' || b.is_ascii_uppercase())
}

/// 驗證 `search_code` shell path / file_pattern argument。這只固定 C helper 的
/// byte-level denylist；`&` 仍合法，因呼叫端會 quoting。
#[must_use]
pub fn search_path_arg_valid(input: Option<&[u8]>) -> bool {
    let Some(value) = input else {
        return false;
    };
    value.iter().all(|&b| match b {
        b'\'' | b'"' | b';' | b'|' | b'$' | b'`' | b'<' | b'>' | b'\n' | b'\r' => false,
        #[cfg(not(windows))]
        b'\\' => false,
        _ => true,
    })
}

/// 驗證 `search_code` root_path 與 file_pattern 組合。root_path 必填且需通過
/// `search_path_arg_valid`；file_pattern 可省略，但若提供也必須通過同一規則。
#[must_use]
pub fn search_args_valid(root_path: Option<&[u8]>, file_pattern: Option<&[u8]>) -> bool {
    if !search_path_arg_valid(root_path) {
        return false;
    }
    match file_pattern {
        Some(pattern) => search_path_arg_valid(Some(pattern)),
        None => true,
    }
}

/// 解析 `search_code.mode`。回傳值對齊 C helper：0=compact/default、
/// 1=full、2=files；未知、空字串與 null 皆回 compact。
#[must_use]
pub fn search_mode(input: Option<&[u8]>) -> i32 {
    match input {
        Some(b"full") => 1,
        Some(b"files") => 2,
        _ => 0,
    }
}

/// 解析 `index_repository.mode`。回傳值對齊 C enum / dispatch：
/// 0=full/default、1=moderate、2=fast、3=cross-repo-intelligence。
/// 未知、空字串與 null 皆回 full。
#[must_use]
pub fn index_mode(input: Option<&[u8]>) -> i32 {
    match input {
        Some(b"moderate") => 1,
        Some(b"fast") => 2,
        Some(b"cross-repo-intelligence") => 3,
        _ => 0,
    }
}

/// 解析 `trace_path.mode` 的預設 edge type set。回傳 bitmask：
/// bit0=CALLS、bit1=DATA_FLOWS、bit2=HTTP_CALLS、bit3=ASYNC_CALLS、
/// bit4..9=CROSS_*。未知、空字串與 null 皆回 CALLS。
#[must_use]
pub fn trace_mode_edge_mask(input: Option<&[u8]>) -> u32 {
    const CALLS: u32 = 1 << 0;
    const DATA_FLOWS: u32 = 1 << 1;
    const HTTP_CALLS: u32 = 1 << 2;
    const ASYNC_CALLS: u32 = 1 << 3;
    const CROSS_HTTP_CALLS: u32 = 1 << 4;
    const CROSS_ASYNC_CALLS: u32 = 1 << 5;
    const CROSS_CHANNEL: u32 = 1 << 6;
    const CROSS_GRPC_CALLS: u32 = 1 << 7;
    const CROSS_GRAPHQL_CALLS: u32 = 1 << 8;
    const CROSS_TRPC_CALLS: u32 = 1 << 9;

    match input {
        Some(b"data_flow") => CALLS | DATA_FLOWS,
        Some(b"cross_service") => {
            HTTP_CALLS
                | ASYNC_CALLS
                | DATA_FLOWS
                | CALLS
                | CROSS_HTTP_CALLS
                | CROSS_ASYNC_CALLS
                | CROSS_CHANNEL
                | CROSS_GRPC_CALLS
                | CROSS_GRAPHQL_CALLS
                | CROSS_TRPC_CALLS
        }
        _ => CALLS,
    }
}

/// 就地清理 MCP `search_code` output 的非 ASCII byte，對齊 C helper：
/// 每個 > 127 的 byte 直接替換為 `?`，不嘗試 UTF-8 decode。
pub fn sanitize_ascii_in_place(bytes: &mut [u8]) {
    for b in bytes {
        if *b > 127 {
            *b = b'?';
        }
    }
}

/// 計算 `search_code` deduped result ranking score。此 helper 只固定 C
/// `compute_search_score()` 的整數加權，不處理 grep、graph lookup 或 JSON。
#[must_use]
pub fn search_code_score(label: Option<&[u8]>, file: Option<&[u8]>, in_degree: i32) -> i32 {
    const SCORE_FUNC: i32 = 10;
    const SCORE_ROUTE: i32 = 15;
    const SCORE_VENDORED: i32 = -50;
    const SCORE_TEST: i32 = -5;

    let mut score = in_degree;
    if matches!(label, Some(b"Function" | b"Method")) {
        score += SCORE_FUNC;
    }
    if matches!(label, Some(b"Route")) {
        score += SCORE_ROUTE;
    }
    if let Some(file) = file {
        if contains_bytes(file, b"vendored/")
            || contains_bytes(file, b"vendor/")
            || contains_bytes(file, b"node_modules/")
        {
            score += SCORE_VENDORED;
        }
        if contains_bytes(file, b"test")
            || contains_bytes(file, b"spec")
            || contains_bytes(file, b"_test.")
        {
            score += SCORE_TEST;
        }
    }
    score
}

/// 對齊 C `search_result_cmp()` 的 descending score comparator。
/// left score 較高時回傳負值，right score 較高時回傳正值。
#[must_use]
pub fn search_score_cmp(left_score: i32, right_score: i32) -> i32 {
    right_score.wrapping_sub(left_score)
}

/// 對齊 C `build_dir_distribution()` 的 top-level directory key 擷取。
/// 若 path 含 `/`，保留到第一個 slash（含 slash）；否則使用整個 path。
#[must_use]
pub fn search_top_dir(file: Option<&[u8]>) -> Option<Vec<u8>> {
    let file = file?;
    let end = file
        .iter()
        .position(|&b| b == b'/')
        .map_or(file.len(), |idx| idx + 1);
    Some(file[..end].to_vec())
}

fn contains_bytes(haystack: &[u8], needle: &[u8]) -> bool {
    !needle.is_empty() && haystack.windows(needle.len()).any(|w| w == needle)
}

/// 計算 `search_code` grep path 去除 root prefix 後的 borrowed offset。
/// 對齊 C `strip_root_prefix()`：只做 byte prefix 比對；prefix 後若是 `/`
/// 再多跳過一個 byte，不檢查 path component 邊界。
#[must_use]
pub fn search_strip_root_prefix_offset(
    path: Option<&[u8]>,
    root: Option<&[u8]>,
    root_len: usize,
) -> usize {
    let (Some(path), Some(root)) = (path, root) else {
        return 0;
    };
    if root_len > path.len() || root_len > root.len() {
        return 0;
    }
    if path[..root_len] != root[..root_len] {
        return 0;
    }
    let mut offset = root_len;
    if path.get(offset) == Some(&b'/') {
        offset += 1;
    }
    offset
}

/// 解析 `detect_changes.scope` 是否要輸出 impacted symbols。對齊 C helper：
/// null/default、`symbols` 與 `impact` 皆啟用；其他值只輸出 changed files。
#[must_use]
pub fn detect_changes_wants_symbols(scope: Option<&[u8]>) -> bool {
    matches!(scope, None | Some(b"symbols" | b"impact"))
}

/// 判斷 `detect_changes` 是否要把某個 node label 放入 impacted symbols。
/// 對齊 C helper：null、File、Folder、Project 都排除，其餘 label 皆保留。
#[must_use]
pub fn detect_changes_impacted_label(label: Option<&[u8]>) -> bool {
    !matches!(label, None | Some(b"File" | b"Folder" | b"Project"))
}

/// 解析 `git status --porcelain`/`git diff --name-only` 行內容，回傳應採用的
/// path 起始 offset。對齊 C parser：有 porcelain 前綴時跳過 `XY `，rename
/// (`old -> new`) 取箭頭後 destination；空路徑回傳 `usize::MAX`。
#[must_use]
pub fn detect_changes_status_path_offset(line: Option<&[u8]>) -> usize {
    const INVALID: usize = usize::MAX;
    const PORCELAIN_PREFIX_LEN: usize = 3;
    const RENAME_ARROW: &[u8] = b" -> ";

    let Some(line) = line else {
        return INVALID;
    };

    let mut path = line;
    if line.len() > 2
        && line[2] == b' '
        && is_porcelain_status_code(line[0])
        && is_porcelain_status_code(line[1])
    {
        path = &line[PORCELAIN_PREFIX_LEN..];
        if let Some(arrow) = find_subslice(path, RENAME_ARROW) {
            path = &path[arrow + RENAME_ARROW.len()..];
        }
    }

    if path.is_empty() {
        return INVALID;
    }

    line.len() - path.len()
}

/// 計算 `search_code` grep hit 是否落在 node 範圍內；命中時回傳 span(`end-start`)，
/// 未命中回傳 -1。對齊 C `find_tightest_node()` 的單一 node 評分 contract。
#[must_use]
pub fn search_line_match_span(start_line: i64, end_line: i64, line: i64) -> i64 {
    if start_line <= line && end_line >= line {
        end_line - start_line
    } else {
        -1
    }
}

/// 依據 candidate scores 決定 `pick_resolved_node()` 的 best index 與 ambiguous flag。
/// 回傳 `(best_index, ambiguous)`；空輸入回 `(-1, false)`。若 top score 有多個候選，
/// 會回傳第一個 top index 並標記 ambiguous=true。
#[must_use]
pub fn search_pick_resolved_index<T: Copy + PartialOrd + PartialEq>(scores: &[T]) -> (i32, bool) {
    let Some((&first, rest)) = scores.split_first() else {
        return (-1, false);
    };
    let mut best_idx = 0usize;
    let mut best_score = first;
    for (i, &score) in rest.iter().enumerate() {
        if score > best_score {
            best_score = score;
            best_idx = i + 1;
        }
    }
    let top_count = scores.iter().filter(|&&s| s == best_score).count();
    (best_idx as i32, top_count > 1)
}

/// 依據 `find_tightest_node()` 的 span 陣列選出最小且非負的 index。
/// 回傳第一個最小有效 span；若沒有有效 span（全為負值或空輸入）回傳 -1。
#[must_use]
pub fn search_pick_tightest_index(spans: &[i32]) -> i32 {
    let mut best_idx = -1;
    let mut best_span = i32::MAX;
    for (i, &span) in spans.iter().enumerate() {
        if span >= 0 && span < best_span {
            best_span = span;
            best_idx = i as i32;
        }
    }
    best_idx
}

/// 判斷 UTF-8 continuation byte（10xxxxxx）。
#[must_use]
pub fn utf8_is_cont_byte(b: u8) -> bool {
    (b & 0xC0) == 0x80
}

#[must_use]
fn is_porcelain_status_code(b: u8) -> bool {
    matches!(
        b,
        b' ' | b'M' | b'A' | b'D' | b'R' | b'C' | b'U' | b'?' | b'!'
    )
}

#[must_use]
fn find_subslice(haystack: &[u8], needle: &[u8]) -> Option<usize> {
    if needle.is_empty() || haystack.len() < needle.len() {
        return None;
    }
    haystack.windows(needle.len()).position(|w| w == needle)
}

/// 計算 `trace_call_path` / snippet auto-resolve 的 candidate score。
/// label tier 是主要排序鍵：Function/Method > class-like 其他 label > Module/File。
/// 同 tier 時較大的 line span 勝出；負 span 對齊 C helper 視為 0。
#[must_use]
pub fn node_resolution_score(label: Option<&[u8]>, start_line: i64, end_line: i64) -> i64 {
    const RES_RANK_CALLABLE: i64 = 2;
    const RES_RANK_OTHER: i64 = 1;
    const RES_RANK_MODULE: i64 = 0;
    const RES_LABEL_WEIGHT: i64 = 1_000_000;

    let label_rank = match label {
        Some(b"Function" | b"Method") => RES_RANK_CALLABLE,
        Some(b"Module" | b"File") | None => RES_RANK_MODULE,
        Some(_) => RES_RANK_OTHER,
    };
    let span = (end_line - start_line).max(0);
    label_rank * RES_LABEL_WEIGHT + span
}

/// 解析 `manage_adr.mode`。回傳值對齊 C handler dispatch：
/// 0=get/default、1=update/store、2=sections。
#[must_use]
pub fn adr_mode(input: Option<&[u8]>) -> i32 {
    match input {
        Some(b"update" | b"store") => 1,
        Some(b"sections") => 2,
        _ => 0,
    }
}

/// 從 ADR markdown 內容擷取 `#` 開頭的 section header，並輸出 JSON array。
/// 對齊 C `adr_list_sections_from_content()`：null/empty 產生空 array，逐行移除
/// trailing `\r`，只接受第一個 byte 為 `#` 且非空的行，單一 header 最多保留
/// 1023 bytes。
#[must_use]
pub fn adr_sections_json(content: Option<&[u8]>) -> Vec<u8> {
    const MAX_ADR_HEADER_BYTES: usize = 1023;

    let mut out = Vec::new();
    out.push(b'[');
    let Some(content) = content else {
        out.push(b']');
        return out;
    };

    let mut first = true;
    for mut line in content.split(|&b| b == b'\n') {
        while line.last() == Some(&b'\r') {
            line = &line[..line.len() - 1];
        }
        if line.first() != Some(&b'#') {
            continue;
        }
        if !first {
            out.push(b',');
        }
        first = false;
        let header = &line[..line.len().min(MAX_ADR_HEADER_BYTES)];
        push_json_string(&mut out, header);
    }
    out.push(b']');
    out
}

/// 建立 MCP `search_graph` BM25 FTS5 MATCH expression，對齊 C helper：
/// 只保留 ASCII alnum/underscore token，以 ` OR ` 串接，輸出空間不足時
/// 停在上一個完整 token。
#[must_use]
pub fn bm25_match_query(input: Option<&[u8]>, out_size: usize) -> (i32, Vec<u8>) {
    const BM25_MIN_BUF: usize = 2;
    const BM25_SEP_RESERVE: usize = 1;

    let Some(query) = input else {
        return (0, Vec::new());
    };
    if out_size < BM25_MIN_BUF {
        return (0, Vec::new());
    }

    let mut out = Vec::new();
    let mut tokens = 0i32;
    let mut i = 0usize;
    while i < query.len() {
        while i < query.len() && !is_bm25_token_byte(query[i]) {
            i += 1;
        }
        if i >= query.len() {
            break;
        }
        let start = i;
        while i < query.len() && is_bm25_token_byte(query[i]) {
            i += 1;
        }
        let tok = &query[start..i];
        let sep = if tokens > 0 {
            b" OR ".as_slice()
        } else {
            b"".as_slice()
        };
        if out.len() + sep.len() + tok.len() + BM25_SEP_RESERVE >= out_size {
            break;
        }
        out.extend_from_slice(sep);
        out.extend_from_slice(tok);
        tokens += 1;
    }
    (tokens, out)
}

fn is_bm25_token_byte(b: u8) -> bool {
    b.is_ascii_alphanumeric() || b == b'_'
}

/// 建立 MCP `search_graph.file_pattern` BM25 side path 的 SQL LIKE pattern。
/// 先沿用 Store glob-to-LIKE helper；若原始 pattern 不含 glob wildcard，對齊 C helper
/// 加上前後 `%`，讓 `foo` 以 contains LIKE 查詢。
#[must_use]
pub fn bm25_file_pattern_like(input: Option<&[u8]>) -> Option<Vec<u8>> {
    let pattern = input?;
    let like = store_search_pattern::glob_to_like(Some(pattern))?;
    if pattern.contains(&b'*') || pattern.contains(&b'?') {
        return Some(like);
    }
    let mut contains = Vec::with_capacity(like.len() + 2);
    contains.push(b'%');
    contains.extend_from_slice(&like);
    contains.push(b'%');
    Some(contains)
}

/// 針對非 UTF-8 byte 進行 lossy 消毒，將其替換為 REPLACEMENT CHARACTER U+FFFD。
/// 對齊 `sanitize_utf8_lossy` 的 C 實作。
#[must_use]
pub fn sanitize_utf8_lossy(input: Option<&[u8]>) -> Option<Vec<u8>> {
    let bytes = input?;
    let cow = String::from_utf8_lossy(bytes);
    Some(cow.into_owned().into_bytes())
}

/// 判斷一個 architecture aspect 是否被請求。
/// 精確對齊 C11 `aspect_wanted` 的 contract：
/// - 缺少 `aspects` 鍵、invalid JSON、root 非 object、aspects 非 array 時視為 wanted (true)
/// - 空 array 或無匹配為 false
/// - "all" 或 exact name match 為 true（大小寫與空白敏感）
/// - 非字串元素忽略。
#[must_use]
pub fn architecture_aspect_wanted(params_json: Option<&[u8]>, name: &[u8]) -> bool {
    let Some(bytes) = params_json else {
        return true;
    };
    let mut parser = Parser::new(bytes);
    parser.skip_ws();
    if !parser.consume(b'{') {
        return true;
    }

    parser.skip_ws();
    if parser.consume(b'}') {
        return true;
    }

    let mut aspects_found_and_not_wanted = false;
    let mut seen_aspects = false;

    loop {
        parser.skip_ws();
        let Some(key) = parser.parse_string() else {
            return true;
        };
        parser.skip_ws();
        if !parser.consume(b':') {
            return true;
        }
        parser.skip_ws();
        if key.as_slice() == b"aspects" && !seen_aspects {
            seen_aspects = true;
            parser.skip_ws();
            if parser.peek() == Some(b'[') {
                let Some(list) = parser.parse_aspects_array() else {
                    return true;
                };
                let mut matched = false;
                for item in list {
                    if item == b"all" || item == name {
                        matched = true;
                        break;
                    }
                }
                if matched {
                    return true;
                } else {
                    aspects_found_and_not_wanted = true;
                }
            } else {
                if parser.parse_value().is_none() {
                    return true;
                }
            }
        } else {
            if parser.parse_value().is_none() {
                return true;
            }
        }

        parser.skip_ws();
        if parser.consume(b'}') {
            break;
        }
        if !parser.consume(b',') {
            return true;
        }
    }

    parser.skip_ws();
    if !parser.is_eof() {
        return true;
    }

    !aspects_found_and_not_wanted
}

/// 判斷 `trace_call_path` BFS result 的 file path 是否看起來像測試檔。
/// 這固定 MCP C helper 的 substring contract，刻意不重用 pipeline/store
/// test detection，因為那些模組的定義較精細且語意不同。
#[must_use]
pub fn trace_is_test_file(input: Option<&[u8]>) -> bool {
    let Some(path) = input else {
        return false;
    };
    path.windows(5).any(|w| w == b"/test")
        || path.windows(5).any(|w| w == b"test_")
        || path.windows(6).any(|w| w == b"_test.")
        || path.windows(7).any(|w| w == b"/tests/")
        || path.windows(6).any(|w| w == b"/spec/")
        || path.windows(6).any(|w| w == b".test.")
}

/// 判斷 cache directory entry 是否可作為 project `.db` 候選檔名。
/// 對齊 MCP C helper：至少 `x.db`、副檔名精準 `.db`、排除 `_` 開頭與
/// `:memory:`，但保留合法的 `tmp-*.db` project 檔。
#[must_use]
pub fn project_db_file_name(input: Option<&[u8]>) -> bool {
    let Some(name) = input else {
        return false;
    };
    name.len() >= 4
        && name.ends_with(b".db")
        && !name.starts_with(b"_")
        && !name.starts_with(b":memory:")
}

/// 比對 `notifications/cancelled` params.requestId。此 helper 只固定取消通知的
/// id matching contract，不處理 server lifecycle 或 pipeline cancellation side effect。
#[must_use]
pub fn cancel_request_matches(
    params_json: Option<&[u8]>,
    active_id: i64,
    active_id_str: Option<&[u8]>,
) -> bool {
    let Some(bytes) = params_json else {
        return false;
    };
    let mut parser = Parser::new(bytes);
    let Some(request_id) = parser.parse_root_request_id() else {
        return false;
    };
    parser.skip_ws();
    if !parser.is_eof() {
        return false;
    }
    match (active_id_str, request_id) {
        (Some(active), Some(JsonValue::String(value))) => value.as_slice() == active,
        (None, Some(JsonValue::Int(value))) => value == active_id,
        _ => false,
    }
}

/// 格式化 JSON-RPC error response，對齊 C `cbm_jsonrpc_format_error()` 的
/// compact JSON 欄位順序。這只處理 numeric id error path；string id response
/// 與 result embedding 仍留在 C。
#[must_use]
pub fn jsonrpc_format_error(id: i64, code: i64, message: Option<&[u8]>) -> Vec<u8> {
    let mut out = Vec::new();
    out.extend_from_slice(br#"{"jsonrpc":"2.0","id":"#);
    out.extend_from_slice(id.to_string().as_bytes());
    out.extend_from_slice(br#","error":{"code":"#);
    out.extend_from_slice(code.to_string().as_bytes());
    out.extend_from_slice(br#","message":"#);
    push_json_string(&mut out, message.unwrap_or_default());
    out.extend_from_slice(b"}}");
    out
}

/// 格式化 JSON-RPC response，對齊 C `cbm_jsonrpc_format_response()` 的主要
/// response contract。`error_json` 優先於 `result_json`；invalid embedded JSON
/// 會像 C/yyjson path 一樣略過該欄位。
#[must_use]
pub fn jsonrpc_format_response(
    id: i64,
    id_str: Option<&[u8]>,
    result_json: Option<&[u8]>,
    error_json: Option<&[u8]>,
) -> Vec<u8> {
    let mut out = Vec::new();
    out.extend_from_slice(br#"{"jsonrpc":"2.0","id":"#);
    if let Some(id_str) = id_str {
        push_json_string(&mut out, id_str);
    } else {
        out.extend_from_slice(id.to_string().as_bytes());
    }

    if let Some(error) = error_json.and_then(minified_json_value) {
        out.extend_from_slice(br#","error":"#);
        out.extend_from_slice(&error);
    } else if error_json.is_none() {
        if let Some(result) = result_json.and_then(minified_json_value) {
            out.extend_from_slice(br#","result":"#);
            out.extend_from_slice(&result);
        } else if result_json.is_none() {
            out.extend_from_slice(br#","result":null"#);
        }
    }

    out.push(b'}');
    out
}

/// 格式化 MCP text result，對齊 C `cbm_mcp_text_result()`。`structuredContent`
/// 只在非 error 且 `text` 是 JSON object 時出現。
#[must_use]
pub fn mcp_text_result(text: Option<&[u8]>, is_error: bool) -> Vec<u8> {
    let text = text.unwrap_or_default();
    let mut out = Vec::new();
    out.extend_from_slice(br#"{"content":[{"type":"text","text":"#);
    push_json_string(&mut out, text);
    out.extend_from_slice(b"}]");

    if !is_error {
        if let Some(structured) = minified_json_object(text) {
            out.extend_from_slice(br#","structuredContent":"#);
            out.extend_from_slice(&structured);
        }
    }

    out.extend_from_slice(br#","isError":"#);
    out.extend_from_slice(if is_error { b"true" } else { b"false" });
    out.push(b'}');
    out
}

fn minified_json_value(input: &[u8]) -> Option<Vec<u8>> {
    let mut parser = Parser::new(input);
    parser.parse_value()?;
    parser.skip_ws();
    if !parser.is_eof() {
        return None;
    }

    let mut out = Vec::with_capacity(input.len());
    let mut in_string = false;
    let mut escaped = false;
    for &b in input {
        if in_string {
            out.push(b);
            if escaped {
                escaped = false;
            } else if b == b'\\' {
                escaped = true;
            } else if b == b'"' {
                in_string = false;
            }
        } else if b == b'"' {
            in_string = true;
            out.push(b);
        } else if !matches!(b, b' ' | b'\n' | b'\r' | b'\t') {
            out.push(b);
        }
    }
    Some(out)
}

fn minified_json_object(input: &[u8]) -> Option<Vec<u8>> {
    let mut parser = Parser::new(input);
    let value = parser.parse_value()?;
    parser.skip_ws();
    if !parser.is_eof() || value.kind() != ValueKind::Object {
        return None;
    }
    minified_json_value(input)
}

fn push_json_string(out: &mut Vec<u8>, input: &[u8]) {
    out.push(b'"');
    let lossy = String::from_utf8_lossy(input);
    for ch in lossy.chars() {
        match ch {
            '"' => out.extend_from_slice(br#"\""#),
            '\\' => out.extend_from_slice(br#"\\"#),
            '\u{08}' => out.extend_from_slice(br#"\b"#),
            '\u{0c}' => out.extend_from_slice(br#"\f"#),
            '\n' => out.extend_from_slice(br#"\n"#),
            '\r' => out.extend_from_slice(br#"\r"#),
            '\t' => out.extend_from_slice(br#"\t"#),
            '\0'..='\u{1f}' => {
                out.extend_from_slice(br#"\u00"#);
                out.push(hex_digit(((ch as u32) >> 4) as u8));
                out.push(hex_digit((ch as u32) as u8));
            }
            _ => {
                let mut buf = [0; 4];
                out.extend_from_slice(ch.encode_utf8(&mut buf).as_bytes());
            }
        }
    }
    out.push(b'"');
}

/// 對齊 C `strtol(s, &endptr, 10)` 搭配 `*endptr == '\0' && errno == 0 && parsed >= 0`
/// 的驗證：跳過前導空白（C locale isspace）、可選正負號、至少一位 base-10 數字、
/// 整串消耗完（無尾端）、無溢位、結果非負。符合時回傳非負值，否則回傳 None。
fn strtol_full_nonneg(s: &[u8]) -> Option<i64> {
    let mut i = 0;
    while i < s.len() && matches!(s[i], b' ' | b'\t' | b'\n' | 0x0b | 0x0c | b'\r') {
        i += 1;
    }
    let negative = match s.get(i) {
        Some(b'-') => {
            i += 1;
            true
        }
        Some(b'+') => {
            i += 1;
            false
        }
        _ => false,
    };
    let digits_start = i;
    let mut value: i64 = 0;
    let mut overflow = false;
    while i < s.len() && s[i].is_ascii_digit() {
        let digit = i64::from(s[i] - b'0');
        match value.checked_mul(10).and_then(|v| v.checked_add(digit)) {
            Some(v) => value = v,
            None => overflow = true,
        }
        i += 1;
    }
    if i == digits_start || i != s.len() || overflow {
        return None;
    }
    if negative && value != 0 {
        return None;
    }
    Some(value)
}

fn strtol_prefix_decimal(s: &[u8]) -> Option<i64> {
    let mut i = 0;
    while i < s.len() && matches!(s[i], b' ' | b'\t' | b'\n' | 0x0b | 0x0c | b'\r') {
        i += 1;
    }
    let negative = match s.get(i) {
        Some(b'-') => {
            i += 1;
            true
        }
        Some(b'+') => {
            i += 1;
            false
        }
        _ => false,
    };
    let digits_start = i;
    let mut value: i128 = 0;
    while i < s.len() && s[i].is_ascii_digit() {
        value = value
            .saturating_mul(10)
            .saturating_add(i128::from(s[i] - b'0'));
        i += 1;
    }
    if i == digits_start {
        return None;
    }
    if negative {
        value = -value;
    }
    if value > i128::from(i64::MAX) {
        Some(i64::MAX)
    } else if value < i128::from(i64::MIN) {
        Some(i64::MIN)
    } else {
        Some(value as i64)
    }
}

fn render_summary(summary: &RequestSummary) -> Vec<u8> {
    let mut out = Vec::new();
    out.extend_from_slice(b"jsonrpc=");
    push_escaped(&mut out, &summary.jsonrpc);
    out.extend_from_slice(b";method=");
    push_escaped(&mut out, &summary.method);
    out.extend_from_slice(b";id=");
    match &summary.id {
        IdSummary::None => out.extend_from_slice(b"none"),
        IdSummary::Int(value) => {
            out.extend_from_slice(b"int:");
            out.extend_from_slice(value.to_string().as_bytes());
        }
        IdSummary::String(value) => {
            out.extend_from_slice(b"string:");
            push_escaped(&mut out, value);
        }
        IdSummary::Other => out.extend_from_slice(b"other"),
    }
    out.extend_from_slice(b";params=");
    out.extend_from_slice(match summary.params.as_ref().map(|params| params.kind) {
        None => b"none".as_slice(),
        Some(ValueKind::Object) => b"object",
        Some(ValueKind::Array) => b"array",
        Some(ValueKind::String) => b"string",
        Some(ValueKind::Int) => b"int",
        Some(ValueKind::Number) => b"number",
        Some(ValueKind::Bool) => b"bool",
        Some(ValueKind::Null) => b"null",
    });
    out
}

fn push_escaped(out: &mut Vec<u8>, value: &[u8]) {
    for &b in value {
        match b {
            b'\\' => out.extend_from_slice(b"\\\\"),
            b';' => out.extend_from_slice(b"\\;"),
            b'=' => out.extend_from_slice(b"\\="),
            b'\n' => out.extend_from_slice(b"\\n"),
            b'\r' => out.extend_from_slice(b"\\r"),
            b'\t' => out.extend_from_slice(b"\\t"),
            0x20..=0x7e => out.push(b),
            _ => {
                out.extend_from_slice(b"\\x");
                out.push(hex_digit(b >> 4));
                out.push(hex_digit(b & 0x0f));
            }
        }
    }
}

fn hex_digit(n: u8) -> u8 {
    match n {
        0..=9 => b'0' + n,
        _ => b'a' + (n - 10),
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
enum JsonValue {
    String(Vec<u8>),
    Int(i64),
    Bool(bool),
    Other(ValueKind),
}

impl JsonValue {
    fn kind(&self) -> ValueKind {
        match self {
            Self::String(_) => ValueKind::String,
            Self::Int(_) => ValueKind::Int,
            Self::Bool(_) => ValueKind::Bool,
            Self::Other(kind) => *kind,
        }
    }
}

struct Parser<'a> {
    input: &'a [u8],
    pos: usize,
}

impl<'a> Parser<'a> {
    fn new(input: &'a [u8]) -> Self {
        Self { input, pos: 0 }
    }

    fn is_eof(&self) -> bool {
        self.pos >= self.input.len()
    }

    fn skip_ws(&mut self) {
        while let Some(b' ' | b'\n' | b'\r' | b'\t') = self.peek() {
            self.pos += 1;
        }
    }

    fn peek(&self) -> Option<u8> {
        self.input.get(self.pos).copied()
    }

    fn consume(&mut self, expected: u8) -> bool {
        if self.peek() == Some(expected) {
            self.pos += 1;
            true
        } else {
            false
        }
    }

    fn parse_root_request(&mut self) -> Option<RequestSummary> {
        self.skip_ws();
        if !self.consume(b'{') {
            return None;
        }

        let mut jsonrpc = b"2.0".to_vec();
        let mut method = None;
        let mut id = IdSummary::None;
        let mut params = None;
        let mut seen_jsonrpc = false;
        let mut seen_method = false;
        let mut seen_id = false;
        let mut seen_params = false;

        self.skip_ws();
        if self.consume(b'}') {
            return None;
        }

        loop {
            self.skip_ws();
            let key = self.parse_string()?;
            self.skip_ws();
            if !self.consume(b':') {
                return None;
            }
            self.skip_ws();
            let value_start = self.pos;
            let value = self.parse_value()?;
            let value_end = self.pos;
            match key.as_slice() {
                b"jsonrpc" if !seen_jsonrpc => {
                    seen_jsonrpc = true;
                    if let JsonValue::String(value) = value {
                        jsonrpc = value;
                    }
                }
                b"method" if !seen_method => {
                    seen_method = true;
                    if let JsonValue::String(value) = value {
                        method = Some(value);
                    }
                }
                b"id" if !seen_id => {
                    seen_id = true;
                    id = match value {
                        JsonValue::Int(value) => IdSummary::Int(value),
                        JsonValue::String(value) => IdSummary::String(value),
                        JsonValue::Bool(_) | JsonValue::Other(_) => IdSummary::Other,
                    };
                }
                b"params" if !seen_params => {
                    seen_params = true;
                    params = Some(ParamSummary {
                        kind: value.kind(),
                        raw: self.input[value_start..value_end].to_vec(),
                    });
                }
                _ => {}
            }

            self.skip_ws();
            if self.consume(b'}') {
                break;
            }
            if !self.consume(b',') {
                return None;
            }
        }

        Some(RequestSummary {
            jsonrpc,
            method: method?,
            id,
            params,
        })
    }

    fn parse_value(&mut self) -> Option<JsonValue> {
        self.skip_ws();
        match self.peek()? {
            b'"' => self.parse_string().map(JsonValue::String),
            b'{' => {
                self.parse_object_skip()?;
                Some(JsonValue::Other(ValueKind::Object))
            }
            b'[' => {
                self.parse_array_skip()?;
                Some(JsonValue::Other(ValueKind::Array))
            }
            b't' => {
                self.expect_bytes(b"true")?;
                Some(JsonValue::Bool(true))
            }
            b'f' => {
                self.expect_bytes(b"false")?;
                Some(JsonValue::Bool(false))
            }
            b'n' => {
                self.expect_bytes(b"null")?;
                Some(JsonValue::Other(ValueKind::Null))
            }
            b'-' | b'0'..=b'9' => self.parse_number(),
            _ => None,
        }
    }

    fn parse_object_skip(&mut self) -> Option<()> {
        if !self.consume(b'{') {
            return None;
        }
        self.skip_ws();
        if self.consume(b'}') {
            return Some(());
        }
        loop {
            self.skip_ws();
            self.parse_string()?;
            self.skip_ws();
            if !self.consume(b':') {
                return None;
            }
            self.parse_value()?;
            self.skip_ws();
            if self.consume(b'}') {
                return Some(());
            }
            if !self.consume(b',') {
                return None;
            }
        }
    }

    fn parse_root_cursor(&mut self) -> Option<Option<JsonValue>> {
        self.skip_ws();
        let cursor = if self.peek() == Some(b'{') {
            self.parse_object_capture_cursor()?
        } else {
            self.parse_value()?;
            None
        };
        self.skip_ws();
        if !self.is_eof() {
            return None;
        }
        Some(cursor)
    }

    fn parse_object_capture_cursor(&mut self) -> Option<Option<JsonValue>> {
        if !self.consume(b'{') {
            return None;
        }
        self.skip_ws();
        if self.consume(b'}') {
            return Some(None);
        }
        let mut cursor = None;
        loop {
            self.skip_ws();
            let key = self.parse_string()?;
            self.skip_ws();
            if !self.consume(b':') {
                return None;
            }
            let value = self.parse_value()?;
            if key.as_slice() == b"cursor" && cursor.is_none() {
                cursor = Some(value);
            }
            self.skip_ws();
            if self.consume(b'}') {
                break;
            }
            if !self.consume(b',') {
                return None;
            }
        }
        Some(cursor)
    }

    fn parse_root_field(&mut self, target: &[u8]) -> Option<Option<JsonValue>> {
        self.skip_ws();
        if !self.consume(b'{') {
            return None;
        }
        self.skip_ws();
        if self.consume(b'}') {
            return Some(None);
        }
        let mut found = None;
        loop {
            self.skip_ws();
            let key = self.parse_string()?;
            self.skip_ws();
            if !self.consume(b':') {
                return None;
            }
            let value = self.parse_value()?;
            if key.as_slice() == target && found.is_none() {
                found = Some(value);
            }
            self.skip_ws();
            if self.consume(b'}') {
                break;
            }
            if !self.consume(b',') {
                return None;
            }
        }
        Some(found)
    }

    fn parse_root_protocol_version(&mut self) -> Option<Option<Vec<u8>>> {
        self.skip_ws();
        if !self.consume(b'{') {
            self.parse_value()?;
            return Some(None);
        }
        self.skip_ws();
        if self.consume(b'}') {
            return Some(None);
        }

        let mut protocol_version = None;
        loop {
            self.skip_ws();
            let key = self.parse_string()?;
            self.skip_ws();
            if !self.consume(b':') {
                return None;
            }
            let value = self.parse_value()?;
            if key.as_slice() == b"protocolVersion" && protocol_version.is_none() {
                protocol_version = Some(match value {
                    JsonValue::String(value) => Some(value),
                    _ => None,
                });
            }
            self.skip_ws();
            if self.consume(b'}') {
                break;
            }
            if !self.consume(b',') {
                return None;
            }
        }
        Some(protocol_version.flatten())
    }

    fn parse_root_tool_name(&mut self) -> Option<Option<Vec<u8>>> {
        self.skip_ws();
        if !self.consume(b'{') {
            return None;
        }
        self.skip_ws();
        if self.consume(b'}') {
            return Some(None);
        }

        let mut name = None;
        loop {
            self.skip_ws();
            let key = self.parse_string()?;
            self.skip_ws();
            if !self.consume(b':') {
                return None;
            }
            let value = self.parse_value()?;
            if key.as_slice() == b"name" && name.is_none() {
                name = Some(match value {
                    JsonValue::String(value) => Some(value),
                    _ => None,
                });
            }
            self.skip_ws();
            if self.consume(b'}') {
                break;
            }
            if !self.consume(b',') {
                return None;
            }
        }
        Some(name.flatten())
    }

    fn parse_root_tool_arguments(&mut self) -> Option<Option<Vec<u8>>> {
        self.skip_ws();
        if !self.consume(b'{') {
            return None;
        }
        self.skip_ws();
        if self.consume(b'}') {
            return Some(None);
        }

        let mut arguments = None;
        loop {
            self.skip_ws();
            let key = self.parse_string()?;
            self.skip_ws();
            if !self.consume(b':') {
                return None;
            }
            self.skip_ws();
            let value_start = self.pos;
            self.parse_value()?;
            let value_end = self.pos;
            if key.as_slice() == b"arguments" && arguments.is_none() {
                arguments = Some(self.input[value_start..value_end].to_vec());
            }
            self.skip_ws();
            if self.consume(b'}') {
                break;
            }
            if !self.consume(b',') {
                return None;
            }
        }
        Some(arguments)
    }

    fn parse_root_request_id(&mut self) -> Option<Option<JsonValue>> {
        self.skip_ws();
        if self.peek() == Some(b'{') {
            self.parse_object_capture_request_id()
        } else {
            self.parse_value()?;
            Some(None)
        }
    }

    fn parse_object_capture_request_id(&mut self) -> Option<Option<JsonValue>> {
        if !self.consume(b'{') {
            return None;
        }
        self.skip_ws();
        if self.consume(b'}') {
            return Some(None);
        }
        let mut request_id = None;
        loop {
            self.skip_ws();
            let key = self.parse_string()?;
            self.skip_ws();
            if !self.consume(b':') {
                return None;
            }
            let value = self.parse_value()?;
            if key.as_slice() == b"requestId" && request_id.is_none() {
                request_id = Some(value);
            }
            self.skip_ws();
            if self.consume(b'}') {
                break;
            }
            if !self.consume(b',') {
                return None;
            }
        }
        Some(request_id)
    }

    fn parse_array_skip(&mut self) -> Option<()> {
        if !self.consume(b'[') {
            return None;
        }
        self.skip_ws();
        if self.consume(b']') {
            return Some(());
        }
        loop {
            self.parse_value()?;
            self.skip_ws();
            if self.consume(b']') {
                return Some(());
            }
            if !self.consume(b',') {
                return None;
            }
        }
    }

    fn parse_aspects_array(&mut self) -> Option<Vec<Vec<u8>>> {
        if !self.consume(b'[') {
            return None;
        }
        self.skip_ws();
        if self.consume(b']') {
            return Some(Vec::new());
        }
        let mut list = Vec::new();
        loop {
            self.skip_ws();
            match self.peek()? {
                b'"' => {
                    let s = self.parse_string()?;
                    list.push(s);
                }
                _ => {
                    self.parse_value()?;
                }
            }
            self.skip_ws();
            if self.consume(b']') {
                return Some(list);
            }
            if !self.consume(b',') {
                return None;
            }
        }
    }

    fn parse_number(&mut self) -> Option<JsonValue> {
        let start = self.pos;
        if self.consume(b'-') && !matches!(self.peek(), Some(b'0'..=b'9')) {
            return None;
        }
        match self.peek()? {
            b'0' => self.pos += 1,
            b'1'..=b'9' => {
                while matches!(self.peek(), Some(b'0'..=b'9')) {
                    self.pos += 1;
                }
            }
            _ => return None,
        }

        let mut integer_only = true;
        if self.consume(b'.') {
            integer_only = false;
            if !matches!(self.peek(), Some(b'0'..=b'9')) {
                return None;
            }
            while matches!(self.peek(), Some(b'0'..=b'9')) {
                self.pos += 1;
            }
        }
        if matches!(self.peek(), Some(b'e' | b'E')) {
            integer_only = false;
            self.pos += 1;
            if matches!(self.peek(), Some(b'+' | b'-')) {
                self.pos += 1;
            }
            if !matches!(self.peek(), Some(b'0'..=b'9')) {
                return None;
            }
            while matches!(self.peek(), Some(b'0'..=b'9')) {
                self.pos += 1;
            }
        }

        if integer_only {
            let text = std::str::from_utf8(&self.input[start..self.pos]).ok()?;
            if let Ok(value) = text.parse::<i64>() {
                return Some(JsonValue::Int(value));
            }
        }
        Some(JsonValue::Other(ValueKind::Number))
    }

    fn parse_string(&mut self) -> Option<Vec<u8>> {
        if !self.consume(b'"') {
            return None;
        }
        let mut out = Vec::new();
        while let Some(b) = self.peek() {
            self.pos += 1;
            match b {
                b'"' => {
                    std::str::from_utf8(&out).ok()?;
                    return Some(out);
                }
                b'\\' => self.parse_escape_into(&mut out)?,
                0x00..=0x1f => return None,
                _ => out.push(b),
            }
        }
        None
    }

    fn parse_escape_into(&mut self, out: &mut Vec<u8>) -> Option<()> {
        let escaped = self.peek()?;
        self.pos += 1;
        match escaped {
            b'"' | b'\\' | b'/' => out.push(escaped),
            b'b' => out.push(0x08),
            b'f' => out.push(0x0c),
            b'n' => out.push(b'\n'),
            b'r' => out.push(b'\r'),
            b't' => out.push(b'\t'),
            b'u' => {
                let scalar = self.parse_unicode_scalar()?;
                let mut buf = [0; 4];
                out.extend_from_slice(scalar.encode_utf8(&mut buf).as_bytes());
            }
            _ => return None,
        };
        Some(())
    }

    fn parse_unicode_escape_value(&mut self) -> Option<u32> {
        let mut value = 0u32;
        for _ in 0..4 {
            value = (value << 4) | u32::from(hex_value(self.peek()?)?);
            self.pos += 1;
        }
        Some(value)
    }

    fn parse_unicode_scalar(&mut self) -> Option<char> {
        let value = self.parse_unicode_escape_value()?;
        let scalar = if (0xd800..=0xdbff).contains(&value) {
            if !self.consume(b'\\') || !self.consume(b'u') {
                return None;
            }
            let low = self.parse_unicode_escape_value()?;
            if !(0xdc00..=0xdfff).contains(&low) {
                return None;
            }
            0x10000 + (((value - 0xd800) << 10) | (low - 0xdc00))
        } else if (0xdc00..=0xdfff).contains(&value) {
            return None;
        } else {
            value
        };
        char::from_u32(scalar)
    }

    fn expect_bytes(&mut self, expected: &[u8]) -> Option<()> {
        if self.input.get(self.pos..self.pos + expected.len()) == Some(expected) {
            self.pos += expected.len();
            Some(())
        } else {
            None
        }
    }
}

fn hex_value(b: u8) -> Option<u8> {
    match b {
        b'0'..=b'9' => Some(b - b'0'),
        b'a'..=b'f' => Some(b - b'a' + 10),
        b'A'..=b'F' => Some(b - b'A' + 10),
        _ => None,
    }
}

#[cfg(test)]
mod tests {
    use super::{
        adr_mode, adr_sections_json, architecture_aspect_wanted, bm25_file_pattern_like,
        bm25_match_query, bool_arg, cancel_request_matches, content_length_header_is_blank,
        content_length_header_matches, content_length_raw_value, content_length_response,
        content_length_value, detect_changes_impacted_label, detect_changes_status_path_offset,
        detect_changes_wants_symbols, edge_type_valid, index_mode, initialize_protocol_version,
        initialize_response, int_arg, jsonrpc_format_error, jsonrpc_format_response,
        jsonrpc_request_summary, mcp_text_result, method_kind, method_not_found_error,
        missing_project_error, missing_tool_name_message, node_resolution_score,
        parse_error_message, parse_file_uri, ping_result, project_db_file_name, project_list_error,
        project_not_found_message, sanitize_ascii_in_place, sanitize_utf8_lossy, search_args_valid,
        search_code_score, search_line_match_span, search_mode, search_path_arg_valid,
        search_pick_resolved_index, search_pick_tightest_index, search_score_cmp,
        search_strip_root_prefix_offset, search_top_dir, string_arg, tool_count, tool_description,
        tool_dispatch_index, tool_index, tool_input_schema, tool_name, tool_output_schema,
        tool_title, tools_call_arguments, tools_call_default_arguments, tools_call_name,
        tools_cursor_offset, tools_list_json, tools_page_bounds, trace_is_test_file,
        trace_mode_edge_mask, unknown_tool_message, utf8_is_cont_byte, MethodKind, ToolsPageBounds,
    };

    fn summary(input: &[u8]) -> String {
        String::from_utf8(jsonrpc_request_summary(input).unwrap()).unwrap()
    }

    #[test]
    fn initialize_protocol_version_matches_c_contract() {
        let f = |s: Option<&str>| {
            String::from_utf8(initialize_protocol_version(s.map(str::as_bytes))).unwrap()
        };
        assert_eq!(f(None), "2025-11-25");
        assert_eq!(f(Some("{}")), "2025-11-25");
        assert_eq!(
            f(Some("{\"protocolVersion\":\"2024-11-05\"}")),
            "2024-11-05"
        );
        assert_eq!(
            f(Some("{\"protocolVersion\":\"2025-06-18\"}")),
            "2025-06-18"
        );
        assert_eq!(
            f(Some("{\"protocolVersion\":\"2025-03-26\"}")),
            "2025-03-26"
        );
        assert_eq!(
            f(Some("{\"protocolVersion\":\"2025-11-25\"}")),
            "2025-11-25"
        );
        assert_eq!(
            f(Some("{\"protocolVersion\":\"9999-01-01\"}")),
            "2025-11-25"
        );
        assert_eq!(f(Some("{\"protocolVersion\":7}")), "2025-11-25");
        assert_eq!(f(Some("[]")), "2025-11-25");
        assert_eq!(f(Some("not json")), "2025-11-25");
        assert_eq!(f(Some("{} trailing")), "2025-11-25");
        assert_eq!(
            f(Some(
                "{\"protocolVersion\":\"2024-11-05\",\"protocolVersion\":\"2025-11-25\"}"
            )),
            "2024-11-05"
        );
        assert_eq!(
            f(Some(
                "{\"protocolVersion\":7,\"protocolVersion\":\"2024-11-05\"}"
            )),
            "2025-11-25"
        );
    }

    #[test]
    fn initialize_response_matches_c_contract() {
        let f =
            |s: Option<&str>| String::from_utf8(initialize_response(s.map(str::as_bytes))).unwrap();
        let latest = concat!(
            r#"{"protocolVersion":"2025-11-25","#,
            r#""serverInfo":{"name":"codebase-memory-mcp","version":"0.10.0"},"#,
            r#""capabilities":{"tools":{"listChanged":false}}}"#
        );
        assert_eq!(f(None), latest);
        assert_eq!(f(Some("{\"capabilities\":{}}")), latest);
        assert_eq!(f(Some("{\"protocolVersion\":\"9999-01-01\"}")), latest);
        assert_eq!(f(Some("{\"protocolVersion\":20250618}")), latest);
        assert_eq!(f(Some("[]")), latest);

        assert_eq!(
            f(Some("{\"protocolVersion\":\"2024-11-05\"}")),
            concat!(
                r#"{"protocolVersion":"2024-11-05","#,
                r#""serverInfo":{"name":"codebase-memory-mcp","version":"0.10.0"},"#,
                r#""capabilities":{"tools":{"listChanged":false}}}"#
            )
        );
        assert_eq!(
            f(Some(
                "{\"protocolVersion\":\"2024-11-05\",\"protocolVersion\":\"2025-11-25\"}"
            )),
            concat!(
                r#"{"protocolVersion":"2024-11-05","#,
                r#""serverInfo":{"name":"codebase-memory-mcp","version":"0.10.0"},"#,
                r#""capabilities":{"tools":{"listChanged":false}}}"#
            )
        );
    }

    #[test]
    fn tool_index_matches_c_tool_order() {
        let names = [
            "index_repository",
            "search_graph",
            "query_graph",
            "trace_path",
            "get_code_snippet",
            "get_graph_schema",
            "get_architecture",
            "search_code",
            "list_projects",
            "delete_project",
            "index_status",
            "detect_changes",
            "manage_adr",
            "ingest_traces",
        ];
        for (idx, name) in names.iter().enumerate() {
            assert_eq!(tool_index(Some(name.as_bytes())), Some(idx));
        }
        assert_eq!(tool_index(None), None);
        assert_eq!(tool_index(Some(b"")), None);
        assert_eq!(tool_index(Some(b"Search_Graph")), None);
        assert_eq!(tool_index(Some(b"unknown_tool")), None);
    }

    #[test]
    fn tool_dispatch_index_matches_c_dispatch_aliases() {
        assert_eq!(tool_dispatch_index(Some(b"trace_path")), Some(3));
        assert_eq!(tool_dispatch_index(Some(b"trace_call_path")), Some(3));
        assert_eq!(tool_dispatch_index(Some(b"search_graph")), Some(1));
        assert_eq!(tool_dispatch_index(None), None);
        assert_eq!(tool_dispatch_index(Some(b"")), None);
        assert_eq!(tool_dispatch_index(Some(b"Trace_Call_Path")), None);
        assert_eq!(tool_dispatch_index(Some(b"unknown_tool")), None);
    }

    #[test]
    fn tool_name_matches_c_tool_order() {
        assert_eq!(tool_count(), 14);
        assert_eq!(tool_name(0), Some(b"index_repository".as_slice()));
        assert_eq!(tool_name(7), Some(b"search_code".as_slice()));
        assert_eq!(tool_name(13), Some(b"ingest_traces".as_slice()));
        assert_eq!(tool_name(-1), None);
        assert_eq!(tool_name(14), None);
    }

    #[test]
    fn tool_title_matches_c_tool_order() {
        assert_eq!(tool_title(0), Some(b"Index repository".as_slice()));
        assert_eq!(tool_title(7), Some(b"Search code".as_slice()));
        assert_eq!(tool_title(13), Some(b"Ingest traces".as_slice()));
        assert_eq!(tool_title(-1), None);
        assert_eq!(tool_title(14), None);
    }

    #[test]
    fn tool_description_matches_c_tool_order() {
        assert_eq!(
            tool_description(5),
            Some(b"Get the schema of the knowledge graph (node labels, edge types)".as_slice())
        );
        let search = tool_description(1).unwrap();
        assert!(search.starts_with(b"Search the code knowledge graph"));
        assert!(search
            .windows(b"Detect truncation with has_more".len())
            .any(|window| window == b"Detect truncation with has_more"));
        let query = tool_description(2).unwrap();
        assert!(query
            .windows(b"COMPLEXITY / BOTTLENECKS".len())
            .any(|window| window == b"COMPLEXITY / BOTTLENECKS"));
        assert_eq!(
            tool_description(13),
            Some(b"Ingest runtime traces to enhance the knowledge graph".as_slice())
        );
        assert_eq!(tool_description(-1), None);
        assert_eq!(tool_description(14), None);
    }

    #[test]
    fn tool_input_schema_matches_c_tool_order() {
        assert_eq!(
            tool_input_schema(8),
            Some(br#"{"type":"object","properties":{}}"#.as_slice())
        );
        let index = tool_input_schema(0).unwrap();
        assert!(index.starts_with(br#"{"type":"object","properties":{"repo_path""#));
        assert!(index
            .windows(br#""required":["repo_path"]"#.len())
            .any(|window| window == br#""required":["repo_path"]"#));
        let search = tool_input_schema(1).unwrap();
        let camel_case_hint = "updateCloudClient → update, cloud, client".as_bytes();
        assert!(search
            .windows(camel_case_hint.len())
            .any(|window| window == camel_case_hint));
        let search_code = tool_input_schema(7).unwrap();
        assert!(search_code
            .windows(br#"\\.(go|ts)$"#.len())
            .any(|window| window == br#"\\.(go|ts)$"#));
        assert_eq!(tool_input_schema(-1), None);
        assert_eq!(tool_input_schema(14), None);
    }

    #[test]
    fn tool_output_schema_matches_c_contract() {
        assert_eq!(
            tool_output_schema(),
            br#"{"type":"object","additionalProperties":true}"#.as_slice()
        );
    }

    #[test]
    fn tools_page_bounds_matches_c_range_contract() {
        let f = |offset, limit, include| tools_page_bounds(offset, limit, include, 14);
        assert_eq!(
            f(0, 14, false),
            ToolsPageBounds {
                start: 0,
                end: 14,
                has_next: false,
                next_cursor: 0,
            }
        );
        assert_eq!(
            f(0, 8, true),
            ToolsPageBounds {
                start: 0,
                end: 8,
                has_next: true,
                next_cursor: 8,
            }
        );
        assert_eq!(
            f(8, 8, true),
            ToolsPageBounds {
                start: 8,
                end: 14,
                has_next: false,
                next_cursor: 0,
            }
        );
        assert_eq!(f(-5, 3, true).start, 0);
        assert_eq!(f(99, 8, true).start, 14);
        assert_eq!(f(99, 8, true).end, 14);
        assert_eq!(f(2, -1, false).end, 14);
        assert_eq!(f(2, 99, false).end, 14);
        assert_eq!(tools_page_bounds(0, 5, true, -1).end, 0);
    }

    #[test]
    fn tools_list_json_matches_c_contract_shape() {
        let full = String::from_utf8(tools_list_json(0, 14, false)).unwrap();
        assert!(full.starts_with(r#"{"tools":[{"name":"index_repository""#));
        assert!(full.contains(r#""title":"Search graph""#));
        assert!(full.contains(r#""description":"Search the code knowledge graph"#));
        assert!(full.contains(r#""inputSchema":{"type":"object""#));
        assert!(full.contains(r#""repo_path":{"type":"string""#));
        assert!(full.contains(r#""required":["repo_path"]"#));
        assert!(full.contains(r#""outputSchema":{"type":"object","additionalProperties":true}"#));
        assert!(full.ends_with("]}"));
        assert!(!full.contains("nextCursor"));

        let page = String::from_utf8(tools_list_json(0, 8, true)).unwrap();
        assert!(page.starts_with(r#"{"tools":[{"name":"index_repository""#));
        assert!(page.contains(r#""name":"search_code""#));
        assert!(!page.contains(r#""name":"list_projects""#));
        assert!(page.ends_with(r#","nextCursor":"8"}"#));

        let empty = String::from_utf8(tools_list_json(14, 8, true)).unwrap();
        assert_eq!(empty, r#"{"tools":[]}"#);
    }

    #[test]
    fn content_length_header_matches_c_framing_classifier() {
        let f = |line: Option<&str>| content_length_header_matches(line.map(str::as_bytes));
        assert!(f(Some("Content-Length: 42")));
        assert!(f(Some("Content-Length:42")));
        assert!(f(Some("Content-Length:")));
        assert!(f(Some("Content-Length:abc")));
        assert!(!f(Some("content-length:5")));
        assert!(!f(Some(" Content-Length:5")));
        assert!(!f(Some("Content-Length")));
        assert!(!f(Some("Content-Type: application/json")));
        assert!(!f(Some("")));
        assert!(!f(None));
    }

    #[test]
    fn content_length_value_matches_c_transport_gate() {
        const MAX: i64 = 10 * 1024 * 1024;
        let f = |line: Option<&str>| content_length_value(line.map(str::as_bytes), MAX);
        assert_eq!(f(Some("Content-Length: 42")), 42);
        assert_eq!(f(Some("Content-Length:42")), 42);
        assert_eq!(f(Some("Content-Length:\t+7")), 7);
        assert_eq!(f(Some("Content-Length:12 trailing")), 12);
        assert_eq!(f(Some("Content-Length:0")), 0);
        assert_eq!(f(Some("Content-Length:-1")), 0);
        assert_eq!(f(Some("Content-Length:")), 0);
        assert_eq!(f(Some("Content-Length:abc")), 0);
        assert_eq!(f(Some("content-length:5")), 0);
        assert_eq!(f(Some("Content-Length:10485760")), 10_485_760);
        assert_eq!(f(Some("Content-Length:10485761")), 0);
        assert_eq!(f(None), 0);
    }

    #[test]
    fn content_length_raw_value_matches_c_transport_parser() {
        let f = |line: Option<&str>| content_length_raw_value(line.map(str::as_bytes));
        assert_eq!(f(Some("Content-Length: 42")), 42);
        assert_eq!(f(Some("Content-Length:42")), 42);
        assert_eq!(f(Some("Content-Length:\t+7")), 7);
        assert_eq!(f(Some("Content-Length:12 trailing")), 12);
        assert_eq!(f(Some("Content-Length:0")), 0);
        assert_eq!(f(Some("Content-Length:-1")), -1);
        assert_eq!(f(Some("Content-Length:")), -1);
        assert_eq!(f(Some("Content-Length:abc")), -1);
        assert_eq!(f(Some("content-length:5")), -1);
        assert_eq!(f(None), -1);
    }

    #[test]
    fn content_length_header_blank_matches_c_separator_contract() {
        let f = |line: Option<&str>| content_length_header_is_blank(line.map(str::as_bytes));
        assert!(f(Some("")));
        assert!(f(Some("\n")));
        assert!(f(Some("\r\n")));
        assert!(f(Some("\r\r\n")));
        assert!(!f(Some(" ")));
        assert!(!f(Some(" \r\n")));
        assert!(!f(Some("Content-Type: application/json\r\n")));
        assert!(!f(None));
    }

    #[test]
    fn content_length_response_matches_c_transport_framing() {
        assert_eq!(
            content_length_response(Some(br#"{"ok":true}"#)).unwrap(),
            b"Content-Length: 11\r\n\r\n{\"ok\":true}"
        );
        assert_eq!(
            content_length_response(Some(b"")).unwrap(),
            b"Content-Length: 0\r\n\r\n"
        );
        assert_eq!(
            content_length_response(Some("中".as_bytes())).unwrap(),
            b"Content-Length: 3\r\n\r\n\xe4\xb8\xad"
        );
        assert!(content_length_response(None).is_none());
    }

    #[test]
    fn parse_file_uri_matches_c_contract() {
        let f = |uri: Option<&str>| {
            parse_file_uri(uri.map(str::as_bytes))
                .map(|path| String::from_utf8(path.to_vec()).unwrap())
        };
        assert_eq!(
            f(Some("file:///home/user/project")).as_deref(),
            Some("/home/user/project")
        );
        assert_eq!(f(Some("file:///tmp/test")).as_deref(), Some("/tmp/test"));
        assert_eq!(f(Some("file:///")).as_deref(), Some("/"));
        assert_eq!(
            f(Some("file:///C:/Users/project")).as_deref(),
            Some("C:/Users/project")
        );
        assert_eq!(
            f(Some("file:///d:/Projects/myapp")).as_deref(),
            Some("d:/Projects/myapp")
        );
        assert_eq!(
            f(Some("file:///home/user/my%20project")).as_deref(),
            Some("/home/user/my%20project")
        );
        assert_eq!(
            f(Some("file://server/share")).as_deref(),
            Some("server/share")
        );
        assert_eq!(f(Some("file://")).as_deref(), Some(""));
        assert!(f(Some("http://example.com/path")).is_none());
        assert!(f(Some("")).is_none());
        assert!(f(None).is_none());
    }

    #[test]
    fn method_kind_matches_c_dispatch_contract() {
        let f = |method: Option<&str>| method_kind(method.map(str::as_bytes));
        assert_eq!(f(Some("initialize")), MethodKind::Initialize);
        assert_eq!(f(Some("ping")), MethodKind::Ping);
        assert_eq!(f(Some("tools/list")), MethodKind::ToolsList);
        assert_eq!(f(Some("tools/call")), MethodKind::ToolsCall);
        assert_eq!(
            f(Some("notifications/cancelled")),
            MethodKind::NotificationsCancelled
        );
        assert_eq!(f(Some("Initialize")), MethodKind::Unknown);
        assert_eq!(f(Some("tools/list ")), MethodKind::Unknown);
        assert_eq!(f(Some("")), MethodKind::Unknown);
        assert_eq!(f(None), MethodKind::Unknown);
    }

    #[test]
    fn method_not_found_error_matches_c_contract() {
        assert_eq!(
            method_not_found_error(),
            br#"{"code":-32601,"message":"Method not found"}"#
        );
    }

    #[test]
    fn parse_error_message_matches_c_contract() {
        assert_eq!(parse_error_message(), b"Parse error");
    }

    #[test]
    fn ping_result_matches_c_contract() {
        assert_eq!(ping_result(), b"{}");
    }

    #[test]
    fn tools_call_default_arguments_matches_c_contract() {
        assert_eq!(tools_call_default_arguments(), b"{}");
    }

    #[test]
    fn missing_tool_name_message_matches_c_contract() {
        assert_eq!(missing_tool_name_message(), b"missing tool name");
    }

    #[test]
    fn missing_project_error_matches_c_contract() {
        assert_eq!(
            missing_project_error(),
            br#"{"error":"missing required argument: project","hint":"Pass the project as the \"project\" argument, e.g. {\"project\":\"<name from list_projects>\"}. Run list_projects to see indexed projects."}"#
        );
    }

    #[test]
    fn project_not_found_message_matches_c_contract() {
        assert_eq!(project_not_found_message(), b"project not found");
    }

    #[test]
    fn project_list_error_matches_c_contract() {
        assert!(project_list_error(None, Some(br#""a""#), 1).is_none());
        assert!(project_list_error(Some(b"reason"), None, 1).is_none());
        assert_eq!(
            project_list_error(Some(b"project not found or not indexed"), Some(br#""a","b""#), 2)
                .unwrap(),
            br#"{"error":"project not found or not indexed","hint":"Use list_projects to see all indexed projects, then pass it as the \"project\" argument.","available_projects":["a","b"],"count":2}"#
        );
        assert_eq!(
            project_list_error(Some(b"project not found or not indexed"), Some(br#""a""#), 0)
                .unwrap(),
            br#"{"error":"project not found or not indexed","hint":"No projects indexed yet. Call index_repository first."}"#
        );
        assert_eq!(
            project_list_error(Some(b"project not found or not indexed"), None, 0).unwrap(),
            br#"{"error":"project not found or not indexed","hint":"No projects indexed yet. Call index_repository first."}"#
        );
    }

    #[test]
    fn unknown_tool_message_matches_c_contract() {
        assert_eq!(
            unknown_tool_message(Some(b"nonexistent_tool")).unwrap(),
            b"unknown tool: nonexistent_tool"
        );
        assert_eq!(
            unknown_tool_message(Some(b"tool \"quote\"")).unwrap(),
            b"unknown tool: tool \"quote\""
        );
        assert!(unknown_tool_message(None).is_none());
    }

    #[test]
    fn tools_cursor_offset_matches_c_contract() {
        const TC: i64 = 14;
        let f = |s: &str| tools_cursor_offset(Some(s.as_bytes()), TC);
        // null / malformed JSON / 尾端殘留 → 0
        assert_eq!(tools_cursor_offset(None, TC), 0);
        assert_eq!(f(""), 0);
        assert_eq!(f("{"), 0);
        assert_eq!(f("{\"cursor\":}"), 0);
        assert_eq!(f("not json"), 0);
        assert_eq!(f("{} trailing"), 0);
        // 合法 JSON 但 root 非物件或無 cursor → 0
        assert_eq!(f("{}"), 0);
        assert_eq!(f("{\"limit\":5}"), 0);
        assert_eq!(f("[]"), 0);
        assert_eq!(f("5"), 0);
        // cursor 存在但非有效非負整數字串 → tool_count
        assert_eq!(f("{\"cursor\":\"\"}"), TC);
        assert_eq!(f("{\"cursor\":\"abc\"}"), TC);
        assert_eq!(f("{\"cursor\":5}"), TC);
        assert_eq!(f("{\"cursor\":null}"), TC);
        assert_eq!(f("{\"cursor\":\"-1\"}"), TC);
        assert_eq!(f("{\"cursor\":\"5 \"}"), TC);
        assert_eq!(f("{\"cursor\":\"0x5\"}"), TC);
        assert_eq!(f("{\"cursor\":\"99999999999999999999\"}"), TC);
        // 有效 cursor → clamp 到 tool_count
        assert_eq!(f("{\"cursor\":\"0\"}"), 0);
        assert_eq!(f("{\"cursor\":\"3\"}"), 3);
        assert_eq!(f("{\"cursor\":\" 4\"}"), 4);
        assert_eq!(f("{\"cursor\":\"+6\"}"), 6);
        assert_eq!(f("{\"cursor\":\"-0\"}"), 0);
        assert_eq!(f("{\"cursor\":\"100\"}"), TC);
        assert_eq!(f("{\"cursor\":\"9223372036854775807\"}"), TC);
        // 首個 cursor 勝出（重複鍵）
        assert_eq!(f("{\"cursor\":\"2\",\"cursor\":\"9\"}"), 2);
    }

    #[test]
    fn tools_call_name_matches_dispatch_contract() {
        let f = |s: &str| tools_call_name(s.as_bytes()).map(|v| String::from_utf8(v).unwrap());
        assert_eq!(
            f(r#"{"name":"search_graph","arguments":{"limit":5}}"#).as_deref(),
            Some("search_graph")
        );
        assert_eq!(
            f(r#"{"arguments":{},"name":"query_graph"}"#).as_deref(),
            Some("query_graph")
        );
        assert_eq!(f(r#"{"name":"\u641c\u5c0b"}"#).as_deref(), Some("搜尋"));
        assert_eq!(
            f(r#"{"name":"first","name":"second"}"#).as_deref(),
            Some("first")
        );
        assert!(f("{}").is_none());
        assert!(f(r#"{"name":7}"#).is_none());
        assert!(f(r#"{"name":7,"name":"late"}"#).is_none());
        assert!(tools_call_name(b"not json").is_none());
        assert!(tools_call_name(b"[]").is_none());
        assert!(tools_call_name(b"{\"name\":\"bad\xff\"}").is_none());
    }

    #[test]
    fn tools_call_arguments_matches_wrapper_contract() {
        let f = |s: &str| tools_call_arguments(s.as_bytes()).map(|v| String::from_utf8(v).unwrap());
        assert_eq!(
            f(r#"{"name":"search_graph","arguments":{"limit":5}}"#).as_deref(),
            Some(r#"{"limit":5}"#)
        );
        assert_eq!(f(r#"{"name":"tool"}"#).as_deref(), Some("{}"));
        assert_eq!(f(r#"{}"#).as_deref(), Some("{}"));
        assert_eq!(
            f(r#"{"arguments":[1,true,null]}"#).as_deref(),
            Some("[1,true,null]")
        );
        assert_eq!(f(r#"{"arguments":"raw"}"#).as_deref(), Some(r#""raw""#));
        assert_eq!(
            f(r#"{"arguments":{"first":1},"arguments":{"second":2}}"#).as_deref(),
            Some(r#"{"first":1}"#)
        );
        assert_eq!(
            f(r#"{"arguments" : { "spaced" : true } }"#).as_deref(),
            Some(r#"{ "spaced" : true }"#)
        );
        assert!(f("").is_none());
        assert!(f("not json").is_none());
        assert!(f("[]").is_none());
        assert!(tools_call_arguments(b"{\"arguments\":\"bad\xff\"}").is_none());
    }

    #[test]
    fn string_arg_matches_c_argument_contract() {
        let f = |json: &str, key: &str| {
            string_arg(json.as_bytes(), key.as_bytes()).map(|v| String::from_utf8(v).unwrap())
        };
        assert_eq!(
            f(
                r#"{"label":"Function","name_pattern":".*Order.*"}"#,
                "label"
            )
            .as_deref(),
            Some("Function")
        );
        assert_eq!(
            f(
                r#"{"label":"Function","name_pattern":".*Order.*"}"#,
                "name_pattern"
            )
            .as_deref(),
            Some(".*Order.*")
        );
        assert_eq!(
            f(r#"{"name":"\u641c\u5c0b"}"#, "name").as_deref(),
            Some("搜尋")
        );
        assert_eq!(
            f(r#"{"name":"first","name":"second"}"#, "name").as_deref(),
            Some("first")
        );
        assert!(f(r#"{}"#, "key").is_none());
        assert!(f(r#"{"config":{"nested":true}}"#, "config").is_none());
        assert!(f(r#"{"count":42}"#, "count").is_none());
        assert!(f(r#"{"name":7,"name":"late"}"#, "name").is_none());
        assert!(string_arg(b"", b"key").is_none());
        assert!(string_arg(b"not json", b"key").is_none());
        assert!(string_arg(b"[]", b"key").is_none());
        assert!(string_arg(b"{\"name\":\"bad\xff\"}", b"name").is_none());
        assert!(string_arg(br#"{"name":"value"}"#, b"").is_none());
    }

    #[test]
    fn int_and_bool_args_match_c_argument_contract() {
        let int = |json: &str, key: &str, default_value| {
            int_arg(json.as_bytes(), key.as_bytes(), default_value)
        };
        assert_eq!(int(r#"{"limit":10,"offset":5}"#, "limit", 0), 10);
        assert_eq!(int(r#"{"limit":10,"offset":5}"#, "offset", 0), 5);
        assert_eq!(int(r#"{"limit":10}"#, "missing", 42), 42);
        assert_eq!(int(r#"{"limit":"ten"}"#, "limit", 5), 5);
        assert_eq!(int(r#"{"flag":true}"#, "flag", -1), -1);
        assert_eq!(int(r#"{"limit":7,"limit":8}"#, "limit", 0), 7);
        assert_eq!(int(r#"{"limit":"ten","limit":8}"#, "limit", 5), 5);
        assert_eq!(int(r#"{"limit":7.5}"#, "limit", 5), 5);
        assert_eq!(int(r#"{"limit":7} trailing"#, "limit", 5), 5);
        assert_eq!(int("", "limit", 5), 5);
        assert_eq!(int("not json", "limit", 5), 5);
        assert_eq!(int("[]", "limit", 5), 5);
        assert_eq!(int(r#"{"limit":7}"#, "", 5), 5);

        let bool_value = |json: &str, key: &str| bool_arg(json.as_bytes(), key.as_bytes());
        assert!(bool_value(
            r#"{"include_connected":true,"regex":false}"#,
            "include_connected"
        ));
        assert!(!bool_value(
            r#"{"include_connected":true,"regex":false}"#,
            "regex"
        ));
        assert!(!bool_value(r#"{"include_connected":true}"#, "missing"));
        assert!(!bool_value(r#"{"flag":1}"#, "flag"));
        assert!(!bool_value(r#"{"flag":"true"}"#, "flag"));
        assert!(bool_value(r#"{"flag":true,"flag":false}"#, "flag"));
        assert!(!bool_value(r#"{"flag":1,"flag":true}"#, "flag"));
        assert!(!bool_arg(b"{\"flag\":\"bad\xff\"}", b"flag"));
        assert!(!bool_value(r#"{"flag":true} trailing"#, "flag"));
        assert!(!bool_value("", "flag"));
        assert!(!bool_value("not json", "flag"));
        assert!(!bool_value("[]", "flag"));
        assert!(!bool_value(r#"{"flag":true}"#, ""));
    }

    #[test]
    fn edge_type_valid_matches_c_contract() {
        assert!(!edge_type_valid(None));
        assert!(edge_type_valid(Some(b"")));
        assert!(edge_type_valid(Some(b"CALLS")));
        assert!(edge_type_valid(Some(b"DEFINES_METHOD")));
        assert!(edge_type_valid(Some(b"_")));
        assert!(!edge_type_valid(Some(b"calls")));
        assert!(!edge_type_valid(Some(b"CALLS-TO")));
        assert!(!edge_type_valid(Some(b"CALLS1")));
        assert!(!edge_type_valid(Some(b"CALLS ")));
        assert!(!edge_type_valid(Some("呼叫".as_bytes())));
        assert!(edge_type_valid(Some(
            b"ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKL"
        )));
        assert!(!edge_type_valid(Some(
            b"ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLM"
        )));
    }

    #[test]
    fn search_path_arg_valid_matches_c_contract() {
        assert!(!search_path_arg_valid(None));
        assert!(search_path_arg_valid(Some(b"")));
        assert!(search_path_arg_valid(Some(b"src/main.go")));
        assert!(search_path_arg_valid(Some(b"*R&D*.go")));
        assert!(search_path_arg_valid(Some(b"apps/foo bar/*.ts")));
        for bad in [
            b"'".as_slice(),
            b"\"".as_slice(),
            b";".as_slice(),
            b"|".as_slice(),
            b"$".as_slice(),
            b"`".as_slice(),
            b"<".as_slice(),
            b">".as_slice(),
            b"\n".as_slice(),
            b"\r".as_slice(),
        ] {
            assert!(!search_path_arg_valid(Some(bad)));
        }
        #[cfg(not(windows))]
        assert!(!search_path_arg_valid(Some(br"src\main.go")));
        #[cfg(windows)]
        assert!(search_path_arg_valid(Some(br"src\main.go")));
    }

    #[test]
    fn search_args_valid_matches_c_contract() {
        assert!(!search_args_valid(None, None));
        assert!(search_args_valid(Some(b""), None));
        assert!(search_args_valid(Some(b"src/main.go"), None));
        assert!(search_args_valid(Some(b"src/main.go"), Some(b"*.go")));
        assert!(search_args_valid(Some(b"apps/foo bar"), Some(b"*R&D*.go")));
        assert!(!search_args_valid(Some(b"src/main.go"), Some(b";")));
        assert!(!search_args_valid(Some(b"'"), Some(b"*.go")));
        assert!(!search_args_valid(
            Some(b"src/main.go"),
            Some(b"bad\npattern")
        ));
    }

    #[test]
    fn search_mode_matches_c_contract() {
        assert_eq!(search_mode(None), 0);
        assert_eq!(search_mode(Some(b"")), 0);
        assert_eq!(search_mode(Some(b"compact")), 0);
        assert_eq!(search_mode(Some(b"full")), 1);
        assert_eq!(search_mode(Some(b"files")), 2);
        assert_eq!(search_mode(Some(b"Full")), 0);
        assert_eq!(search_mode(Some(b"files ")), 0);
        assert_eq!(search_mode(Some(b"unknown")), 0);
    }

    #[test]
    fn index_mode_matches_c_contract() {
        assert_eq!(index_mode(None), 0);
        assert_eq!(index_mode(Some(b"")), 0);
        assert_eq!(index_mode(Some(b"full")), 0);
        assert_eq!(index_mode(Some(b"moderate")), 1);
        assert_eq!(index_mode(Some(b"fast")), 2);
        assert_eq!(index_mode(Some(b"cross-repo-intelligence")), 3);
        assert_eq!(index_mode(Some(b"Fast")), 0);
        assert_eq!(index_mode(Some(b"fast ")), 0);
        assert_eq!(index_mode(Some(b"unknown")), 0);
    }

    #[test]
    fn trace_mode_edge_mask_matches_c_defaults() {
        const CALLS: u32 = 1 << 0;
        const DATA_FLOWS: u32 = 1 << 1;
        const HTTP_CALLS: u32 = 1 << 2;
        const ASYNC_CALLS: u32 = 1 << 3;
        const CROSS_HTTP_CALLS: u32 = 1 << 4;
        const CROSS_ASYNC_CALLS: u32 = 1 << 5;
        const CROSS_CHANNEL: u32 = 1 << 6;
        const CROSS_GRPC_CALLS: u32 = 1 << 7;
        const CROSS_GRAPHQL_CALLS: u32 = 1 << 8;
        const CROSS_TRPC_CALLS: u32 = 1 << 9;
        const CROSS_SERVICE: u32 = HTTP_CALLS
            | ASYNC_CALLS
            | DATA_FLOWS
            | CALLS
            | CROSS_HTTP_CALLS
            | CROSS_ASYNC_CALLS
            | CROSS_CHANNEL
            | CROSS_GRPC_CALLS
            | CROSS_GRAPHQL_CALLS
            | CROSS_TRPC_CALLS;

        assert_eq!(trace_mode_edge_mask(None), CALLS);
        assert_eq!(trace_mode_edge_mask(Some(b"")), CALLS);
        assert_eq!(trace_mode_edge_mask(Some(b"calls")), CALLS);
        assert_eq!(trace_mode_edge_mask(Some(b"data_flow")), CALLS | DATA_FLOWS);
        assert_eq!(trace_mode_edge_mask(Some(b"cross_service")), CROSS_SERVICE);
        assert_eq!(trace_mode_edge_mask(Some(b"Data_Flow")), CALLS);
        assert_eq!(trace_mode_edge_mask(Some(b"data_flow ")), CALLS);
        assert_eq!(trace_mode_edge_mask(Some(b"unknown")), CALLS);
    }

    #[test]
    fn sanitize_ascii_in_place_matches_c_contract() {
        let mut plain = b"abc XYZ 123 \n\t".to_vec();
        sanitize_ascii_in_place(&mut plain);
        assert_eq!(plain, b"abc XYZ 123 \n\t");

        let mut mixed = vec![b'a', 0x7f, 0x80, 0xc3, 0xa9, b'z', 0xff];
        sanitize_ascii_in_place(&mut mixed);
        assert_eq!(mixed, vec![b'a', 0x7f, b'?', b'?', b'?', b'z', b'?']);

        let mut empty = Vec::new();
        sanitize_ascii_in_place(&mut empty);
        assert!(empty.is_empty());
    }

    #[test]
    fn search_code_score_matches_c_contract() {
        assert_eq!(search_code_score(None, None, 7), 7);
        assert_eq!(
            search_code_score(Some(b"Function"), Some(b"src/app.c"), 3),
            13
        );
        assert_eq!(
            search_code_score(Some(b"Method"), Some(b"src/app.c"), 3),
            13
        );
        assert_eq!(search_code_score(Some(b"Route"), Some(b"src/app.c"), 3), 18);
        assert_eq!(search_code_score(Some(b"Class"), Some(b"src/app.c"), 3), 3);
        assert_eq!(
            search_code_score(Some(b"function"), Some(b"src/app.c"), 3),
            3
        );
        assert_eq!(
            search_code_score(Some(b"Function"), Some(b"vendored/lib.c"), 3),
            -37
        );
        assert_eq!(
            search_code_score(Some(b"Function"), Some(b"vendor/lib.c"), 3),
            -37
        );
        assert_eq!(
            search_code_score(Some(b"Function"), Some(b"node_modules/pkg/index.js"), 3),
            -37
        );
        assert_eq!(
            search_code_score(Some(b"Function"), Some(b"src/test_helper.c"), 3),
            8
        );
        assert_eq!(
            search_code_score(Some(b"Function"), Some(b"src/spec/helper.c"), 3),
            8
        );
        assert_eq!(
            search_code_score(Some(b"Function"), Some(b"src/foo_test.c"), 3),
            8
        );
        assert_eq!(
            search_code_score(Some(b"Route"), Some(b"vendor/test_api.c"), 3),
            -37
        );
    }

    #[test]
    fn search_score_cmp_matches_c_contract() {
        assert_eq!(search_score_cmp(20, 10), -10);
        assert_eq!(search_score_cmp(10, 20), 10);
        assert_eq!(search_score_cmp(7, 7), 0);
        assert_eq!(search_score_cmp(-5, 5), 10);
        assert_eq!(search_score_cmp(5, -5), -10);
    }

    #[test]
    fn search_top_dir_matches_c_contract() {
        assert_eq!(search_top_dir(None), None);
        assert_eq!(search_top_dir(Some(b"")), Some(Vec::new()));
        assert_eq!(search_top_dir(Some(b"src/app.c")), Some(b"src/".to_vec()));
        assert_eq!(
            search_top_dir(Some(b"src/nested/app.c")),
            Some(b"src/".to_vec())
        );
        assert_eq!(search_top_dir(Some(b"app.c")), Some(b"app.c".to_vec()));
        assert_eq!(search_top_dir(Some(b"/abs/app.c")), Some(b"/".to_vec()));
    }

    #[test]
    fn search_strip_root_prefix_offset_matches_c_contract() {
        assert_eq!(search_strip_root_prefix_offset(None, Some(b"/repo"), 5), 0);
        assert_eq!(
            search_strip_root_prefix_offset(Some(b"/repo/a.c"), None, 5),
            0
        );
        assert_eq!(
            search_strip_root_prefix_offset(Some(b"/repo/src/a.c"), Some(b"/repo"), 5),
            6
        );
        assert_eq!(
            search_strip_root_prefix_offset(Some(b"/repo"), Some(b"/repo"), 5),
            5
        );
        assert_eq!(
            search_strip_root_prefix_offset(Some(b"/repo/submarine.c"), Some(b"/repo/sub"), 9),
            9
        );
        assert_eq!(
            search_strip_root_prefix_offset(Some(b"/repo/src/a.c"), Some(b"/repo/"), 6),
            6
        );
        assert_eq!(
            search_strip_root_prefix_offset(Some(b"/other/a.c"), Some(b"/repo"), 5),
            0
        );
        assert_eq!(
            search_strip_root_prefix_offset(Some(b"/repo"), Some(b"/repo-long"), 10),
            0
        );
        assert_eq!(
            search_strip_root_prefix_offset(Some(b"/repo/a.c"), Some(b"/repo"), 0),
            1
        );
    }

    #[test]
    fn detect_changes_wants_symbols_matches_c_contract() {
        assert!(detect_changes_wants_symbols(None));
        assert!(detect_changes_wants_symbols(Some(b"symbols")));
        assert!(detect_changes_wants_symbols(Some(b"impact")));
        assert!(!detect_changes_wants_symbols(Some(b"")));
        assert!(!detect_changes_wants_symbols(Some(b"files")));
        assert!(!detect_changes_wants_symbols(Some(b"Symbols")));
        assert!(!detect_changes_wants_symbols(Some(b"symbols ")));
        assert!(!detect_changes_wants_symbols(Some(b"unknown")));
    }

    #[test]
    fn detect_changes_impacted_label_matches_c_contract() {
        assert!(!detect_changes_impacted_label(None));
        assert!(!detect_changes_impacted_label(Some(b"File")));
        assert!(!detect_changes_impacted_label(Some(b"Folder")));
        assert!(!detect_changes_impacted_label(Some(b"Project")));
        assert!(detect_changes_impacted_label(Some(b"Function")));
        assert!(detect_changes_impacted_label(Some(b"Method")));
        assert!(detect_changes_impacted_label(Some(b"Route")));
        assert!(detect_changes_impacted_label(Some(b"")));
        assert!(detect_changes_impacted_label(Some(b"file")));
        assert!(detect_changes_impacted_label(Some(b"Project ")));
    }

    #[test]
    fn detect_changes_status_path_offset_matches_c_contract() {
        let f = |s: Option<&str>| detect_changes_status_path_offset(s.map(str::as_bytes));

        assert_eq!(f(None), usize::MAX);
        assert_eq!(f(Some("")), usize::MAX);
        assert_eq!(f(Some("src/mcp/mcp.c")), 0);
        assert_eq!(f(Some("?? src/new.c")), 3);
        assert_eq!(f(Some("A  src/a.c")), 3);
        assert_eq!(f(Some(" M src/m.c")), 3);
        assert_eq!(f(Some("R  old/name.c -> new/name.c")), 17);
        assert_eq!(f(Some("R  old -> ")), usize::MAX);
        assert_eq!(f(Some("?? ")), usize::MAX);
        assert_eq!(f(Some("ZZ weird")), 0);
        assert_eq!(f(Some("R  no-arrow")), 3);
    }

    #[test]
    fn search_line_match_span_matches_c_contract() {
        assert_eq!(search_line_match_span(10, 20, 15), 10);
        assert_eq!(search_line_match_span(10, 20, 10), 10);
        assert_eq!(search_line_match_span(10, 20, 20), 10);
        assert_eq!(search_line_match_span(10, 20, 9), -1);
        assert_eq!(search_line_match_span(10, 20, 21), -1);
        assert_eq!(search_line_match_span(20, 10, 15), -1);
    }

    #[test]
    fn search_pick_resolved_index_matches_c_contract() {
        assert_eq!(search_pick_resolved_index(&[] as &[i64]), (-1, false));
        assert_eq!(search_pick_resolved_index(&[42]), (0, false));
        assert_eq!(search_pick_resolved_index(&[10, 20, 15]), (1, false));
        assert_eq!(search_pick_resolved_index(&[20, 10, 20]), (0, true));
        assert_eq!(search_pick_resolved_index(&[-5, -1, -1]), (1, true));
    }

    #[test]
    fn search_pick_tightest_index_matches_c_contract() {
        assert_eq!(search_pick_tightest_index(&[]), -1);
        assert_eq!(search_pick_tightest_index(&[-1, -1]), -1);
        assert_eq!(search_pick_tightest_index(&[10, 5, 20]), 1);
        assert_eq!(search_pick_tightest_index(&[3, 3, 5]), 0);
        assert_eq!(search_pick_tightest_index(&[-1, 7, -1]), 1);
    }

    #[test]
    fn utf8_is_cont_byte_matches_c_contract() {
        assert!(utf8_is_cont_byte(0x80));
        assert!(utf8_is_cont_byte(0xBF));
        assert!(!utf8_is_cont_byte(0x7F));
        assert!(!utf8_is_cont_byte(0xC0));
        assert!(!utf8_is_cont_byte(0x00));
    }

    #[test]
    fn node_resolution_score_matches_c_contract() {
        assert_eq!(node_resolution_score(None, 10, 20), 10);
        assert_eq!(node_resolution_score(Some(b""), 10, 20), 1_000_010);
        assert_eq!(node_resolution_score(Some(b"Function"), 10, 20), 2_000_010);
        assert_eq!(node_resolution_score(Some(b"Method"), 10, 20), 2_000_010);
        assert_eq!(node_resolution_score(Some(b"Class"), 10, 20), 1_000_010);
        assert_eq!(node_resolution_score(Some(b"Struct"), 10, 20), 1_000_010);
        assert_eq!(node_resolution_score(Some(b"Module"), 10, 20), 10);
        assert_eq!(node_resolution_score(Some(b"File"), 10, 20), 10);
        assert_eq!(node_resolution_score(Some(b"Function"), 20, 10), 2_000_000);
        assert_eq!(node_resolution_score(Some(b"function"), 10, 20), 1_000_010);
    }

    #[test]
    fn adr_mode_matches_c_contract() {
        assert_eq!(adr_mode(None), 0);
        assert_eq!(adr_mode(Some(b"")), 0);
        assert_eq!(adr_mode(Some(b"get")), 0);
        assert_eq!(adr_mode(Some(b"update")), 1);
        assert_eq!(adr_mode(Some(b"store")), 1);
        assert_eq!(adr_mode(Some(b"sections")), 2);
        assert_eq!(adr_mode(Some(b"Update")), 0);
        assert_eq!(adr_mode(Some(b"update ")), 0);
        assert_eq!(adr_mode(Some(b"unknown")), 0);
    }

    #[test]
    fn adr_sections_json_matches_c_contract() {
        assert_eq!(adr_sections_json(None), b"[]".to_vec());
        assert_eq!(adr_sections_json(Some(b"")), b"[]".to_vec());
        assert_eq!(
            adr_sections_json(Some(b"# PURPOSE\r\nbody\n## STACK\nnot header")),
            br####"["# PURPOSE","## STACK"]"####.to_vec()
        );
        assert_eq!(
            adr_sections_json(Some(b" # indented\n### Deep\n# quote \"slash\\\"\n")),
            br####"["### Deep","# quote \"slash\\\""]"####.to_vec()
        );
        let mut invalid_expected = b"[\"# invalid ".to_vec();
        invalid_expected.extend_from_slice(&[0xef, 0xbf, 0xbd]);
        invalid_expected.extend_from_slice(b"\"]");
        assert_eq!(adr_sections_json(Some(b"# invalid \xff")), invalid_expected);

        let long = [b"#".as_slice(), &[b'a'; 1100]].concat();
        let json = adr_sections_json(Some(&long));
        assert_eq!(json.len(), 1027);
        assert!(json.starts_with(b"[\"#"));
        assert!(json.ends_with(b"\"]"));
    }

    #[test]
    fn bm25_match_query_matches_c_contract() {
        assert_eq!(bm25_match_query(None, 1024), (0, Vec::new()));
        assert_eq!(bm25_match_query(Some(b"abc"), 0), (0, Vec::new()));
        assert_eq!(bm25_match_query(Some(b"abc"), 1), (0, Vec::new()));
        assert_eq!(bm25_match_query(Some(b""), 1024), (0, Vec::new()));
        assert_eq!(bm25_match_query(Some(b"!!!"), 1024), (0, Vec::new()));
        assert_eq!(
            bm25_match_query(Some(b"update settings"), 1024),
            (2, b"update OR settings".to_vec())
        );
        assert_eq!(
            bm25_match_query(Some(b"foo-bar baz.qux"), 1024),
            (4, b"foo OR bar OR baz OR qux".to_vec())
        );
        assert_eq!(
            bm25_match_query(Some(b"camelCase snake_case 123"), 1024),
            (3, b"camelCase OR snake_case OR 123".to_vec())
        );
        assert_eq!(
            bm25_match_query(Some("呼叫 alpha β gamma".as_bytes()), 1024),
            (2, b"alpha OR gamma".to_vec())
        );
        assert_eq!(
            bm25_match_query(Some(b"alpha beta"), 10),
            (1, b"alpha".to_vec())
        );
        assert_eq!(
            bm25_match_query(Some(b"alpha beta"), 7),
            (1, b"alpha".to_vec())
        );
        assert_eq!(bm25_match_query(Some(b"abcdef"), 7), (0, Vec::new()));
    }

    #[test]
    fn bm25_file_pattern_like_matches_c_contract() {
        assert_eq!(bm25_file_pattern_like(None), None);
        assert_eq!(bm25_file_pattern_like(Some(b"")), Some(b"%%".to_vec()));
        assert_eq!(
            bm25_file_pattern_like(Some(b"src/lib")),
            Some(b"%src/lib%".to_vec())
        );
        assert_eq!(
            bm25_file_pattern_like(Some(b"*.go")),
            Some(b"%.go".to_vec())
        );
        assert_eq!(
            bm25_file_pattern_like(Some(b"src/api/*")),
            Some(b"src/api/%".to_vec())
        );
        assert_eq!(
            bm25_file_pattern_like(Some(b"f???.txt")),
            Some(b"f___.txt".to_vec())
        );
        assert_eq!(
            bm25_file_pattern_like(Some(b"src/[abc]/*.ts")),
            Some(b"src/[abc]/%.ts".to_vec())
        );
    }

    #[test]
    fn sanitize_utf8_lossy_matches_c_contract() {
        assert_eq!(sanitize_utf8_lossy(None), None);
        assert_eq!(sanitize_utf8_lossy(Some(b"")), Some(Vec::new()));
        assert_eq!(sanitize_utf8_lossy(Some(b"hello")), Some(b"hello".to_vec()));
        assert_eq!(
            sanitize_utf8_lossy(Some(b"hello \xff world")),
            Some(b"hello \xef\xbf\xbd world".to_vec())
        );
        assert_eq!(
            sanitize_utf8_lossy(Some(b"\xc3\x28")),
            Some(b"\xef\xbf\xbd\x28".to_vec())
        );
    }

    #[test]
    fn architecture_aspect_wanted_matches_c_contract() {
        assert!(architecture_aspect_wanted(None, b"structure"));
        assert!(architecture_aspect_wanted(Some(b""), b"structure"));
        assert!(architecture_aspect_wanted(Some(b"{}"), b"structure"));
        assert!(architecture_aspect_wanted(
            Some(b"{\"path\":\"/foo\"}"),
            b"structure"
        ));
        assert!(architecture_aspect_wanted(
            Some(b"{\"path\":\"/foo\",\"aspects\":null}"),
            b"structure"
        ));
        assert!(architecture_aspect_wanted(
            Some(b"{\"path\":\"/foo\",\"aspects\":\"structure\"}"),
            b"structure"
        ));
        assert!(!architecture_aspect_wanted(
            Some(b"{\"path\":\"/foo\",\"aspects\":[]}"),
            b"structure"
        ));
        assert!(architecture_aspect_wanted(
            Some(b"{\"path\":\"/foo\",\"aspects\":[\"all\"]}"),
            b"structure"
        ));
        assert!(architecture_aspect_wanted(
            Some(b"{\"path\":\"/foo\",\"aspects\":[\"structure\"]}"),
            b"structure"
        ));
        assert!(!architecture_aspect_wanted(
            Some(b"{\"path\":\"/foo\",\"aspects\":[\"structure\"]}"),
            b"dependencies"
        ));
        assert!(architecture_aspect_wanted(
            Some(b"{\"path\":\"/foo\",\"aspects\":[\"dependencies\",\"routes\"]}"),
            b"routes"
        ));
        assert!(architecture_aspect_wanted(
            Some(b"{\"path\":\"/foo\",\"aspects\":[123,\"structure\"]}"),
            b"structure"
        ));
        assert!(architecture_aspect_wanted(
            Some(b"{\"path\":"),
            b"structure"
        )); // invalid json -> true
        assert!(architecture_aspect_wanted(
            Some(b"{\"aspects\":[]} trailing"),
            b"structure"
        ));
    }

    #[test]
    fn trace_is_test_file_matches_c_contract() {
        assert!(!trace_is_test_file(None));
        assert!(!trace_is_test_file(Some(b"")));
        assert!(trace_is_test_file(Some(b"src/testdata/helper.go")));
        assert!(trace_is_test_file(Some(b"test_handler.py")));
        assert!(trace_is_test_file(Some(b"src/foo_test.go")));
        assert!(trace_is_test_file(Some(b"src/tests/helper.c")));
        assert!(trace_is_test_file(Some(b"src/spec/helper.rb")));
        assert!(trace_is_test_file(Some(b"handler.test.ts")));
        assert!(!trace_is_test_file(Some(b"src/contest/helper.c")));
        assert!(!trace_is_test_file(Some(b"src/mytests/helper.c")));
        assert!(!trace_is_test_file(Some(b"handler.spec.ts")));
        assert!(!trace_is_test_file(Some(br"C:\repo\tests\helper.py")));
    }

    #[test]
    fn project_db_file_name_matches_c_contract() {
        assert!(!project_db_file_name(None));
        assert!(!project_db_file_name(Some(b"")));
        assert!(!project_db_file_name(Some(b".db")));
        assert!(project_db_file_name(Some(b"x.db")));
        assert!(project_db_file_name(Some(b"alpha704.db")));
        assert!(project_db_file_name(Some(b"tmp-bench-project.db")));
        assert!(!project_db_file_name(Some(b"alpha704.db-wal")));
        assert!(!project_db_file_name(Some(b"alpha704.sqlite")));
        assert!(!project_db_file_name(Some(b"_internal.db")));
        assert!(!project_db_file_name(Some(b":memory:")));
        assert!(!project_db_file_name(Some(b":memory:.db")));
        assert!(project_db_file_name(Some(b"tmp-.db")));
    }

    #[test]
    fn cancel_request_matches_contract() {
        let numeric = |s: &str, active| cancel_request_matches(Some(s.as_bytes()), active, None);
        let string = |s: &str, active: &str| {
            cancel_request_matches(Some(s.as_bytes()), -1, Some(active.as_bytes()))
        };

        assert!(!cancel_request_matches(None, 7, None));
        assert!(!numeric("", 7));
        assert!(!numeric("not json", 7));
        assert!(!numeric("[]", 7));
        assert!(!numeric("{}", 7));
        assert!(numeric(r#"{"requestId":7}"#, 7));
        assert!(!numeric(r#"{"requestId":8}"#, 7));
        assert!(!numeric(r#"{"requestId":"7"}"#, 7));
        assert!(!numeric(r#"{"requestId":7.0}"#, 7));
        assert!(!numeric(r#"{"requestId":true}"#, 7));
        assert!(numeric(r#"{"requestId":7,"requestId":8}"#, 7));
        assert!(!numeric(r#"{"requestId":7,"requestId":8}"#, 8));

        assert!(string(r#"{"requestId":"call-1"}"#, "call-1"));
        assert!(!string(r#"{"requestId":"call-2"}"#, "call-1"));
        assert!(!string(r#"{"requestId":7}"#, "7"));
        assert!(string(r#"{"requestId":"call-\u4e2d"}"#, "call-中"));
        assert!(string(r#"{"requestId":"","requestId":"late"}"#, ""));
        assert!(!cancel_request_matches(
            Some(b"{\"requestId\":\"bad\xff\"}"),
            -1,
            Some(b"bad")
        ));
    }

    #[test]
    fn jsonrpc_format_error_matches_c_contract() {
        let f = |id, code, msg: Option<&[u8]>| {
            String::from_utf8(jsonrpc_format_error(id, code, msg)).unwrap()
        };
        assert_eq!(
            f(5, -32600, Some(b"Invalid Request")),
            r#"{"jsonrpc":"2.0","id":5,"error":{"code":-32600,"message":"Invalid Request"}}"#
        );
        assert_eq!(
            f(-1, -32700, Some(b"bad \"json\" \\ input\n")),
            "{\"jsonrpc\":\"2.0\",\"id\":-1,\"error\":{\"code\":-32700,\"message\":\"bad \\\"json\\\" \\\\ input\\n\"}}"
        );
        assert_eq!(
            f(7, -32000, Some("錯誤".as_bytes())),
            r#"{"jsonrpc":"2.0","id":7,"error":{"code":-32000,"message":"錯誤"}}"#
        );
        assert_eq!(
            f(0, -1, None),
            r#"{"jsonrpc":"2.0","id":0,"error":{"code":-1,"message":""}}"#
        );
    }

    #[test]
    fn jsonrpc_format_response_matches_c_contract() {
        let f = |id, id_str: Option<&[u8]>, result: Option<&[u8]>, error: Option<&[u8]>| {
            String::from_utf8(jsonrpc_format_response(id, id_str, result, error)).unwrap()
        };
        assert_eq!(
            f(1, None, Some(br#"{"name":"codebase-memory-mcp"}"#), None),
            r#"{"jsonrpc":"2.0","id":1,"result":{"name":"codebase-memory-mcp"}}"#
        );
        assert_eq!(
            f(0, Some(b"init-abc"), Some(br#"{ "ok" : true }"#), None),
            r#"{"jsonrpc":"2.0","id":"init-abc","result":{"ok":true}}"#
        );
        assert_eq!(
            f(5, None, None, Some(br#"{"code":-32600,"message":"bad"}"#)),
            r#"{"jsonrpc":"2.0","id":5,"error":{"code":-32600,"message":"bad"}}"#
        );
        assert_eq!(
            f(7, Some(b"a\"b\\c"), None, None),
            r#"{"jsonrpc":"2.0","id":"a\"b\\c","result":null}"#
        );
        assert_eq!(
            f(9, None, Some(b"not json"), None),
            r#"{"jsonrpc":"2.0","id":9}"#
        );
        assert_eq!(
            f(9, None, Some(br#"{"ok":true}"#), Some(b"not json")),
            r#"{"jsonrpc":"2.0","id":9}"#
        );
    }

    #[test]
    fn mcp_text_result_matches_c_contract() {
        let f = |text: Option<&[u8]>, is_error| {
            String::from_utf8(mcp_text_result(text, is_error)).unwrap()
        };
        assert_eq!(
            f(Some(br#"{"total":5}"#), false),
            r#"{"content":[{"type":"text","text":"{\"total\":5}"}],"structuredContent":{"total":5},"isError":false}"#
        );
        assert_eq!(
            f(Some(b"plain text"), false),
            r#"{"content":[{"type":"text","text":"plain text"}],"isError":false}"#
        );
        assert_eq!(
            f(Some(br#"[1,2]"#), false),
            r#"{"content":[{"type":"text","text":"[1,2]"}],"isError":false}"#
        );
        assert_eq!(
            f(Some(b"bad \"json\" \\ input\n"), true),
            "{\"content\":[{\"type\":\"text\",\"text\":\"bad \\\"json\\\" \\\\ input\\n\"}],\"isError\":true}"
        );
        assert_eq!(
            f(None, false),
            r#"{"content":[{"type":"text","text":""}],"isError":false}"#
        );
    }

    #[test]
    fn summarizes_request_notification_and_params() {
        assert_eq!(
            summary(
                br#"{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"capabilities":{}}}"#
            ),
            "jsonrpc=2.0;method=initialize;id=int:1;params=object"
        );
        assert_eq!(
            summary(br#"{"jsonrpc":"2.0","method":"notifications/initialized"}"#),
            "jsonrpc=2.0;method=notifications/initialized;id=none;params=none"
        );
        assert_eq!(
            summary(br#"{"jsonrpc":"2.0","id":42,"method":"tools/call","params":{"name":"search_graph","arguments":{"limit":5}}}"#),
            "jsonrpc=2.0;method=tools/call;id=int:42;params=object"
        );
    }

    #[test]
    fn mirrors_c_parser_defaults_and_id_kinds() {
        assert_eq!(
            summary(br#"{"id":1,"method":"initialize","params":{}}"#),
            "jsonrpc=2.0;method=initialize;id=int:1;params=object"
        );
        assert_eq!(
            summary(br#"  { "jsonrpc" : "2.0" , "id" : "99" , "method" : "tools/list" }  "#),
            "jsonrpc=2.0;method=tools/list;id=string:99;params=none"
        );
        assert_eq!(
            summary(br#"{"jsonrpc":2,"id":true,"method":"ping","params":null}"#),
            "jsonrpc=2.0;method=ping;id=other;params=null"
        );
    }

    #[test]
    fn rejects_invalid_or_missing_method() {
        assert!(jsonrpc_request_summary(b"not json").is_none());
        assert!(jsonrpc_request_summary(b"").is_none());
        assert!(jsonrpc_request_summary(b"[1,2,3]").is_none());
        assert!(jsonrpc_request_summary(br#"{"jsonrpc":"2.0","id":1}"#).is_none());
        assert!(jsonrpc_request_summary(br#"{"jsonrpc":"2.0","method":7}"#).is_none());
    }

    #[test]
    fn escapes_summary_delimiters() {
        assert_eq!(
            summary(br#"{"method":"weird\nmethod","id":"a;b=c\\d","params":[]}"#),
            "jsonrpc=2.0;method=weird\\nmethod;id=string:a\\;b\\=c\\\\d;params=array"
        );
    }

    #[test]
    fn parses_request_fields_for_c_wrapper() {
        let req = super::jsonrpc_parse(
            br#" { "id" : "abc" , "method" : "tools/call" , "params" : { "name" : "search_graph" } } "#,
        )
        .unwrap();
        assert_eq!(req.jsonrpc, b"2.0");
        assert_eq!(req.method, b"tools/call");
        assert_eq!(req.id, super::ParsedId::String(b"abc".to_vec()));
        assert_eq!(
            req.params_raw,
            Some(br#"{ "name" : "search_graph" }"#.to_vec())
        );

        let unicode = super::jsonrpc_parse(br#"{"id":"\u4e2d","method":"ping"}"#).unwrap();
        assert_eq!(
            unicode.id,
            super::ParsedId::String("中".as_bytes().to_vec())
        );
    }

    #[test]
    fn mirrors_first_key_wins_and_utf8_validation() {
        let duplicate = super::jsonrpc_parse(
            br#"{"method":"ping","method":"tools/list","id":1,"id":"late","params":{"a":1},"params":{"b":2},"jsonrpc":3,"jsonrpc":"late"}"#,
        )
        .unwrap();
        assert_eq!(duplicate.jsonrpc, b"2.0");
        assert_eq!(duplicate.method, b"ping");
        assert_eq!(duplicate.id, super::ParsedId::Int(1));
        assert_eq!(duplicate.params_raw, Some(br#"{"a":1}"#.to_vec()));

        assert!(super::jsonrpc_parse(b"{\"method\":\"bad\xff\"}").is_none());
    }
}
