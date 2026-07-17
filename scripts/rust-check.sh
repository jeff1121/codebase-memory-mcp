#!/bin/bash
# rust-check.sh - Rust 重構階段的 Rust-side 檢查。
#
# 使用方式：
#   scripts/rust-check.sh

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

if ! command -v cargo >/dev/null 2>&1; then
    echo "ERROR: cargo is required for Rust refactor checks" >&2
    exit 1
fi

echo "=== rust-ci coverage check ==="
CI_COVERAGE_REPORT=$(awk '
  {
    split($0, lhs, ":");
    if (lhs[1] ~ /-only-test$/) {
      only[lhs[1]] = 1;
    }
  }
  /^rust-ci:/ {
    for (i = 2; i <= NF; i++) {
      ci[$i] = 1;
    }
  }
  /^rust-ci-tests:/ {
    for (i = 2; i <= NF; i++) {
      ci_tests[$i] = 1;
    }
  }
  END {
    missing_ci = 0;
    missing_ci_tests = 0;
    for (t in only) {
      if (!(t in ci)) {
        missing_ci = 1;
        printf "Missing in rust-ci: %s\n", t;
      }
      if (!(t in ci_tests)) {
        missing_ci_tests = 1;
        printf "Missing in rust-ci-tests: %s\n", t;
      }
    }
    if (missing_ci || missing_ci_tests) exit 1;
  }
' Makefile.cbm)
if [ -n "$CI_COVERAGE_REPORT" ]; then
    echo "$CI_COVERAGE_REPORT" >&2
fi
if echo "$CI_COVERAGE_REPORT" | grep -q "^Missing in rust-ci"; then
    exit 1
fi
if echo "$CI_COVERAGE_REPORT" | grep -q "^Missing in rust-ci-tests"; then
    exit 1
fi
if [ -n "$CI_COVERAGE_REPORT" ]; then
    echo "$CI_COVERAGE_REPORT"
fi

echo "=== cargo fmt ==="
cargo fmt --all -- --check

echo "=== cargo clippy ==="
cargo clippy --workspace --all-targets --all-features -- -D warnings

echo "=== cargo test ==="
cargo test --workspace --all-targets --all-features --locked

echo "=== Rust C ABI smoke test ==="
make -f Makefile.cbm rust-ffi-test

echo "=== Rust foundation opt-in matrix ==="
make -f Makefile.cbm rust-foundation-optin-test

echo "=== Rust YAML opt-in ==="
make -f Makefile.cbm rust-yaml-optin-test

echo "=== Rust-only YAML ==="
make -f Makefile.cbm rust-yaml-only-test

echo "=== Rust traces opt-in ==="
make -f Makefile.cbm rust-traces-optin-test

echo "=== Rust-only traces ==="
make -f Makefile.cbm rust-traces-only-test

echo "=== Rust CLI progress sink opt-in ==="
make -f Makefile.cbm rust-cli-progress-sink-optin-test

echo "=== Rust pipeline registry opt-in ==="
make -f Makefile.cbm rust-pipeline-registry-optin-test

echo "=== Rust pipeline registry test-QN opt-in ==="
make -f Makefile.cbm rust-pipeline-registry-test-qn-optin-test

echo "=== Rust pipeline plan opt-in ==="
make -f Makefile.cbm rust-pipeline-plan-optin-test

echo "=== Rust pipeline route canon opt-in ==="
make -f Makefile.cbm rust-pipeline-route-canon-optin-test

echo "=== Rust pipeline infra scan opt-in ==="
make -f Makefile.cbm rust-pipeline-infrascan-optin-test

echo "=== Rust pipeline githistory opt-in ==="
make -f Makefile.cbm rust-pipeline-githistory-optin-test

echo "=== Rust pipeline gitdiff range opt-in ==="
make -f Makefile.cbm rust-pipeline-gitdiff-range-optin-test

echo "=== Rust pipeline gitdiff name-status opt-in ==="
make -f Makefile.cbm rust-pipeline-gitdiff-name-status-optin-test

echo "=== Rust pipeline gitdiff hunks opt-in ==="
make -f Makefile.cbm rust-pipeline-gitdiff-hunks-optin-test

echo "=== Rust pipeline module directory opt-in ==="
make -f Makefile.cbm rust-pipeline-module-dir-optin-test

echo "=== Rust pipeline route argument opt-in ==="
make -f Makefile.cbm rust-pipeline-route-args-optin-test

echo "=== Rust pipeline route-node classifiers opt-in ==="
make -f Makefile.cbm rust-pipeline-route-node-classifiers-optin-test

echo "=== Rust pipeline K8s file classifiers opt-in ==="
make -f Makefile.cbm rust-pipeline-k8s-file-classifiers-optin-test

echo "=== Rust pipeline K8s file classifiers only-mode ==="
make -f Makefile.cbm rust-pipeline-k8s-file-classifiers-only-test

echo "=== Rust pipeline test detect opt-in ==="
make -f Makefile.cbm rust-pipeline-test-detect-optin-test

echo "=== Rust pipeline checked exception opt-in ==="
make -f Makefile.cbm rust-pipeline-checked-exception-optin-test

echo "=== Rust pipeline artifact path opt-in ==="
make -f Makefile.cbm rust-pipeline-artifact-path-optin-test

echo "=== Rust pipeline project-name opt-in ==="
make -f Makefile.cbm rust-pipeline-project-name-optin-test

echo "=== Rust pipeline configures opt-in ==="
make -f Makefile.cbm rust-pipeline-configures-optin-test

echo "=== Rust-only pipeline configures ==="
make -f Makefile.cbm rust-pipeline-configures-only-test

echo "=== Rust store FTS tokenizer opt-in ==="
make -f Makefile.cbm rust-store-fts-tokenizer-optin-test

echo "=== Rust store mmap resolver opt-in ==="
make -f Makefile.cbm rust-store-mmap-resolver-optin-test

echo "=== Rust store immutable URI opt-in ==="
make -f Makefile.cbm rust-store-immutable-uri-optin-test

echo "=== Rust store search pattern opt-in ==="
make -f Makefile.cbm rust-store-search-pattern-optin-test

echo "=== Rust store architecture helper opt-in ==="
make -f Makefile.cbm rust-store-arch-helper-optin-test

echo "=== Rust store architecture path scope opt-in ==="
make -f Makefile.cbm rust-store-arch-path-scope-optin-test

echo "=== Rust store file ext opt-in ==="
make -f Makefile.cbm rust-store-file-ext-optin-test

echo "=== Rust MCP codec opt-in ==="
make -f Makefile.cbm rust-mcp-codec-optin-test

echo "=== Rust Cypher scalar func opt-in ==="
make -f Makefile.cbm rust-cypher-scalar-func-optin-test

echo "=== Rust Cypher multiarg func opt-in ==="
make -f Makefile.cbm rust-cypher-multiarg-func-optin-test

echo "=== Rust Cypher aggregate func opt-in ==="
make -f Makefile.cbm rust-cypher-agg-func-optin-test

echo "=== Rust Cypher string func opt-in ==="
make -f Makefile.cbm rust-cypher-string-func-optin-test

echo "=== Rust Cypher single-char lexer opt-in ==="
make -f Makefile.cbm rust-cypher-lex-single-char-optin-test

echo "=== Rust Cypher two-char lexer opt-in ==="
make -f Makefile.cbm rust-cypher-lex-two-char-optin-test

echo "=== Rust store language map opt-in ==="
make -f Makefile.cbm rust-store-language-map-optin-test

echo "=== Rust language graph parity ==="
make -f Makefile.cbm rust-language-graph-parity
