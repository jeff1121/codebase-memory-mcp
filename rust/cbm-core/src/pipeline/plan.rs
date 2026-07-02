//! C pipeline orchestration 的 pass plan parity fixture。
//!
//! 此模組只建模 plan 資料，不呼叫 extraction、tree-sitter、LSP、graph-buffer
//! 或 store code。

const MODE_MODERATE: i32 = 1;
const MODE_FAST: i32 = 2;
const MIN_FILES_FOR_PARALLEL: i32 = 50;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum PlanKind {
    Sequential,
    Predump,
    ExtractionChoice,
    IncrementalExtractResolve,
    IncrementalPost,
    ParallelExtraction,
    FullPipeline,
}

impl PlanKind {
    pub fn from_i32(value: i32) -> Option<Self> {
        match value {
            0 => Some(Self::Sequential),
            1 => Some(Self::Predump),
            2 => Some(Self::ExtractionChoice),
            3 => Some(Self::IncrementalExtractResolve),
            4 => Some(Self::IncrementalPost),
            5 => Some(Self::ParallelExtraction),
            6 => Some(Self::FullPipeline),
            _ => None,
        }
    }
}

pub fn describe(kind: PlanKind, mode: i32, worker_count: i32, file_count: i32) -> String {
    match kind {
        PlanKind::Sequential => sequential_plan(),
        PlanKind::Predump => predump_plan(mode),
        PlanKind::ExtractionChoice => extraction_choice(worker_count, file_count).to_owned(),
        PlanKind::IncrementalExtractResolve => {
            incremental_extract_resolve_plan(worker_count, file_count)
        }
        PlanKind::IncrementalPost => incremental_post_plan(mode),
        PlanKind::ParallelExtraction => parallel_extraction_plan(),
        PlanKind::FullPipeline => full_pipeline_plan(mode, worker_count, file_count),
    }
}

fn sequential_plan() -> String {
    [
        "definitions:required",
        "k8s:ignore_err",
        "lsp_cross:ignore_err:requires_result_cache",
        "calls:required",
        "usages:required",
        "semantic:required",
        "infra_routes:after_success",
        "infra_bindings:after_success",
    ]
    .join(",")
}

fn predump_plan(mode: i32) -> String {
    let mut passes = vec!["decorator_tags", "configlink", "route_match"];
    if mode != MODE_FAST {
        passes.push("similarity");
        passes.push("semantic_edges");
    }
    passes.push("complexity");
    passes.join(",")
}

fn extraction_choice(worker_count: i32, file_count: i32) -> &'static str {
    if worker_count > 1 && file_count > MIN_FILES_FOR_PARALLEL {
        "parallel"
    } else {
        "sequential"
    }
}

fn parallel_extraction_plan() -> String {
    [
        "parallel_extract:required",
        "registry_build:required",
        "lsp_cross_prepare:env_optional",
        "parallel_resolve:required",
        "infra_routes:required",
        "infra_bindings:required",
        "k8s:ignore_err",
    ]
    .join(",")
}

fn incremental_extract_resolve_plan(worker_count: i32, file_count: i32) -> String {
    if extraction_choice(worker_count, file_count) == "parallel" {
        [
            "incr_extract:ignore_err",
            "incr_registry:ignore_err",
            "incr_resolve:ignore_err:no_cross_lsp_prebuild",
        ]
        .join(",")
    } else {
        [
            "definitions:ignore_err",
            "calls:ignore_err",
            "usages:ignore_err",
            "semantic:ignore_err",
        ]
        .join(",")
    }
}

fn incremental_post_plan(mode: i32) -> String {
    let mut passes = vec![
        "k8s:ignore_err",
        "incr_tests:ignore_err",
        "incr_decorator_tags:ignore_err",
        "incr_configlink:ignore_err",
    ];
    if mode <= MODE_MODERATE {
        passes.push("incr_similarity:ignore_err");
        passes.push("incr_semantic_edges:ignore_err");
    }
    passes.push("edge_relink:best_effort");
    passes.push("incremental_dump:best_effort");
    passes.push("persist_hashes:best_effort");
    passes.push("artifact_export:optional_existing_artifact");
    passes.join(",")
}

