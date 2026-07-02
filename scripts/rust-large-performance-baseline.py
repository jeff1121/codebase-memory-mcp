#!/usr/bin/env python3
"""產生並驗證 Rust 重構 Phase 0 的大型效能 baseline。

此手動 gate 目前支援 macOS/Linux；Windows RSS 取樣需另建等價實作。
"""

from __future__ import annotations

import argparse
import json
import os
import platform
import subprocess
import sys
import tempfile
import time
from pathlib import Path
from typing import Any

try:
    import resource
except ModuleNotFoundError:  # pragma: no cover - Windows fallback path
    resource = None


ROOT = Path(__file__).resolve().parents[1]
GOLDEN_PATH = ROOT / "tests" / "golden" / "rust-refactor" / "large-performance-baseline.json"
PROJECT = "rust_refactor_large_performance"
TIMEOUT = 900
INDEX_GUARD_MS = 15 * 60 * 1000
SMOKE_GUARD_MS = 45 * 1000
RSS_GUARD_KB = 4 * 1024 * 1024
BINARY_SIZE_GUARD_RATIO = 1.20
FILES_PER_LANG = 96
FUNCTIONS_PER_FILE = 16
SUPPORTED_PLATFORMS = {"darwin", "linux"}


class BaselineError(RuntimeError):
    pass


def canonical_json(value: Any) -> str:
    return json.dumps(value, ensure_ascii=False, indent=2, sort_keys=True) + "\n"


def load_json(path: Path) -> Any:
    with path.open("r", encoding="utf-8") as f:
        return json.load(f)


