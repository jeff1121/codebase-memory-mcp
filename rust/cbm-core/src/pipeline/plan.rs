//! C pipeline orchestration 的 pass plan parity fixture。
//!
//! 此模組只建模 plan 資料，不呼叫 extraction、tree-sitter、LSP、graph-buffer
//! 或 store code。

const MODE_MODERATE: i32 = 1;
const MODE_FAST: i32 = 2;
const MIN_FILES_FOR_PARALLEL: i32 = 50;
const MAX_PLAN_STEP_V2_KIND: u32 = 64;

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

#[repr(i32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum PlanStepKind {
    K8s = 1,
    IncrTests = 2,
    IncrDecoratorTags = 3,
    IncrConfiglink = 4,
    IncrSimilarity = 5,
    IncrSemanticEdges = 6,
    EdgeRelink = 7,
    IncrementalDump = 8,
    PersistHashes = 9,
    ArtifactExport = 10,
}

impl PlanStepKind {
    pub fn as_name(self) -> &'static str {
        match self {
            Self::K8s => "k8s",
            Self::IncrTests => "incr_tests",
            Self::IncrDecoratorTags => "incr_decorator_tags",
            Self::IncrConfiglink => "incr_configlink",
            Self::IncrSimilarity => "incr_similarity",
            Self::IncrSemanticEdges => "incr_semantic_edges",
            Self::EdgeRelink => "edge_relink",
            Self::IncrementalDump => "incremental_dump",
            Self::PersistHashes => "persist_hashes",
            Self::ArtifactExport => "artifact_export",
        }
    }
}

#[repr(i32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum PlanStepV2Kind {
    PredumpDecoratorTags = 1,
    PredumpConfiglink = 2,
    PredumpRouteMatch = 3,
    PredumpSimilarity = 4,
    PredumpSemanticEdges = 5,
    PredumpComplexity = 6,
    SequentialDefinitions = 16,
    SequentialK8s = 17,
    SequentialLspCross = 18,
    SequentialCalls = 19,
    SequentialUsages = 20,
    SequentialSemantic = 21,
    ParallelExtract = 32,
    ParallelRegistryBuild = 33,
    ParallelLspCrossPrepare = 34,
    ParallelResolve = 35,
    ParallelInfraRoutes = 36,
    ParallelInfraBindings = 37,
    ParallelK8s = 38,
    IncrementalDefinitions = 48,
    IncrementalCalls = 49,
    IncrementalUsages = 50,
    IncrementalSemantic = 51,
    IncrementalParallelExtract = 52,
    IncrementalRegistry = 53,
    IncrementalResolve = 54,
}

impl PlanStepV2Kind {
    pub fn as_name(self) -> &'static str {
        match self {
            Self::PredumpDecoratorTags => "decorator_tags",
            Self::PredumpConfiglink => "configlink",
            Self::PredumpRouteMatch => "route_match",
            Self::PredumpSimilarity => "similarity",
            Self::PredumpSemanticEdges => "semantic_edges",
            Self::PredumpComplexity => "complexity",
            Self::SequentialDefinitions => "definitions",
            Self::SequentialK8s => "k8s",
            Self::SequentialLspCross => "lsp_cross",
            Self::SequentialCalls => "calls",
            Self::SequentialUsages => "usages",
            Self::SequentialSemantic => "semantic",
            Self::ParallelExtract => "parallel_extract",
            Self::ParallelRegistryBuild => "registry_build",
            Self::ParallelLspCrossPrepare => "lsp_cross_prepare",
            Self::ParallelResolve => "parallel_resolve",
            Self::ParallelInfraRoutes => "infra_routes",
            Self::ParallelInfraBindings => "infra_bindings",
            Self::ParallelK8s => "k8s",
            Self::IncrementalDefinitions => "definitions",
            Self::IncrementalCalls => "calls",
            Self::IncrementalUsages => "usages",
            Self::IncrementalSemantic => "semantic",
            Self::IncrementalParallelExtract => "incr_extract",
            Self::IncrementalRegistry => "incr_registry",
            Self::IncrementalResolve => "incr_resolve",
        }
    }

    fn bit(self) -> u64 {
        let kind = self as u32;
        debug_assert!(kind > 0 && kind <= MAX_PLAN_STEP_V2_KIND);
        1u64 << (kind - 1)
    }
}