fn full_pipeline_plan(mode: i32, worker_count: i32, file_count: i32) -> String {
    let mut passes = vec![
        "macro_extraction:full_mode_only".to_owned(),
        "userconfig_load:fail_open".to_owned(),
        "discover:required".to_owned(),
        "try_incremental_or_delete_db:existing_db_only".to_owned(),
        "structure:required".to_owned(),
    ];

    if extraction_choice(worker_count, file_count) == "parallel" {
        passes.push(parallel_extraction_plan());
    } else {
        passes.push(sequential_plan());
    }

    passes.push("tests:required".to_owned());
    if mode != MODE_FAST {
        passes.push("githistory:required".to_owned());
    }
    passes.push(predump_plan(mode));
    passes.push("dump:required".to_owned());
    passes.push("persist_hashes:best_effort".to_owned());
    passes.push("artifact_export:optional_persistence".to_owned());
    passes.join(",")
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn sequential_plan_matches_c_dispatch_table() {
        assert_eq!(
            describe(PlanKind::Sequential, 0, 0, 0),
            "definitions:required,k8s:ignore_err,lsp_cross:ignore_err:requires_result_cache,\
             calls:required,usages:required,semantic:required,infra_routes:after_success,\
             infra_bindings:after_success"
        );
    }

    #[test]
    fn predump_skips_moderate_only_passes_only_in_fast_mode() {
        assert_eq!(
            describe(PlanKind::Predump, 0, 0, 0),
            "decorator_tags,configlink,route_match,similarity,semantic_edges,complexity"
        );
        assert_eq!(
            describe(PlanKind::Predump, MODE_FAST, 0, 0),
            "decorator_tags,configlink,route_match,complexity"
        );
        assert_eq!(
            describe(PlanKind::Predump, 3, 0, 0),
            "decorator_tags,configlink,route_match,similarity,semantic_edges,complexity"
        );
    }

    #[test]
    fn parallel_threshold_matches_c_pipeline() {
        assert_eq!(
            describe(PlanKind::ExtractionChoice, 2, 2, MIN_FILES_FOR_PARALLEL),
            "sequential"
        );
        assert_eq!(
            describe(PlanKind::ExtractionChoice, 2, 2, MIN_FILES_FOR_PARALLEL + 1),
            "parallel"
        );
        assert_eq!(
            describe(PlanKind::ExtractionChoice, 2, 1, MIN_FILES_FOR_PARALLEL + 1),
            "sequential"
        );
        assert_eq!(
            describe(PlanKind::ExtractionChoice, 2, 0, MIN_FILES_FOR_PARALLEL + 1),
            "sequential"
        );
        assert_eq!(
            describe(
                PlanKind::ExtractionChoice,
                2,
                -1,
                MIN_FILES_FOR_PARALLEL + 1
            ),
            "sequential"
        );
        assert_eq!(describe(PlanKind::ExtractionChoice, 2, 2, -1), "sequential");
    }

    #[test]
    fn incremental_plans_match_c_gating() {
        assert_eq!(
            describe(PlanKind::IncrementalExtractResolve, 0, 2, 51),
            "incr_extract:ignore_err,incr_registry:ignore_err,\
             incr_resolve:ignore_err:no_cross_lsp_prebuild"
        );
        assert_eq!(
            describe(PlanKind::IncrementalExtractResolve, 0, 2, 50),
            "definitions:ignore_err,calls:ignore_err,usages:ignore_err,semantic:ignore_err"
        );
        assert_eq!(
            describe(PlanKind::IncrementalPost, 0, 0, 0),
            "k8s:ignore_err,incr_tests:ignore_err,incr_decorator_tags:ignore_err,\
             incr_configlink:ignore_err,incr_similarity:ignore_err,\
             incr_semantic_edges:ignore_err,edge_relink:best_effort,\
             incremental_dump:best_effort,persist_hashes:best_effort,\
             artifact_export:optional_existing_artifact"
        );
        assert_eq!(
            describe(PlanKind::IncrementalPost, MODE_FAST, 0, 0),
            "k8s:ignore_err,incr_tests:ignore_err,incr_decorator_tags:ignore_err,\
             incr_configlink:ignore_err,edge_relink:best_effort,\
             incremental_dump:best_effort,persist_hashes:best_effort,\
             artifact_export:optional_existing_artifact"
        );
        assert_eq!(
            describe(PlanKind::IncrementalPost, 3, 0, 0),
            "k8s:ignore_err,incr_tests:ignore_err,incr_decorator_tags:ignore_err,\
             incr_configlink:ignore_err,edge_relink:best_effort,\
             incremental_dump:best_effort,persist_hashes:best_effort,\
             artifact_export:optional_existing_artifact"
        );
    }

    #[test]
    fn parallel_and_full_plans_capture_outer_orchestration() {
        assert_eq!(
            describe(PlanKind::ParallelExtraction, 0, 0, 0),
            "parallel_extract:required,registry_build:required,lsp_cross_prepare:env_optional,\
             parallel_resolve:required,infra_routes:required,infra_bindings:required,\
             k8s:ignore_err"
        );
        assert_eq!(
            describe(PlanKind::FullPipeline, MODE_FAST, 2, 51),
            "macro_extraction:full_mode_only,userconfig_load:fail_open,discover:required,\
             try_incremental_or_delete_db:existing_db_only,structure:required,\
             parallel_extract:required,registry_build:required,lsp_cross_prepare:env_optional,\
             parallel_resolve:required,infra_routes:required,infra_bindings:required,\
             k8s:ignore_err,tests:required,decorator_tags,configlink,route_match,\
             complexity,dump:required,persist_hashes:best_effort,\
             artifact_export:optional_persistence"
        );
    }
}
