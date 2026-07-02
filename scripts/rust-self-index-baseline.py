#!/usr/bin/env python3
"""產生並驗證 Rust 重構 Phase 0 的 repository self-index baseline。"""

from __future__ import annotations

import argparse
import contextlib
import json
import os
import platform
import subprocess
import sys
import tempfile
import time
from pathlib import Path
from typing import Any, Iterator


ROOT = Path(__file__).resolve().parents[1]
GOLDEN_PATH = ROOT / "tests" / "golden" / "rust-refactor" / "self-index-baseline.json"
PROJECT = "rust_refactor_self_index_baseline"
TIMEOUT = 900
INDEX_GUARD_MS = 10 * 60 * 1000
SMOKE_GUARD_MS = 30 * 1000
BINARY_SIZE_GUARD_RATIO = 1.20


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


def is_relative_to(path: Path, parent: Path) -> bool:
    try:
        path.relative_to(parent)
        return True
    except ValueError:
        return False


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
                " ".join(cmd), proc.returncode, proc.stdout[-2000:], proc.stderr[-2000:]
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


def guard_bucket(elapsed_ms: float, guard_ms: int) -> dict[str, Any]:
    return {
        "bucket": "lte_guard" if elapsed_ms <= guard_ms else "gt_guard",
        "guard_ms": guard_ms,
        "within_guard": elapsed_ms <= guard_ms,
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


def scalar_values(value: Any) -> list[Any]:
    out: list[Any] = []
    if isinstance(value, dict):
        for v in value.values():
            out.extend(scalar_values(v))
    elif isinstance(value, list):
        for v in value:
            out.extend(scalar_values(v))
    elif isinstance(value, (str, int, float, bool)) or value is None:
        out.append(value)
    return out


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


def summarize_schema(schema: Any) -> dict[str, Any]:
    node_labels = []
    edge_types = []
    if isinstance(schema, dict):
        for item in schema.get("node_labels", []):
            if isinstance(item, dict):
                props = item.get("properties") if isinstance(item.get("properties"), list) else []
                node_labels.append(
                    {
                        "label": item.get("label"),
                        "count": item.get("count"),
                        "property_count": len(props),
                    }
                )
        for item in schema.get("edge_types", []):
            if isinstance(item, dict):
                props = item.get("properties") if isinstance(item.get("properties"), list) else []
                edge_types.append(
                    {
                        "type": item.get("type"),
                        "count": item.get("count"),
                        "property_count": len(props),
                    }
                )
    node_labels.sort(key=lambda item: str(item.get("label")))
    edge_types.sort(key=lambda item: str(item.get("type")))
    return {
        "top_level_keys": sorted(schema.keys()) if isinstance(schema, dict) else [],
        "node_labels": node_labels,
        "edge_types": edge_types,
    }


def stable_columns(value: Any) -> list[str]:
    columns = value.get("columns") if isinstance(value, dict) else None
    if isinstance(columns, list):
        return [str(c) for c in columns]
    return []


def summarize_query(value: Any, elapsed_ms: float) -> dict[str, Any]:
    rows = query_rows(value)
    return {
        "latency": guard_bucket(elapsed_ms, SMOKE_GUARD_MS),
        "columns": stable_columns(value),
        "row_count": len(rows),
        "total": value.get("total") if isinstance(value, dict) else None,
    }


def summarize_search_graph(value: Any, elapsed_ms: float) -> dict[str, Any]:
    results = value.get("results") if isinstance(value, dict) and isinstance(value.get("results"), list) else []
    return {
        "latency": guard_bucket(elapsed_ms, SMOKE_GUARD_MS),
        "top_level_keys": sorted(value.keys()) if isinstance(value, dict) else [],
        "result_count": len(results),
        "total": value.get("total") if isinstance(value, dict) else None,
        "has_more": value.get("has_more") if isinstance(value, dict) else None,
    }


def summarize_search_code(value: Any, elapsed_ms: float) -> dict[str, Any]:
    results = value.get("results") if isinstance(value, dict) and isinstance(value.get("results"), list) else []
    files = value.get("files") if isinstance(value, dict) and isinstance(value.get("files"), list) else []
    directories = (
        value.get("directories")
        if isinstance(value, dict) and isinstance(value.get("directories"), list)
        else []
    )
    return {
        "latency": guard_bucket(elapsed_ms, SMOKE_GUARD_MS),
        "top_level_keys": sorted(value.keys()) if isinstance(value, dict) else [],
        "result_count": len(results) if results else len(files),
        "file_count": len(files),
        "directory_count": len(directories),
        "raw_match_count": value.get("raw_match_count") if isinstance(value, dict) else None,
        "total_grep_matches": value.get("total_grep_matches") if isinstance(value, dict) else None,
        "total_results": value.get("total_results") if isinstance(value, dict) else None,
    }


def git_file_list(repo: Path) -> tuple[list[Path], str]:
    proc = subprocess.run(
        ["git", "-C", str(repo), "ls-files", "-co", "--exclude-standard", "-z"],
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        check=False,
    )
    if proc.returncode == 0:
        rels = [Path(p.decode("utf-8", errors="surrogateescape")) for p in proc.stdout.split(b"\0") if p]
        return rels, "git ls-files -co --exclude-standard"

    skipped_dirs = {".git", "build", "target", "node_modules", "__pycache__"}
    rels = []
    for dirpath, dirnames, filenames in os.walk(repo):
        dirnames[:] = [d for d in dirnames if d not in skipped_dirs]
        base = Path(dirpath)
        for filename in filenames:
            path = base / filename
            try:
                rels.append(path.relative_to(repo))
            except ValueError:
                continue
    return rels, "filesystem walk"


def is_binary_file(path: Path) -> bool:
    try:
        with path.open("rb") as f:
            return b"\0" in f.read(8192)
    except OSError:
        return True


def count_text_lines(path: Path) -> int:
    lines = 0
    last = b""
    try:
        with path.open("rb") as f:
            for chunk in iter(lambda: f.read(1024 * 1024), b""):
                lines += chunk.count(b"\n")
                if chunk:
                    last = chunk[-1:]
    except OSError:
        return 0
    if last and last != b"\n":
        lines += 1
    return lines


def repository_summary(repo: Path) -> dict[str, Any]:
    rels, basis = git_file_list(repo)
    fixture_rel: Path | None = None
    if is_relative_to(GOLDEN_PATH, repo):
        fixture_rel = GOLDEN_PATH.relative_to(repo)

    files = []
    self_fixture_excluded = False
    for rel in sorted(set(rels), key=lambda p: normalize_path(p)):
        if fixture_rel is not None and rel == fixture_rel:
            self_fixture_excluded = True
            continue
        full = repo / rel
        if full.is_file():
            files.append(rel)

    loc = 0
    text_files = 0
    binary_files = 0
    for rel in files:
        full = repo / rel
        if is_binary_file(full):
            binary_files += 1
            continue
        text_files += 1
        loc += count_text_lines(full)

    return {
        "count_basis": basis,
        "root": "default_repo_root" if repo == ROOT else "custom_repo",
        "file_count": len(files),
        "text_file_count": text_files,
        "binary_file_count": binary_files,
        "loc": loc,
        "self_fixture_excluded": self_fixture_excluded,
    }


@contextlib.contextmanager
def masked_self_fixture(repo: Path) -> Iterator[None]:
    if not is_relative_to(GOLDEN_PATH, repo):
        yield
        return

    original = GOLDEN_PATH.read_bytes() if GOLDEN_PATH.exists() else None
    GOLDEN_PATH.parent.mkdir(parents=True, exist_ok=True)
    GOLDEN_PATH.write_text("{}\n", encoding="utf-8")
    try:
        yield
    finally:
        if original is None:
            try:
                GOLDEN_PATH.unlink()
            except FileNotFoundError:
                pass
        else:
            GOLDEN_PATH.write_bytes(original)


def assert_no_path_leaks(value: Any, paths: list[Path | str]) -> None:
    blob = canonical_json(value)
    leaks: list[str] = []
    for item in paths:
        text = normalize_path(Path(item)) if not isinstance(item, str) else item.replace("\\", "/")
        if len(text) > 1 and text in blob:
            leaks.append(text)
    if leaks:
        joined = "\n".join(sorted(set(leaks)))
        raise BaselineError(f"baseline 含有本機或暫存路徑:\n{joined}")


def build_baseline(binary: Path, repo: Path, env: dict[str, str]) -> dict[str, Any]:
    repo_info = repository_summary(repo)

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
    if not isinstance(index_result, dict) or index_result.get("status") != "indexed":
        raise BaselineError(f"index_repository 未成功完成: {index_result}")

    schema_result, schema_ms = run_cli_tool(
        binary, env, "get_graph_schema", {"project": PROJECT}, timeout=TIMEOUT
    )
    function_query, function_ms = run_cli_tool(
        binary,
        env,
        "query_graph",
        {
            "project": PROJECT,
            "query": "MATCH (f:Function) RETURN f.qualified_name LIMIT 5",
            "max_rows": 5,
        },
        timeout=TIMEOUT,
    )
    edge_query, edge_ms = run_cli_tool(
        binary,
        env,
        "query_graph",
        {
            "project": PROJECT,
            "query": "MATCH ()-[r]->() RETURN type(r) LIMIT 5",
            "max_rows": 5,
        },
        timeout=TIMEOUT,
    )
    search_graph, search_graph_ms = run_cli_tool(
        binary,
        env,
        "search_graph",
        {"project": PROJECT, "query": "index repository", "limit": 5},
        timeout=TIMEOUT,
    )
    search_code, search_code_ms = run_cli_tool(
        binary,
        env,
        "search_code",
        {"project": PROJECT, "pattern": "index_repository", "mode": "files", "limit": 5},
        timeout=TIMEOUT,
    )

    excluded = index_result.get("excluded") if isinstance(index_result.get("excluded"), dict) else {}
    schema_summary = summarize_schema(schema_result)
    node_hist = schema_histogram(schema_result, "node_labels", "label")
    edge_hist = schema_histogram(schema_result, "edge_types", "type")

    return {
        "binary": {
            "platform": f"{sys.platform}-{platform.machine()}",
            "size_bytes": binary.stat().st_size,
            "size_guard_ratio": BINARY_SIZE_GUARD_RATIO,
        },
        "repository": repo_info,
        "index_repository": {
            "project": index_result.get("project"),
            "mode": "full",
            "status": index_result.get("status"),
            "nodes": index_result.get("nodes"),
            "edges": index_result.get("edges"),
            "expected_nodes": index_result.get("expected_nodes"),
            "expected_edges": index_result.get("expected_edges"),
            "artifact_present": index_result.get("artifact_present"),
            "adr_present": index_result.get("adr_present"),
            "excluded_dir_count": excluded.get("count"),
            "excluded_dir_truncated": excluded.get("truncated"),
            "wall_time": guard_bucket(index_ms, INDEX_GUARD_MS),
        },
        "schema_summary": schema_summary,
        "node_label_histogram": node_hist,
        "edge_type_histogram": edge_hist,
        "latency_smoke": {
            "get_graph_schema": {
                "latency": guard_bucket(schema_ms, SMOKE_GUARD_MS),
                "node_label_kinds": len(node_hist),
                "edge_type_kinds": len(edge_hist),
            },
            "query_functions": summarize_query(function_query, function_ms),
            "query_edges": summarize_query(edge_query, edge_ms),
            "search_graph": summarize_search_graph(search_graph, search_graph_ms),
            "search_code": summarize_search_code(search_code, search_code_ms),
        },
    }


def check_latency_guards(value: Any, path: str = "") -> None:
    if isinstance(value, dict):
        if {"bucket", "guard_ms", "within_guard"}.issubset(value.keys()) and not value.get(
            "within_guard"
        ):
            raise BaselineError(f"{path or 'latency'} 超過 guard: {value}")
        for key, child in value.items():
            check_latency_guards(child, f"{path}.{key}" if path else str(key))
    elif isinstance(value, list):
        for idx, child in enumerate(value):
            check_latency_guards(child, f"{path}[{idx}]")


def compare_baseline(expected: dict[str, Any], actual: dict[str, Any]) -> None:
    check_latency_guards(actual)

    expected_binary = expected.get("binary", {})
    actual_binary = actual.get("binary", {})
    if expected_binary.get("platform") != actual_binary.get("platform"):
        raise BaselineError(
            f"platform 不一致: {expected_binary.get('platform')} != {actual_binary.get('platform')}"
        )
    guard_ratio = float(expected_binary.get("size_guard_ratio", BINARY_SIZE_GUARD_RATIO))
    expected_size = int(expected_binary.get("size_bytes", 0))
    actual_size = int(actual_binary.get("size_bytes", 0))
    if expected_size > 0 and actual_size > int(expected_size * guard_ratio):
        raise BaselineError(f"binary size 超過 guard: {actual_size} > {int(expected_size * guard_ratio)}")

    expected_cmp = json.loads(canonical_json(expected))
    actual_cmp = json.loads(canonical_json(actual))
    if expected_size > 0:
        actual_cmp["binary"]["size_bytes"] = expected_cmp["binary"]["size_bytes"]

    if expected_cmp != actual_cmp:
        raise BaselineError(
            "self-index baseline 不一致\n--- expected ---\n{}\n--- actual ---\n{}".format(
                canonical_json(expected_cmp), canonical_json(actual_cmp)
            )
        )


def main() -> int:
    parser = argparse.ArgumentParser(description="驗證 repository self-index baseline")
    parser.add_argument("binary", help="要驗證的 codebase-memory-mcp binary")
    parser.add_argument("--repo", default=str(ROOT), help="要 self-index 的 repository root")
    parser.add_argument("--update", action="store_true", help="重寫 self-index baseline fixture")
    args = parser.parse_args()

    binary = Path(args.binary).resolve()
    repo = Path(args.repo).resolve()
    if not binary.exists():
        raise BaselineError(f"找不到 binary: {binary}")
    if not repo.is_dir():
        raise BaselineError(f"找不到 repo: {repo}")

    with tempfile.TemporaryDirectory(prefix="cbm-rust-self-index-") as tmp_s:
        tmp = Path(tmp_s)
        env = make_env(tmp)
        with masked_self_fixture(repo):
            baseline = build_baseline(binary, repo, env)
        leak_paths: list[Path | str] = [
            tmp,
            binary,
            repo,
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
        print(f"更新 self-index baseline: {GOLDEN_PATH}")
        return 0

    if not GOLDEN_PATH.exists():
        raise BaselineError(f"缺少 self-index baseline: {GOLDEN_PATH}，請先執行 --update")
    expected = load_json(GOLDEN_PATH)
    if not isinstance(expected, dict):
        raise BaselineError("self-index baseline fixture 必須是 JSON object")
    compare_baseline(expected, baseline)
    print("Rust self-index baseline passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (BaselineError, subprocess.TimeoutExpired) as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