#[repr(i32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum PlanStepPhase {
    Predump = 1,
    SequentialExtract = 2,
    ParallelExtract = 3,
    IncrementalExtractResolve = 4,
}

#[repr(i32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum PlanStepPolicy {
    Required = 0,
    IgnoreErr = 1,
    BestEffort = 2,
    OptionalExistingArtifact = 3,
    EnvOptional = 4,
}

impl PlanStepPolicy {
    pub fn as_name(self) -> &'static str {
        match self {
            Self::Required => "required",
            Self::IgnoreErr => "ignore_err",
            Self::BestEffort => "best_effort",
            Self::OptionalExistingArtifact => "optional_existing_artifact",
            Self::EnvOptional => "env_optional",
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct PlanStep {
    pub kind: PlanStepKind,
    pub policy: PlanStepPolicy,
}

pub const PLAN_STEP_GATE_SKIP_FAST: u32 = 1 << 0;
pub const PLAN_STEP_GATE_REQUIRES_RESULT_CACHE: u32 = 1 << 1;
pub const PLAN_STEP_GATE_NO_CROSS_LSP_PREBUILD: u32 = 1 << 2;
pub const PLAN_STEP_EFFECT_MUTATES_GRAPH: u32 = 1 << 0;
pub const PLAN_TOP_GATE_SKIP_FAST: u32 = 1 << 0;
pub const PLAN_TOP_GATE_MAY_SHORT_CIRCUIT: u32 = 1 << 1;
pub const PLAN_TOP_EFFECT_MUTATES_GRAPH: u32 = 1 << 0;
pub const PLAN_TOP_EFFECT_WRITES_STORE_OR_ARTIFACT: u32 = 1 << 1;
pub const PLAN_TOP_NO_NESTED_PLAN: i32 = -1;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct PlanStepV2 {
    pub kind: PlanStepV2Kind,
    pub phase: PlanStepPhase,
    pub policy: PlanStepPolicy,
    pub gate_flags: u32,
    pub requires_mask: u64,
    pub effect_flags: u32,
}

#[repr(i32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum TopStepKind {
    MacroExtraction = 1,
    UserconfigLoad = 2,
    Discover = 3,
    TryIncrementalOrDeleteDb = 4,
    Structure = 5,
    ExtractionDispatch = 6,
    Tests = 7,
    Githistory = 8,
    Predump = 9,
    Dump = 10,
    PersistHashes = 11,
    ArtifactExport = 12,
}

impl TopStepKind {
    fn bit(self) -> u64 {
        1u64 << ((self as u32) - 1)
    }
}

#[repr(i32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum TopStepPhase {
    FullPipeline = 5,
}

#[repr(i32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum TopStepPolicy {
    Required = 0,
    BestEffort = 2,
    FullModeOnly = 5,
    FailOpen = 6,
    ExistingDbOnly = 7,
    OptionalPersistence = 8,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct TopLevelPlanStep {
    pub kind: TopStepKind,
    pub phase: TopStepPhase,
    pub policy: TopStepPolicy,
    pub gate_flags: u32,
    pub requires_mask: u64,
    pub effect_flags: u32,
    pub nested_plan_kind: i32,
}

impl PlanStep {
    fn render(self) -> String {
        format!("{}:{}", self.kind.as_name(), self.policy.as_name())
    }
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
    predump_steps_v2(mode)
        .into_iter()
        .map(|step| step.kind.as_name())
        .collect::<Vec<_>>()
        .join(",")
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
    incremental_post_steps(mode)
        .into_iter()
        .map(PlanStep::render)
        .collect::<Vec<_>>()
        .join(",")
}

