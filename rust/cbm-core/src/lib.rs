//! codebase-memory-mcp 階段性 Rust 重構的 foundation code。
//!
//! 這個 crate 先作為低風險 C foundation helpers 的 parity layer；預設不接入
//! production C binary。

pub mod cli;
pub mod cypher;
pub mod ffi;
#[cfg(feature = "graph-buffer-mutation-only")]
pub mod ffi_graph_buffer_mutation_only;
#[cfg(feature = "system-info-only")]
pub mod ffi_system_info_only;

#[cfg(feature = "pipeline-worker-pool-only")]
pub mod ffi_pipeline_worker_pool_only;

#[cfg(feature = "slab-alloc-only")]
pub mod ffi_slab_alloc_only;

pub mod cli_archive;
pub mod discover;
#[cfg(feature = "ast-profile-codec-only")]
pub mod ffi_ast_profile_codec_only;
#[cfg(feature = "discover-gitignore-only")]
pub mod ffi_gitignore_only;
#[cfg(feature = "discover-language-only")]
pub mod ffi_language_only;
#[cfg(feature = "mem-only")]
pub mod ffi_mem_only;
#[cfg(feature = "pipeline-fqn-only")]
pub mod ffi_pipeline_fqn_only;
#[cfg(feature = "profile-only")]
pub mod ffi_profile_only;
#[cfg(feature = "semantic-config-only")]
pub mod ffi_semantic_config_only;
#[cfg(feature = "semantic-proximity-only")]
pub mod ffi_semantic_proximity_only;
#[cfg(feature = "semantic-tokenize-only")]
pub mod ffi_semantic_tokenize_only;
#[cfg(feature = "semantic-vector-only")]
pub mod ffi_semantic_vector_only;
#[cfg(feature = "discover-userconfig-only")]
pub mod ffi_userconfig_only;
pub mod foundation;
pub mod git_context;
pub mod language_name;
pub mod log_path;
pub mod mcp;
pub mod pipeline;
pub mod pipeline_calls;
pub mod pipeline_compile_commands;
pub mod pipeline_complexity;
pub mod pipeline_configlink;
pub mod pipeline_cross_repo;
pub mod pipeline_definitions;
pub mod pipeline_enrichment;
pub mod pipeline_envscan;
pub mod pipeline_incremental;
pub mod pipeline_json;
pub mod pipeline_k8s;
pub mod pipeline_lsp_cross;
pub mod pipeline_pkgmap;
pub mod pipeline_semantic_edges;
pub mod simhash;
pub mod vmem;

#[cfg(feature = "pipeline-checked-exception-only")]
mod ffi_checked_exception;
#[cfg(feature = "pipeline-k8s-file-classifiers-only")]
mod ffi_k8s_file_classifiers;
#[cfg(feature = "pipeline-registry-only")]
pub mod ffi_pipeline_registry_only;
#[cfg(feature = "pipeline-route-args-only")]
mod ffi_route_args;
#[cfg(feature = "pipeline-route-node-classifiers-only")]
mod ffi_route_node_classifiers;
pub mod store;
pub mod traces;
pub mod watcher;

#[cfg(feature = "str-util-only")]
mod ffi_str_util_only;

#[cfg(feature = "diagnostics-format-only")]
mod ffi_diagnostics_format_only;
#[cfg(all(feature = "diagnostics-only", not(feature = "diagnostics-format-only")))]
mod ffi_diagnostics_only;
#[cfg(feature = "hash-table-only")]
mod ffi_hash_table_only;
#[cfg(feature = "pipeline-path-alias-only")]
mod ffi_pipeline_path_alias_only;

pub mod ast_profile_classifiers;
#[cfg(feature = "store-fts-tokenizer-only")]
mod fts_sqlite;
pub mod pipeline_usages;
pub mod semantic_token_classifiers;
pub mod ui_httpd;
pub mod ui_layout;