def write_json(path: Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(canonical_json(value), encoding="utf-8")


def normalize_path(path: Path) -> str:
    return str(path).replace("\\", "/")


def display_path(path: Path) -> str:
    try:
        return normalize_path(path.relative_to(ROOT))
    except ValueError:
        return f"<path>/{path.name}"


def display_cmd(cmd: list[str]) -> str:
    rendered: list[str] = []
    for idx, arg in enumerate(cmd):
        if idx == 0:
            rendered.append(display_path(Path(arg)))
        elif arg.startswith("{") or arg.startswith("["):
            rendered.append("<json>")
        elif "/" in arg or "\\" in arg:
            rendered.append(display_path(Path(arg)))
        else:
            rendered.append(arg)
    return " ".join(rendered)


def make_env(tmp: Path) -> dict[str, str]:
    env = {key: value for key, value in os.environ.items() if not key.startswith("CBM_")}
    home = tmp / "home"
    config = tmp / "config"
    appdata = tmp / "appdata"
    local = tmp / "localappdata"
    cache = tmp / "cache"
    for p in (home, config, appdata, local, cache):
        p.mkdir(parents=True, exist_ok=True)
    env.update(
        {
            "HOME": str(home),
            "USERPROFILE": str(home),
            "XDG_CONFIG_HOME": str(config),
            "APPDATA": str(appdata),
            "LOCALAPPDATA": str(local),
            "CBM_CACHE_DIR": str(cache),
            "CBM_LOG_FORMAT": "text",
            "CBM_WORKERS": "1",
            "LC_ALL": "C",
            "TZ": "UTC",
        }
    )
    return env


def child_peak_rss_kb() -> int:
    if resource is None:
        raise BaselineError("此平台尚未支援 child peak RSS 取樣")
    value = int(resource.getrusage(resource.RUSAGE_CHILDREN).ru_maxrss)
    if sys.platform == "darwin":
        return value // 1024
    return value


def run_cmd(
    cmd: list[str],
    env: dict[str, str],
    *,
    timeout: int = TIMEOUT,
    cwd: Path = ROOT,
) -> tuple[str, str, float]:
    start = time.perf_counter()
    proc = subprocess.run(cmd, text=True, capture_output=True, env=env, timeout=timeout, cwd=cwd)
    elapsed_ms = (time.perf_counter() - start) * 1000.0
    if proc.returncode != 0:
        raise BaselineError(
            "命令失敗: {}\nrc={}\nstdout={}\nstderr={}".format(
                display_cmd(cmd), proc.returncode, proc.stdout[-2000:], proc.stderr[-2000:]
            )
        )
    return proc.stdout, proc.stderr, elapsed_ms


def parse_tool_value(value: Any) -> Any:
    if isinstance(value, dict) and "structuredContent" in value:
        return value["structuredContent"]
    if isinstance(value, dict) and isinstance(value.get("content"), list) and value["content"]:
        first = value["content"][0]
        text = first.get("text") if isinstance(first, dict) else None
        if isinstance(text, str):
            try:
                return json.loads(text)
            except json.JSONDecodeError:
                return text
    return value


def parse_tool_result(raw: str) -> Any:
    data = json.loads(raw)
    if isinstance(data, dict) and "structuredContent" in data:
        return data["structuredContent"]
    if isinstance(data, dict) and "result" in data:
        return parse_tool_value(data["result"])
    return parse_tool_value(data)


def run_cli_tool(
    binary: Path,
    env: dict[str, str],
    tool: str,
    args: dict[str, Any],
    *,
    timeout: int = TIMEOUT,
) -> tuple[Any, float]:
    raw, _err, elapsed_ms = run_cmd(
        [str(binary), "cli", "--json", tool, json.dumps(args, separators=(",", ":"))],
        env,
        timeout=timeout,
    )
    return parse_tool_result(raw), elapsed_ms


def write_file(path: Path, text: str) -> int:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")
    return len(text.splitlines())


def generate_python_file(i: int) -> str:
    lines = [
        f'"""Generated Python module {i} for Rust refactor performance baseline."""',
        "",
    ]
    if i > 0:
        prev = i - 1
        lines.append(f"from pkg_{prev % 12:02d}.mod_{prev:03d} import py_func_{prev:03d}_00")
        lines.append("")
    for j in range(FUNCTIONS_PER_FILE):
        name = f"py_func_{i:03d}_{j:02d}"
        callee = f"py_func_{i:03d}_{j - 1:02d}" if j > 0 else "int"
        lines.extend(
            [
                f"def {name}(value):",
                f"    total = {callee}(value) if value else value",
                f"    return total + {i + j}",
                "",
            ]
        )
    lines.extend(
        [
            f"class PyWorker{i:03d}:",
            "    def __init__(self, seed):",
            "        self.seed = seed",
            "",
            "    def run(self, value):",
            f"        return py_func_{i:03d}_{FUNCTIONS_PER_FILE - 1:02d}(value) + self.seed",
            "",
        ]
    )
    return "\n".join(lines)


def generate_typescript_file(i: int) -> str:
    lines = [f"// Generated TypeScript module {i}", ""]
    if i > 0:
        prev = i - 1
        lines.append(f'import {{ tsFunc{prev:03d}_00 }} from "./mod_{prev:03d}";')
        lines.append("")
    for j in range(FUNCTIONS_PER_FILE):
        name = f"tsFunc{i:03d}_{j:02d}"
        callee = f"tsFunc{i:03d}_{j - 1:02d}" if j > 0 else "Number"
        lines.extend(
            [
                f"export function {name}(value: number): number {{",
                f"  const total = value ? {callee}(value) : value;",
                f"  return total + {i + j};",
                "}",
                "",
            ]
        )
    lines.extend(
        [
            f"export class TsWorker{i:03d} {{",
            "  constructor(private seed: number) {}",
            "  run(value: number): number {",
            f"    return tsFunc{i:03d}_{FUNCTIONS_PER_FILE - 1:02d}(value) + this.seed;",
            "  }",
            "}",
            "",
        ]
    )
    return "\n".join(lines)


def generate_go_file(i: int) -> str:
    package = f"pkg{i % 8:02d}"
    lines = [f"package {package}", ""]
    for j in range(FUNCTIONS_PER_FILE):
        name = f"GoFunc{i:03d}_{j:02d}"
        callee = f"GoFunc{i:03d}_{j - 1:02d}" if j > 0 else ""
        lines.extend([f"func {name}(value int) int {{"])
        if callee:
            lines.append(f"    total := {callee}(value)")
        else:
            lines.append("    total := value")
        lines.extend([f"    return total + {i + j}", "}", ""])
    lines.extend(
        [
            f"type GoWorker{i:03d} struct {{",
            "    Seed int",
            "}",
            "",
            f"func (w GoWorker{i:03d}) Run(value int) int {{",
            f"    return GoFunc{i:03d}_{FUNCTIONS_PER_FILE - 1:02d}(value) + w.Seed",
            "}",
            "",
        ]
    )
    return "\n".join(lines)


def generate_c_file(i: int) -> str:
    lines = [f"/* Generated C module {i}. */", ""]
    for j in range(FUNCTIONS_PER_FILE):
        name = f"c_func_{i:03d}_{j:02d}"
        callee = f"c_func_{i:03d}_{j - 1:02d}" if j > 0 else ""
        lines.append(f"int {name}(int value) {{")
        if callee:
            lines.append(f"    int total = {callee}(value);")
        else:
            lines.append("    int total = value;")
        lines.extend([f"    return total + {i + j};", "}", ""])
    lines.extend(
        [
            f"typedef struct CWorker{i:03d} {{",
            "    int seed;",
            f"}} CWorker{i:03d};",
            "",
            f"int c_worker_{i:03d}_run(CWorker{i:03d} worker, int value) {{",
            f"    return c_func_{i:03d}_{FUNCTIONS_PER_FILE - 1:02d}(value) + worker.seed;",
            "}",
            "",
        ]
    )
    return "\n".join(lines)


def generate_corpus(repo: Path) -> dict[str, Any]:
    generators = {
        "python": ("py", generate_python_file),
        "typescript": ("ts", generate_typescript_file),
        "go": ("go", generate_go_file),
        "c": ("c", generate_c_file),
    }
    per_language: dict[str, dict[str, int]] = {}
    total_files = 0
    total_loc = 0
    for lang, (ext, generator) in generators.items():
        loc = 0
        for i in range(FILES_PER_LANG):
            if lang == "python":
                path = repo / f"pkg_{i % 12:02d}" / f"mod_{i:03d}.{ext}"
            elif lang == "typescript":
                path = repo / "web" / "src" / f"mod_{i:03d}.{ext}"
            elif lang == "go":
                path = repo / "go" / f"pkg{i % 8:02d}" / f"file_{i:03d}.{ext}"
            else:
                path = repo / "csrc" / f"mod_{i:03d}.{ext}"
            loc += write_file(path, generator(i))
            total_files += 1
        total_loc += loc
        per_language[lang] = {"files": FILES_PER_LANG, "loc": loc}

    readme = "\n".join(
        [
            "# Generated Large Performance Fixture",
            "",
            "This repository is generated by scripts/rust-large-performance-baseline.py.",
            "It is intentionally deterministic and must not be committed as source.",
            "",
        ]
    )
    total_loc += write_file(repo / "README.md", readme)
    total_files += 1
    return {
        "kind": "generated-large-v1",
        "files_per_language": FILES_PER_LANG,
        "functions_per_file": FUNCTIONS_PER_FILE,
        "languages": per_language,
        "file_count": total_files,
        "loc": total_loc,
    }


def guard_bucket(value: float | int, guard: int, unit: str) -> dict[str, Any]:
    numeric = float(value)
    return {
        "bucket": "lte_guard" if numeric <= guard else "gt_guard",
        f"guard_{unit}": guard,
        "within_guard": numeric <= guard,
    }


def query_rows(value: Any) -> list[Any]:
    if isinstance(value, list):
        return value
    if isinstance(value, dict):
        for key in ("rows", "results", "data"):
            rows = value.get(key)
            if isinstance(rows, list):
                return rows
    return []


def stable_columns(value: Any) -> list[str]:
    columns = value.get("columns") if isinstance(value, dict) else None
    if isinstance(columns, list):
        return [str(c) for c in columns]
    return []


def schema_histogram(schema: Any, key: str, name_key: str) -> dict[str, int]:
    hist: dict[str, int] = {}
    items = schema.get(key) if isinstance(schema, dict) else None
    if not isinstance(items, list):
        return hist
    for item in items:
        if not isinstance(item, dict):
            continue
        name = item.get(name_key)
        count = item.get("count")
        if isinstance(name, str) and isinstance(count, int):
            hist[name] = count
    return dict(sorted(hist.items()))


def summarize_query(value: Any, elapsed_ms: float) -> dict[str, Any]:
    rows = query_rows(value)
    return {
        "latency": guard_bucket(elapsed_ms, SMOKE_GUARD_MS, "ms"),
        "columns": stable_columns(value),
        "row_count": len(rows),
        "total": value.get("total") if isinstance(value, dict) else None,
    }


def summarize_search_graph(value: Any, elapsed_ms: float) -> dict[str, Any]:
    results = value.get("results") if isinstance(value, dict) and isinstance(value.get("results"), list) else []
    return {
        "latency": guard_bucket(elapsed_ms, SMOKE_GUARD_MS, "ms"),
        "result_count": len(results),
        "total": value.get("total") if isinstance(value, dict) else None,
        "has_more": value.get("has_more") if isinstance(value, dict) else None,
    }


def summarize_search_code(value: Any, elapsed_ms: float) -> dict[str, Any]:
    files = value.get("files") if isinstance(value, dict) and isinstance(value.get("files"), list) else []
    return {
        "latency": guard_bucket(elapsed_ms, SMOKE_GUARD_MS, "ms"),
        "file_count": len(files),
        "raw_match_count": value.get("raw_match_count") if isinstance(value, dict) else None,
        "total_results": value.get("total_results") if isinstance(value, dict) else None,
    }


def build_baseline(binary: Path, env: dict[str, str], repo: Path) -> dict[str, Any]:
    corpus = generate_corpus(repo)
    rss_before = child_peak_rss_kb()
    index_result, index_ms = run_cli_tool(
        binary,
        env,
        "index_repository",
        {
            "repo_path": normalize_path(repo),
            "mode": "full",
            "name": PROJECT,
        },
        timeout=TIMEOUT,
    )
    peak_rss = max(0, child_peak_rss_kb() - rss_before)
    if not isinstance(index_result, dict) or index_result.get("status") != "indexed":
        raise BaselineError(f"index_repository 未成功完成: {index_result}")

    schema_result, schema_ms = run_cli_tool(
        binary, env, "get_graph_schema", {"project": PROJECT}, timeout=TIMEOUT
    )
    functions_query, functions_ms = run_cli_tool(
        binary,
        env,
        "query_graph",
        {
            "project": PROJECT,
            "query": "MATCH (f:Function) RETURN f.qualified_name LIMIT 10",
            "max_rows": 10,
        },
        timeout=TIMEOUT,
    )
    calls_query, calls_ms = run_cli_tool(
        binary,
        env,
        "query_graph",
        {
            "project": PROJECT,
            "query": "MATCH ()-[r:CALLS]->() RETURN type(r) LIMIT 10",
            "max_rows": 10,
        },
        timeout=TIMEOUT,
    )
    search_graph, search_graph_ms = run_cli_tool(
        binary,
        env,
        "search_graph",
        {"project": PROJECT, "query": "Worker run function", "limit": 10},
        timeout=TIMEOUT,
    )
    search_code, search_code_ms = run_cli_tool(
        binary,
        env,
        "search_code",
        {"project": PROJECT, "pattern": "Worker", "mode": "files", "limit": 10},
        timeout=TIMEOUT,
    )

    node_hist = schema_histogram(schema_result, "node_labels", "label")
    edge_hist = schema_histogram(schema_result, "edge_types", "type")
    excluded = index_result.get("excluded") if isinstance(index_result.get("excluded"), dict) else {}
    return {
        "schema_version": 1,
        "binary": {
            "platform": f"{sys.platform}-{platform.machine()}",
            "size_bytes": binary.stat().st_size,
            "size_guard_ratio": BINARY_SIZE_GUARD_RATIO,
        },
        "corpus": corpus,
        "index_repository": {
            "project": index_result.get("project"),
            "mode": "full",
            "status": index_result.get("status"),
            "nodes": index_result.get("nodes"),
            "edges": index_result.get("edges"),
            "expected_nodes": index_result.get("expected_nodes"),
            "expected_edges": index_result.get("expected_edges"),
            "artifact_present": index_result.get("artifact_present"),
            "excluded_dir_count": excluded.get("count"),
            "excluded_dir_truncated": excluded.get("truncated"),
            "wall_time": guard_bucket(index_ms, INDEX_GUARD_MS, "ms"),
            "child_peak_rss": guard_bucket(peak_rss, RSS_GUARD_KB, "kb"),
        },
        "node_label_histogram": node_hist,
        "edge_type_histogram": edge_hist,
        "latency_smoke": {
            "get_graph_schema": {
                "latency": guard_bucket(schema_ms, SMOKE_GUARD_MS, "ms"),
                "node_label_kinds": len(node_hist),
                "edge_type_kinds": len(edge_hist),
            },
            "query_functions": summarize_query(functions_query, functions_ms),
            "query_calls": summarize_query(calls_query, calls_ms),
            "search_graph": summarize_search_graph(search_graph, search_graph_ms),
            "search_code": summarize_search_code(search_code, search_code_ms),
        },
    }


def check_guards(value: Any, path: str = "") -> None:
    if isinstance(value, dict):
        if "within_guard" in value and not value.get("within_guard"):
            raise BaselineError(f"{path or 'guard'} 超過 guard: {value}")
        for key, child in value.items():
            check_guards(child, f"{path}.{key}" if path else str(key))
    elif isinstance(value, list):
        for idx, child in enumerate(value):
            check_guards(child, f"{path}[{idx}]")


def compare_baseline(expected: dict[str, Any], actual: dict[str, Any]) -> None:
    check_guards(actual)
    if expected.get("schema_version") != actual.get("schema_version"):
        raise BaselineError("large performance baseline schema_version 不一致")
    expected_binary = expected.get("binary", {})
    actual_binary = actual.get("binary", {})
    if expected_binary.get("platform") != actual_binary.get("platform"):
        raise BaselineError(
            f"platform 不一致: {expected_binary.get('platform')} != {actual_binary.get('platform')}"
        )
    expected_size = int(expected_binary.get("size_bytes", 0))
    actual_size = int(actual_binary.get("size_bytes", 0))
    guard_ratio = float(expected_binary.get("size_guard_ratio", BINARY_SIZE_GUARD_RATIO))
    if expected_size > 0 and actual_size > int(expected_size * guard_ratio):
        raise BaselineError(f"binary size 超過 guard: {actual_size} > {int(expected_size * guard_ratio)}")

    expected_cmp = json.loads(canonical_json(expected))
    actual_cmp = json.loads(canonical_json(actual))
    if expected_size > 0:
        actual_cmp["binary"]["size_bytes"] = expected_cmp["binary"]["size_bytes"]
    if expected_cmp != actual_cmp:
        raise BaselineError(
            "large performance baseline 不一致\n--- expected ---\n{}\n--- actual ---\n{}".format(
                canonical_json(expected_cmp), canonical_json(actual_cmp)
            )
        )


def assert_no_path_leaks(value: Any, paths: list[Path | str]) -> None:
    blob = canonical_json(value)
    leaks: list[str] = []
    for item in paths:
        text = normalize_path(Path(item)) if not isinstance(item, str) else item.replace("\\", "/")
        if len(text) > 1 and text in blob:
            leaks.append(text)
    if leaks:
        joined = "\n".join(sorted(set(leaks)))
        raise BaselineError(f"large performance baseline 含有本機或暫存路徑:\n{joined}")


def main() -> int:
    parser = argparse.ArgumentParser(description="驗證大型 synthetic performance baseline")
    parser.add_argument("binary", help="要驗證的 codebase-memory-mcp binary")
    parser.add_argument("--update", action="store_true", help="重寫 large performance baseline fixture")
    args = parser.parse_args()

    if sys.platform not in SUPPORTED_PLATFORMS:
        raise BaselineError("大型效能 baseline 目前只支援 macOS/Linux")

    binary = Path(args.binary).resolve()
    if not binary.exists():
        raise BaselineError(f"找不到 binary: {display_path(binary)}")

    with tempfile.TemporaryDirectory(prefix="cbm-rust-large-perf-") as tmp_s:
        tmp = Path(tmp_s)
        repo = tmp / "large-fixture"
        repo.mkdir(parents=True)
        env = make_env(tmp)
        baseline = build_baseline(binary, env, repo)
        leak_paths: list[Path | str] = [
            ROOT,
            tmp,
            repo,
            binary,
            *[
                value
                for key, value in env.items()
                if key
                in {
                    "HOME",
                    "USERPROFILE",
                    "XDG_CONFIG_HOME",
                    "APPDATA",
                    "LOCALAPPDATA",
                    "CBM_CACHE_DIR",
                }
            ],
        ]

    assert_no_path_leaks(baseline, leak_paths)
    if args.update:
        write_json(GOLDEN_PATH, baseline)
        print(f"更新大型效能 baseline: {display_path(GOLDEN_PATH)}")
        return 0

    if not GOLDEN_PATH.exists():
        raise BaselineError(
            f"缺少大型效能 baseline: {display_path(GOLDEN_PATH)}，請先執行 --update"
        )
    expected = load_json(GOLDEN_PATH)
    if not isinstance(expected, dict):
        raise BaselineError("large performance baseline fixture 必須是 JSON object")
    compare_baseline(expected, baseline)
    print("Rust large performance baseline passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (BaselineError, subprocess.TimeoutExpired) as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