pub fn incremental_post_steps(mode: i32) -> Vec<PlanStep> {
    use PlanStepKind::*;
    use PlanStepPolicy::*;

    let mut passes = vec![
        PlanStep {
            kind: K8s,
            policy: IgnoreErr,
        },
        PlanStep {
            kind: IncrTests,
            policy: IgnoreErr,
        },
        PlanStep {
            kind: IncrDecoratorTags,
            policy: IgnoreErr,
        },
        PlanStep {
            kind: IncrConfiglink,
            policy: IgnoreErr,
        },
    ];
    if mode <= MODE_MODERATE {
        passes.push(PlanStep {
            kind: IncrSimilarity,
            policy: IgnoreErr,
        });
        passes.push(PlanStep {
            kind: IncrSemanticEdges,
            policy: IgnoreErr,
        });
    }
    passes.push(PlanStep {
        kind: EdgeRelink,
        policy: BestEffort,
    });
    passes.push(PlanStep {
        kind: IncrementalDump,
        policy: BestEffort,
    });
    passes.push(PlanStep {
        kind: PersistHashes,
        policy: BestEffort,
    });
    passes.push(PlanStep {
        kind: ArtifactExport,
        policy: OptionalExistingArtifact,
    });
    passes
}

pub fn steps_v2(
    kind: PlanKind,
    mode: i32,
    worker_count: i32,
    file_count: i32,
) -> Option<Vec<PlanStepV2>> {
    match kind {
        PlanKind::Sequential => Some(sequential_steps_v2()),
        PlanKind::Predump => Some(predump_steps_v2(mode)),
        PlanKind::IncrementalExtractResolve => Some(incremental_extract_resolve_steps_v2(
            worker_count,
            file_count,
        )),
        PlanKind::ParallelExtraction => Some(parallel_extraction_steps_v2()),
        _ => None,
    }
}

fn incremental_extract_resolve_steps_v2(worker_count: i32, file_count: i32) -> Vec<PlanStepV2> {
    use PlanStepPolicy::IgnoreErr;
    use PlanStepV2Kind::*;

    let specs = if extraction_choice(worker_count, file_count) == "parallel" {
        vec![
            (IncrementalParallelExtract, 0),
            (IncrementalRegistry, 0),
            (IncrementalResolve, PLAN_STEP_GATE_NO_CROSS_LSP_PREBUILD),
        ]
    } else {
        vec![
            (IncrementalDefinitions, 0),
            (IncrementalCalls, 0),
            (IncrementalUsages, 0),
            (IncrementalSemantic, 0),
        ]
    };
    let mut seen = 0u64;
    let mut steps = Vec::with_capacity(specs.len());
    for (kind, gate_flags) in specs {
        steps.push(PlanStepV2 {
            kind,
            phase: PlanStepPhase::IncrementalExtractResolve,
            policy: IgnoreErr,
            gate_flags,
            requires_mask: seen,
            effect_flags: PLAN_STEP_EFFECT_MUTATES_GRAPH,
        });
        seen |= kind.bit();
    }
    steps
}

fn parallel_extraction_steps_v2() -> Vec<PlanStepV2> {
    use PlanStepPolicy::*;
    use PlanStepV2Kind::*;

    let specs = [
        (ParallelExtract, Required, PLAN_STEP_EFFECT_MUTATES_GRAPH),
        (
            ParallelRegistryBuild,
            Required,
            PLAN_STEP_EFFECT_MUTATES_GRAPH,
        ),
        (ParallelLspCrossPrepare, EnvOptional, 0),
        (ParallelResolve, Required, PLAN_STEP_EFFECT_MUTATES_GRAPH),
        (
            ParallelInfraRoutes,
            Required,
            PLAN_STEP_EFFECT_MUTATES_GRAPH,
        ),
        (
            ParallelInfraBindings,
            Required,
            PLAN_STEP_EFFECT_MUTATES_GRAPH,
        ),
        (ParallelK8s, IgnoreErr, PLAN_STEP_EFFECT_MUTATES_GRAPH),
    ];
    let mut seen = 0u64;
    let mut steps = Vec::with_capacity(specs.len());
    for (kind, policy, effect_flags) in specs {
        steps.push(PlanStepV2 {
            kind,
            phase: PlanStepPhase::ParallelExtract,
            policy,
            gate_flags: 0,
            requires_mask: seen,
            effect_flags,
        });
        seen |= kind.bit();
    }
    steps
}

