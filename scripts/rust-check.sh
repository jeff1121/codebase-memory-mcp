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

echo "=== Rust pipeline registry opt-in ==="
make -f Makefile.cbm rust-pipeline-registry-optin-test

echo "=== Rust pipeline plan opt-in ==="
make -f Makefile.cbm rust-pipeline-plan-optin-test

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

echo "=== Rust MCP codec opt-in ==="
make -f Makefile.cbm rust-mcp-codec-optin-test

echo "=== Rust language graph parity ==="
make -f Makefile.cbm rust-language-graph-parity