fn sequential_steps_v2() -> Vec<PlanStepV2> {
    use PlanStepPolicy::*;
    use PlanStepV2Kind::*;

    let specs = [
        (SequentialDefinitions, Required, 0),
        (SequentialK8s, IgnoreErr, 0),
        (
            SequentialLspCross,
            IgnoreErr,
            PLAN_STEP_GATE_REQUIRES_RESULT_CACHE,
        ),
        (SequentialCalls, Required, 0),
        (SequentialUsages, Required, 0),
        (SequentialSemantic, Required, 0),
    ];
    let mut seen = 0u64;
    let mut steps = Vec::with_capacity(specs.len());
    for (kind, policy, gate_flags) in specs {
        steps.push(PlanStepV2 {
            kind,
            phase: PlanStepPhase::SequentialExtract,
            policy,
            gate_flags,
            requires_mask: seen,
            effect_flags: PLAN_STEP_EFFECT_MUTATES_GRAPH,
        });
        seen |= kind.bit();
    }
    steps
}

fn predump_steps_v2(mode: i32) -> Vec<PlanStepV2> {
    use PlanStepPolicy::Required;
    use PlanStepV2Kind::*;

    let specs = [
        (PredumpDecoratorTags, 0),
        (PredumpConfiglink, 0),
        (PredumpRouteMatch, 0),
        (PredumpSimilarity, PLAN_STEP_GATE_SKIP_FAST),
        (PredumpSemanticEdges, PLAN_STEP_GATE_SKIP_FAST),
        (PredumpComplexity, 0),
    ];
    let mut seen = 0u64;
    let mut steps = Vec::with_capacity(specs.len());
    for (kind, gate_flags) in specs {
        if gate_flags & PLAN_STEP_GATE_SKIP_FAST != 0 && mode == MODE_FAST {
            continue;
        }
        steps.push(PlanStepV2 {
            kind,
            phase: PlanStepPhase::Predump,
            policy: Required,
            gate_flags,
            requires_mask: seen,
            effect_flags: PLAN_STEP_EFFECT_MUTATES_GRAPH,
        });
        seen |= kind.bit();
    }
    steps
}

pub fn full_pipeline_top_steps(
    mode: i32,
    worker_count: i32,
    file_count: i32,
) -> Vec<TopLevelPlanStep> {
    use TopStepKind::*;
    use TopStepPolicy::*;

    let extraction_plan = if extraction_choice(worker_count, file_count) == "parallel" {
        PlanKind::ParallelExtraction as i32
    } else {
        PlanKind::Sequential as i32
    };

    let mut specs = vec![
        (MacroExtraction, FullModeOnly, 0, 0, PLAN_TOP_NO_NESTED_PLAN),
        (UserconfigLoad, FailOpen, 0, 0, PLAN_TOP_NO_NESTED_PLAN),
        (Discover, Required, 0, 0, PLAN_TOP_NO_NESTED_PLAN),
        (
            TryIncrementalOrDeleteDb,
            ExistingDbOnly,
            PLAN_TOP_GATE_MAY_SHORT_CIRCUIT,
            PLAN_TOP_EFFECT_WRITES_STORE_OR_ARTIFACT,
            PLAN_TOP_NO_NESTED_PLAN,
        ),
        (
            Structure,
            Required,
            0,
            PLAN_TOP_EFFECT_MUTATES_GRAPH,
            PLAN_TOP_NO_NESTED_PLAN,
        ),
        (
            ExtractionDispatch,
            Required,
            0,
            PLAN_TOP_EFFECT_MUTATES_GRAPH,
            extraction_plan,
        ),
        (
            Tests,
            Required,
            0,
            PLAN_TOP_EFFECT_MUTATES_GRAPH,
            PLAN_TOP_NO_NESTED_PLAN,
        ),
    ];

    if mode != MODE_FAST {
        specs.push((
            Githistory,
            Required,
            PLAN_TOP_GATE_SKIP_FAST,
            PLAN_TOP_EFFECT_MUTATES_GRAPH,
            PLAN_TOP_NO_NESTED_PLAN,
        ));
    }

    specs.extend([
        (
            Predump,
            Required,
            0,
            PLAN_TOP_EFFECT_MUTATES_GRAPH,
            PlanKind::Predump as i32,
        ),
        (
            Dump,
            Required,
            0,
            PLAN_TOP_EFFECT_WRITES_STORE_OR_ARTIFACT,
            PLAN_TOP_NO_NESTED_PLAN,
        ),
        (
            PersistHashes,
            BestEffort,
            0,
            PLAN_TOP_EFFECT_WRITES_STORE_OR_ARTIFACT,
            PLAN_TOP_NO_NESTED_PLAN,
        ),
        (
            ArtifactExport,
            OptionalPersistence,
            0,
            PLAN_TOP_EFFECT_WRITES_STORE_OR_ARTIFACT,
            PLAN_TOP_NO_NESTED_PLAN,
        ),
    ]);

    let mut seen = 0u64;
    let mut steps = Vec::with_capacity(specs.len());
    for (kind, policy, gate_flags, effect_flags, nested_plan_kind) in specs {
        steps.push(TopLevelPlanStep {
            kind,
            phase: TopStepPhase::FullPipeline,
            policy,
            gate_flags,
            requires_mask: seen,
            effect_flags,
            nested_plan_kind,
        });
        seen |= kind.bit();
    }
    steps
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
    fn typed_v2_step_kind_fits_requires_mask_contract() {
        assert!((PlanStepV2Kind::IncrementalResolve as u32) <= MAX_PLAN_STEP_V2_KIND);
    }

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
    fn predump_typed_v2_metadata_matches_string_plan() {
        use PlanStepPhase::Predump;
        use PlanStepPolicy::Required;
        use PlanStepV2Kind::*;

        let full = steps_v2(PlanKind::Predump, MODE_MODERATE, 99, 99).unwrap();
        assert_eq!(full.len(), 6);
        assert_eq!(
            full.iter()
                .map(|step| step.kind.as_name())
                .collect::<Vec<_>>()
                .join(","),
            describe(PlanKind::Predump, MODE_MODERATE, 0, 0)
        );
        assert_eq!(full[0].kind, PredumpDecoratorTags);
        assert_eq!(full[0].phase, Predump);
        assert_eq!(full[0].policy, Required);
        assert_eq!(full[0].gate_flags, 0);
        assert_eq!(full[0].requires_mask, 0);
        assert_eq!(full[0].effect_flags, PLAN_STEP_EFFECT_MUTATES_GRAPH);
        assert_eq!(full[3].kind, PredumpSimilarity);
        assert_eq!(full[3].gate_flags, PLAN_STEP_GATE_SKIP_FAST);
        assert_eq!(
            full[3].requires_mask,
            PredumpDecoratorTags.bit() | PredumpConfiglink.bit() | PredumpRouteMatch.bit()
        );
        assert_eq!(full[5].kind, PredumpComplexity);
        assert_eq!(
            full[5].requires_mask,
            PredumpDecoratorTags.bit()
                | PredumpConfiglink.bit()
                | PredumpRouteMatch.bit()
                | PredumpSimilarity.bit()
                | PredumpSemanticEdges.bit()
        );

        let fast = steps_v2(PlanKind::Predump, MODE_FAST, 0, 0).unwrap();
        assert_eq!(fast.len(), 4);
        assert_eq!(fast[3].kind, PredumpComplexity);
        assert_eq!(
            fast[3].requires_mask,
            PredumpDecoratorTags.bit() | PredumpConfiglink.bit() | PredumpRouteMatch.bit()
        );
        assert_eq!(
            fast.iter()
                .map(|step| step.kind.as_name())
                .collect::<Vec<_>>()
                .join(","),
            describe(PlanKind::Predump, MODE_FAST, 0, 0)
        );
        assert!(steps_v2(PlanKind::FullPipeline, 0, 0, 0).is_none());
    }

    #[test]
    fn parallel_typed_v2_metadata_captures_outer_dispatch() {
        use PlanStepPhase::ParallelExtract as ParallelExtractPhase;
        use PlanStepPolicy::*;
        use PlanStepV2Kind::*;

        let steps = steps_v2(PlanKind::ParallelExtraction, MODE_MODERATE, 2, 51).unwrap();
        assert_eq!(steps.len(), 7);
        assert_eq!(
            steps
                .iter()
                .map(|step| step.kind.as_name())
                .collect::<Vec<_>>()
                .join(","),
            "parallel_extract,registry_build,lsp_cross_prepare,parallel_resolve,\
             infra_routes,infra_bindings,k8s"
        );
        assert_eq!(steps[0].kind, ParallelExtract);
        assert_eq!(steps[0].phase, ParallelExtractPhase);
        assert_eq!(steps[0].policy, Required);
        assert_eq!(steps[0].requires_mask, 0);
        assert_eq!(steps[0].effect_flags, PLAN_STEP_EFFECT_MUTATES_GRAPH);
        assert_eq!(steps[1].kind, ParallelRegistryBuild);
        assert_eq!(steps[1].policy, Required);
        assert_eq!(steps[1].requires_mask, ParallelExtract.bit());
        assert_eq!(steps[1].effect_flags, PLAN_STEP_EFFECT_MUTATES_GRAPH);

        assert_eq!(steps[2].kind, ParallelLspCrossPrepare);
        assert_eq!(steps[2].policy, EnvOptional);
        assert_eq!(
            steps[2].requires_mask,
            ParallelExtract.bit() | ParallelRegistryBuild.bit()
        );
        assert_eq!(steps[2].effect_flags, 0);

        assert_eq!(steps[3].kind, ParallelResolve);
        assert_eq!(steps[3].policy, Required);
        assert_eq!(
            steps[3].requires_mask,
            ParallelExtract.bit() | ParallelRegistryBuild.bit() | ParallelLspCrossPrepare.bit()
        );
        assert_eq!(steps[3].effect_flags, PLAN_STEP_EFFECT_MUTATES_GRAPH);

        assert_eq!(steps[6].kind, ParallelK8s);
        assert_eq!(steps[6].policy, IgnoreErr);
        assert_eq!(
            steps[6].requires_mask,
            ParallelExtract.bit()
                | ParallelRegistryBuild.bit()
                | ParallelLspCrossPrepare.bit()
                | ParallelResolve.bit()
                | ParallelInfraRoutes.bit()
                | ParallelInfraBindings.bit()
        );
        assert_eq!(steps[6].effect_flags, PLAN_STEP_EFFECT_MUTATES_GRAPH);
    }

    #[test]
    fn full_pipeline_top_level_metadata_captures_outer_orchestration() {
        use TopStepKind::*;
        use TopStepPhase::FullPipeline;
        use TopStepPolicy::*;

        let full = full_pipeline_top_steps(MODE_MODERATE, 2, 51);
        assert_eq!(full.len(), 12);
        assert_eq!(full[0].kind, MacroExtraction);
        assert_eq!(full[0].phase, FullPipeline);
        assert_eq!(full[0].policy, FullModeOnly);
        assert_eq!(full[0].nested_plan_kind, PLAN_TOP_NO_NESTED_PLAN);
        assert_eq!(full[1].kind, UserconfigLoad);
        assert_eq!(full[1].policy, FailOpen);
        assert_eq!(full[3].kind, TryIncrementalOrDeleteDb);
        assert_eq!(full[3].policy, ExistingDbOnly);
        assert_eq!(full[3].gate_flags, PLAN_TOP_GATE_MAY_SHORT_CIRCUIT);
        assert_eq!(
            full[3].effect_flags,
            PLAN_TOP_EFFECT_WRITES_STORE_OR_ARTIFACT
        );
        assert_eq!(full[5].kind, ExtractionDispatch);
        assert_eq!(
            full[5].requires_mask,
            MacroExtraction.bit()
                | UserconfigLoad.bit()
                | Discover.bit()
                | TryIncrementalOrDeleteDb.bit()
                | Structure.bit()
        );
        assert_eq!(full[5].effect_flags, PLAN_TOP_EFFECT_MUTATES_GRAPH);
        assert_eq!(
            full[5].nested_plan_kind,
            PlanKind::ParallelExtraction as i32
        );
        assert_eq!(full[7].kind, Githistory);
        assert_eq!(full[7].gate_flags, PLAN_TOP_GATE_SKIP_FAST);
        assert_eq!(full[8].kind, Predump);
        assert_eq!(full[8].nested_plan_kind, PlanKind::Predump as i32);
        assert_eq!(
            full[8].requires_mask,
            MacroExtraction.bit()
                | UserconfigLoad.bit()
                | Discover.bit()
                | TryIncrementalOrDeleteDb.bit()
                | Structure.bit()
                | ExtractionDispatch.bit()
                | Tests.bit()
                | Githistory.bit()
        );
        assert_eq!(full[9].kind, Dump);
        assert_eq!(
            full[9].effect_flags,
            PLAN_TOP_EFFECT_WRITES_STORE_OR_ARTIFACT
        );
        assert_eq!(full[10].kind, PersistHashes);
        assert_eq!(full[10].policy, BestEffort);
        assert_eq!(full[11].kind, ArtifactExport);
        assert_eq!(full[11].policy, OptionalPersistence);

        let fast = full_pipeline_top_steps(MODE_FAST, 2, 51);
        assert_eq!(fast.len(), 11);
        assert!(!fast.iter().any(|step| step.kind == Githistory));
        assert_eq!(fast[7].kind, Predump);
        assert_eq!(
            fast[7].requires_mask,
            MacroExtraction.bit()
                | UserconfigLoad.bit()
                | Discover.bit()
                | TryIncrementalOrDeleteDb.bit()
                | Structure.bit()
                | ExtractionDispatch.bit()
                | Tests.bit()
        );

        let sequential = full_pipeline_top_steps(MODE_MODERATE, 1, 99);
        assert_eq!(sequential[5].kind, ExtractionDispatch);
        assert_eq!(sequential[5].nested_plan_kind, PlanKind::Sequential as i32);
        assert!(steps_v2(PlanKind::FullPipeline, MODE_MODERATE, 2, 51).is_none());
    }

    #[test]
    fn sequential_typed_v2_metadata_captures_dispatch_loop() {
        use PlanStepPhase::SequentialExtract;
        use PlanStepPolicy::*;
        use PlanStepV2Kind::*;

        let steps = steps_v2(PlanKind::Sequential, MODE_MODERATE, 99, 99).unwrap();
        assert_eq!(steps.len(), 6);
        assert_eq!(
            steps
                .iter()
                .map(|step| step.kind.as_name())
                .collect::<Vec<_>>()
                .join(","),
            "definitions,k8s,lsp_cross,calls,usages,semantic"
        );
        assert_eq!(steps[0].kind, SequentialDefinitions);
        assert_eq!(steps[0].phase, SequentialExtract);
        assert_eq!(steps[0].policy, Required);
        assert_eq!(steps[0].gate_flags, 0);
        assert_eq!(steps[0].requires_mask, 0);
        assert_eq!(steps[0].effect_flags, PLAN_STEP_EFFECT_MUTATES_GRAPH);

        assert_eq!(steps[1].kind, SequentialK8s);
        assert_eq!(steps[1].policy, IgnoreErr);
        assert_eq!(steps[1].requires_mask, SequentialDefinitions.bit());

        assert_eq!(steps[2].kind, SequentialLspCross);
        assert_eq!(steps[2].policy, IgnoreErr);
        assert_eq!(steps[2].gate_flags, PLAN_STEP_GATE_REQUIRES_RESULT_CACHE);
        assert_eq!(
            steps[2].requires_mask,
            SequentialDefinitions.bit() | SequentialK8s.bit()
        );

        assert_eq!(steps[5].kind, SequentialSemantic);
        assert_eq!(
            steps[5].requires_mask,
            SequentialDefinitions.bit()
                | SequentialK8s.bit()
                | SequentialLspCross.bit()
                | SequentialCalls.bit()
                | SequentialUsages.bit()
        );
    }

    #[test]
    fn incremental_extract_resolve_typed_v2_metadata_matches_c_dispatch() {
        use PlanStepPhase::IncrementalExtractResolve;
        use PlanStepPolicy::IgnoreErr;
        use PlanStepV2Kind::*;

        let sequential =
            steps_v2(PlanKind::IncrementalExtractResolve, MODE_MODERATE, 2, 50).unwrap();
        assert_eq!(sequential.len(), 4);
        assert_eq!(
            sequential
                .iter()
                .map(|step| step.kind.as_name())
                .collect::<Vec<_>>()
                .join(","),
            "definitions,calls,usages,semantic"
        );
        assert_eq!(sequential[0].kind, IncrementalDefinitions);
        assert_eq!(sequential[0].phase, IncrementalExtractResolve);
        assert_eq!(sequential[0].policy, IgnoreErr);
        assert_eq!(sequential[0].gate_flags, 0);
        assert_eq!(sequential[0].requires_mask, 0);
        assert_eq!(sequential[0].effect_flags, PLAN_STEP_EFFECT_MUTATES_GRAPH);
        assert_eq!(sequential[3].kind, IncrementalSemantic);
        assert_eq!(
            sequential[3].requires_mask,
            IncrementalDefinitions.bit() | IncrementalCalls.bit() | IncrementalUsages.bit()
        );
        assert_eq!(
            sequential
                .iter()
                .map(|step| step.kind.as_name())
                .zip(sequential.iter().map(|step| step.policy.as_name()))
                .map(|(name, policy)| format!("{name}:{policy}"))
                .collect::<Vec<_>>()
                .join(","),
            describe(PlanKind::IncrementalExtractResolve, MODE_MODERATE, 2, 50)
        );

        let parallel = steps_v2(PlanKind::IncrementalExtractResolve, 0, 2, 51).unwrap();
        assert_eq!(parallel.len(), 3);
        assert_eq!(
            parallel
                .iter()
                .map(|step| step.kind.as_name())
                .collect::<Vec<_>>()
                .join(","),
            "incr_extract,incr_registry,incr_resolve"
        );
        assert_eq!(parallel[0].kind, IncrementalParallelExtract);
        assert_eq!(parallel[0].phase, IncrementalExtractResolve);
        assert_eq!(parallel[0].policy, IgnoreErr);
        assert_eq!(parallel[0].requires_mask, 0);
        assert_eq!(parallel[1].kind, IncrementalRegistry);
        assert_eq!(parallel[1].requires_mask, IncrementalParallelExtract.bit());
        assert_eq!(parallel[2].kind, IncrementalResolve);
        assert_eq!(parallel[2].gate_flags, PLAN_STEP_GATE_NO_CROSS_LSP_PREBUILD);
        assert_eq!(
            parallel[2].requires_mask,
            IncrementalParallelExtract.bit() | IncrementalRegistry.bit()
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
    fn incremental_post_typed_steps_capture_tail_policy() {
        use PlanStepKind::*;
        use PlanStepPolicy::*;

        let full = incremental_post_steps(MODE_MODERATE);
        assert_eq!(full.len(), 10);
        assert_eq!(
            full,
            vec![
                PlanStep {
                    kind: K8s,
                    policy: IgnoreErr
                },
                PlanStep {
                    kind: IncrTests,
                    policy: IgnoreErr
                },
                PlanStep {
                    kind: IncrDecoratorTags,
                    policy: IgnoreErr
                },
                PlanStep {
                    kind: IncrConfiglink,
                    policy: IgnoreErr
                },
                PlanStep {
                    kind: IncrSimilarity,
                    policy: IgnoreErr
                },
                PlanStep {
                    kind: IncrSemanticEdges,
                    policy: IgnoreErr
                },
                PlanStep {
                    kind: EdgeRelink,
                    policy: BestEffort
                },
                PlanStep {
                    kind: IncrementalDump,
                    policy: BestEffort
                },
                PlanStep {
                    kind: PersistHashes,
                    policy: BestEffort
                },
                PlanStep {
                    kind: ArtifactExport,
                    policy: OptionalExistingArtifact
                },
            ]
        );

        let fast = incremental_post_steps(MODE_FAST);
        assert_eq!(fast.len(), 8);
        assert_eq!(fast[4].kind, EdgeRelink);
        assert_eq!(fast[5].kind, IncrementalDump);
        assert_eq!(fast[6].kind, PersistHashes);
        assert_eq!(fast[7].kind, ArtifactExport);
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
