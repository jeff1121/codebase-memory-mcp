#!/usr/bin/env python3
"""產生並驗證 Rust 重構用的公開行為 golden fixtures。"""

from __future__ import annotations

import argparse
import json
import os
import platform
import queue
import resource
import shutil
import subprocess
import sys
import tempfile
import threading
import time
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
GOLDEN_DIR = ROOT / "tests" / "golden" / "rust-refactor"
PROJECT = "rust_refactor_golden"
ARTIFACT_PROJECT = "rust_refactor_golden_artifact"
TIMEOUT = 120


class FixtureError(RuntimeError):
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


def assert_no_path_leaks(value: Any, paths: list[Path | str]) -> None:
    blob = canonical_json(value)
    leaks: list[str] = []
    for item in paths:
        text = normalize_path(Path(item)) if not isinstance(item, str) else item.replace("\\", "/")
        if len(text) > 1 and text in blob:
            leaks.append(text)
    if leaks:
        joined = "\n".join(sorted(set(leaks)))
        raise FixtureError(f"golden fixture 含有本機或暫存路徑:\n{joined}")


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
            "GIT_AUTHOR_DATE": "2024-01-01T00:00:00Z",
            "GIT_COMMITTER_DATE": "2024-01-01T00:00:00Z",
            "LC_ALL": "C",
            "TZ": "UTC",
        }
    )
    return env


def child_peak_rss_kb() -> int:
    value = int(resource.getrusage(resource.RUSAGE_CHILDREN).ru_maxrss)
    if sys.platform == "darwin":
        return value // 1024
    return value


def run_cmd(
    cmd: list[str],
    env: dict[str, str],
    *,
    timeout: int = TIMEOUT,
    stdin: str | None = None,
) -> tuple[str, str, float]:
    start = time.perf_counter()
    proc = subprocess.run(
        cmd,
        input=stdin,
        text=True,
        capture_output=True,
        env=env,
        timeout=timeout,
        cwd=ROOT,
    )
    elapsed_ms = (time.perf_counter() - start) * 1000.0
    if proc.returncode != 0:
        raise FixtureError(
            "命令失敗: {}\nrc={}\nstdout={}\nstderr={}".format(
                " ".join(cmd), proc.returncode, proc.stdout[-2000:], proc.stderr[-2000:]
            )
        )
    return proc.stdout, proc.stderr, elapsed_ms


def run_cmd_status(
    cmd: list[str],
    env: dict[str, str],
    *,
    timeout: int = TIMEOUT,
    stdin: str | None = None,
) -> tuple[int, str, str, float]:
    start = time.perf_counter()
    proc = subprocess.run(
        cmd,
        input=stdin,
        text=True,
        capture_output=True,
        env=env,
        timeout=timeout,
        cwd=ROOT,
    )
    elapsed_ms = (time.perf_counter() - start) * 1000.0
    return proc.returncode, proc.stdout, proc.stderr, elapsed_ms


def parse_tool_result(raw: str) -> Any:
    data = json.loads(raw)
    if isinstance(data, dict) and "structuredContent" in data:
        return data["structuredContent"]
    if isinstance(data, dict) and "result" in data:
        return parse_tool_value(data["result"])
    return parse_tool_value(data)


def parse_tool_value(value: Any) -> Any:
    if isinstance(value, dict) and "structuredContent" in value:
        return value["structuredContent"]
    if isinstance(value, dict) and isinstance(value.get("content"), list) and value["content"]:
        text = value["content"][0].get("text") if isinstance(value["content"][0], dict) else None
        if isinstance(text, str):
            try:
                return json.loads(text)
            except json.JSONDecodeError:
                return text
    return value


def run_cli_tool(binary: Path, env: dict[str, str], tool: str, args: dict[str, Any]) -> tuple[Any, float]:
    raw, _err, elapsed_ms = run_cmd(
        [str(binary), "cli", "--json", tool, json.dumps(args, separators=(",", ":"))],
        env,
    )
    return parse_tool_result(raw), elapsed_ms


def run_cli_tool_capture(
    binary: Path, env: dict[str, str], tool: str, args: dict[str, Any]
) -> tuple[Any, str, float]:
    raw, err, elapsed_ms = run_cmd(
        [str(binary), "cli", "--json", tool, json.dumps(args, separators=(",", ":"))],
        env,
    )
    return parse_tool_result(raw), err, elapsed_ms


def summarize_index_status(value: Any) -> dict[str, Any]:
    return {
        "tool": "index_status",
        "project": value.get("project") if isinstance(value, dict) else None,
        "status": value.get("status") if isinstance(value, dict) else None,
    }


def stable_cli_stderr(stderr: str) -> str:
    lines = [line for line in stderr.splitlines() if not line.startswith("level=")]
    return "\n".join(lines).strip()


def build_cli_invocation_forms(binary: Path, env: dict[str, str], tmp: Path) -> dict[str, Any]:
    status_args = {"project": PROJECT}
    status_json = json.dumps(status_args, separators=(",", ":"))

    raw_stdout, _raw_err, _raw_ms = run_cmd(
        [str(binary), "cli", "--json", "index_status", status_json], env
    )
    raw_value = parse_tool_result(raw_stdout)

    stdin_stdout, _stdin_err, _stdin_ms = run_cmd(
        [str(binary), "cli", "--json", "index_status"], env, stdin=status_json
    )
    stdin_value = parse_tool_result(stdin_stdout)

    flag_stdout, _flag_err, _flag_ms = run_cmd(
        [str(binary), "cli", "--json", "index_status", "--project", PROJECT], env
    )
    flag_value = parse_tool_result(flag_stdout)

    args_path = tmp / "query-args.json"
    write_json(
        args_path,
        {"project": PROJECT, "query": "MATCH (n) RETURN n.label", "max_rows": 2},
    )
    args_file_stdout, _args_file_err, _args_file_ms = run_cmd(
        [str(binary), "cli", "--json", "query_graph", "--args-file", str(args_path)], env
    )
    args_file_value = parse_tool_result(args_file_stdout)

    envelope = json.loads(raw_stdout)
    content = envelope.get("content", []) if isinstance(envelope, dict) else []
    content_type = None
    if content and isinstance(content[0], dict):
        content_type = content[0].get("type")

    rc_ok, plain_out, plain_err, _plain_ms = run_cmd_status(
        [str(binary), "cli", "index_status", status_json], env
    )
    plain_value: Any = None
    plain_json = False
    try:
        plain_value = json.loads(plain_out)
        plain_json = True
    except json.JSONDecodeError:
        plain_value = None

    rc_err, err_out, err_stderr, _err_ms = run_cmd_status(
        [str(binary), "cli", "query_graph", json.dumps({"project": PROJECT}, separators=(",", ":"))],
        env,
    )
    stable_plain_err = stable_cli_stderr(plain_err)
    stable_error_stderr = stable_cli_stderr(err_stderr)

    return {
        "raw_json_args": summarize_index_status(raw_value),
        "stdin_json": summarize_index_status(stdin_value),
        "args_file": {
            "tool": "query_graph",
            "columns": args_file_value.get("columns") if isinstance(args_file_value, dict) else [],
            "row_count": len(query_rows(args_file_value)),
        },
        "flag_form": summarize_index_status(flag_value),
        "json_envelope": {
            "isError": envelope.get("isError") if isinstance(envelope, dict) else None,
            "top_level_keys": sorted(envelope.keys()) if isinstance(envelope, dict) else [],
            "content_type": content_type,
        },
        "plain_success": {
            "exit_code": rc_ok,
            "stdout_json": plain_json,
            "stderr_empty": stable_plain_err == "",
            "project": plain_value.get("project") if isinstance(plain_value, dict) else None,
            "status": plain_value.get("status") if isinstance(plain_value, dict) else None,
        },
        "plain_error": {
            "exit_code": rc_err,
            "stdout_empty": err_out == "",
            "stderr": stable_error_stderr,
        },
    }


def create_repo(root: Path) -> Path:
    repo = root / "repo"
    (repo / "src" / "pkg").mkdir(parents=True)
    (repo / "src" / "main.py").write_text(
        "\n".join(
            [
                "from pkg import helper",
                "",
                "def main():",
                "    result = helper.compute(42)",
                "    print(result)",
                "",
                "class Config:",
                "    DEBUG = True",
                "",
            ]
        ),
        encoding="utf-8",
    )
    (repo / "src" / "pkg" / "__init__.py").write_text(
        "from .helper import compute\n", encoding="utf-8"
    )
    (repo / "src" / "pkg" / "helper.py").write_text(
        "\n".join(
            [
                "def compute(x):",
                "    return x * 2",
                "",
                "def validate(data):",
                "    if not data:",
                "        raise ValueError('empty')",
                "    return True",
                "",
            ]
        ),
        encoding="utf-8",
    )
    (repo / "src" / "server.go").write_text(
        "\n".join(
            [
                "package main",
                "",
                'import "fmt"',
                "",
                "func StartServer(port int) {",
                '    fmt.Printf("listening on :%d\\n", port)',
                "}",
                "",
                "func HandleRequest(path string) string {",
                '    return "ok: " + path',
                "}",
                "",
            ]
        ),
        encoding="utf-8",
    )
    (repo / "service.yaml").write_text(
        "\n".join(
            [
                "service:",
                "  name: golden-api",
                "  port: 8080",
                "",
            ]
        ),
        encoding="utf-8",
    )
    subprocess.run(["git", "-C", str(repo), "init", "-q"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    subprocess.run(["git", "-C", str(repo), "add", "-A"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    subprocess.run(
        [
            "git",
            "-C",
            str(repo),
            "-c",
            "user.email=golden@example.invalid",
            "-c",
            "user.name=golden",
            "commit",
            "-q",
            "-m",
            "init",
        ],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    return repo


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


def query_rows(value: Any) -> list[Any]:
    if isinstance(value, list):
        return value
    if isinstance(value, dict):
        for key in ("rows", "results", "data"):
            rows = value.get(key)
            if isinstance(rows, list):
                return rows
    return []


def histogram(values: list[Any]) -> dict[str, int]:
    hist: dict[str, int] = {}
    for value in values:
        key = str(value)
        hist[key] = hist.get(key, 0) + 1
    return dict(sorted(hist.items()))


def summarize_schema(schema: Any) -> dict[str, Any]:
    text = canonical_json(schema)
    labels = sorted(set(part for part in ("File", "Folder", "Function", "Class", "Module") if part in text))
    edges = sorted(
        set(
            part
            for part in (
                "CALLS",
                "CONTAINS_FILE",
                "DEFINES",
                "IMPORTS",
                "USAGE",
                "ROUTE",
            )
            if part in text
        )
    )
    return {
        "contains_labels": labels,
        "contains_edge_types": edges,
        "top_level_keys": sorted(schema.keys()) if isinstance(schema, dict) else [],
    }


def build_graph_summary(binary: Path, env: dict[str, str], project: str) -> dict[str, Any]:
    node_labels, _node_ms = run_cli_tool(
        binary,
        env,
        "query_graph",
        {"project": project, "query": "MATCH (n) RETURN n.label"},
    )
    edge_types, _edge_ms = run_cli_tool(
        binary,
        env,
        "query_graph",
        {"project": project, "query": "MATCH ()-[r]->() RETURN type(r)"},
    )
    return {
        "node_label_histogram": histogram(scalar_values(query_rows(node_labels))),
        "edge_type_histogram": histogram(scalar_values(query_rows(edge_types))),
    }


def summarize_artifact_metadata(meta: dict[str, Any], index_result: Any) -> dict[str, Any]:
    index_nodes = index_result.get("nodes") if isinstance(index_result, dict) else None
    index_edges = index_result.get("edges") if isinstance(index_result, dict) else None
    indexed_at = meta.get("indexed_at")
    stable_keys = sorted(key for key in meta.keys() if key != "indexed_at")
    return {
        "stable_key_set": stable_keys,
        "contains_timestamp_key": "indexed_at" in meta,
        "schema_version": meta.get("schema_version"),
        "project": meta.get("project"),
        "nodes": meta.get("nodes"),
        "edges": meta.get("edges"),
        "counts_match_index_response": meta.get("nodes") == index_nodes
        and meta.get("edges") == index_edges,
        "compression_level": meta.get("compression_level"),
        "original_size_gt_0": isinstance(meta.get("original_size"), int)
        and meta.get("original_size", 0) > 0,
        "compressed_size_gt_0": isinstance(meta.get("compressed_size"), int)
        and meta.get("compressed_size", 0) > 0,
        "compressed_not_larger_than_original": isinstance(meta.get("compressed_size"), int)
        and isinstance(meta.get("original_size"), int)
        and meta.get("compressed_size", 0) <= meta.get("original_size", 0),
        "commit_sha_length": len(meta.get("commit", "")) if isinstance(meta.get("commit"), str) else 0,
        "timestamp_shape": isinstance(indexed_at, str)
        and "T" in indexed_at
        and indexed_at.endswith("Z"),
    }


def build_artifact_golden(binary: Path, env: dict[str, str], repo: Path) -> dict[str, Any]:
    index_result, _index_ms = run_cli_tool(
        binary,
        env,
        "index_repository",
        {
            "repo_path": normalize_path(repo),
            "mode": "fast",
            "name": ARTIFACT_PROJECT,
            "persistence": True,
        },
    )
    export_graph = build_graph_summary(binary, env, ARTIFACT_PROJECT)
    artifact_dir = repo / ".codebase-memory"
    artifact_path = artifact_dir / "graph.db.zst"
    meta_path = artifact_dir / "artifact.json"
    gitattributes_path = artifact_dir / ".gitattributes"
    if not artifact_path.exists() or artifact_path.stat().st_size <= 0:
        raise FixtureError("artifact fixture 未產生 graph.db.zst")
    if not meta_path.exists():
        raise FixtureError("artifact fixture 未產生 artifact.json")
    if not gitattributes_path.exists():
        raise FixtureError("artifact fixture 未產生 .gitattributes")

    meta = load_json(meta_path)
    gitattributes = gitattributes_path.read_text(encoding="utf-8")

    bootstrap_env = make_env(repo.parent / "artifact-bootstrap-env")
    bootstrap_result, bootstrap_stderr, _bootstrap_ms = run_cli_tool_capture(
        binary,
        bootstrap_env,
        "index_repository",
        {
            "repo_path": normalize_path(repo),
            "mode": "fast",
            "name": ARTIFACT_PROJECT,
        },
    )
    bootstrap_graph = build_graph_summary(binary, bootstrap_env, ARTIFACT_PROJECT)
    if bootstrap_graph != export_graph:
        raise FixtureError("artifact bootstrap 匯入後 graph 與 export baseline 不一致")

    mismatch_meta = dict(meta)
    mismatch_meta["schema_version"] = 999
    write_json(meta_path, mismatch_meta)
    mismatch_env = make_env(repo.parent / "artifact-mismatch-env")
    mismatch_result, mismatch_stderr, _mismatch_ms = run_cli_tool_capture(
        binary,
        mismatch_env,
        "index_repository",
        {
            "repo_path": normalize_path(repo),
            "mode": "fast",
            "name": ARTIFACT_PROJECT,
        },
    )
    write_json(meta_path, meta)
    if "index.artifact_bootstrap" in mismatch_stderr:
        raise FixtureError("schema mismatch artifact 不應觸發 bootstrap 匯入")

    return {
        "index_repository": {
            "project": index_result.get("project") if isinstance(index_result, dict) else None,
            "status": index_result.get("status") if isinstance(index_result, dict) else None,
            "artifact_present": index_result.get("artifact_present")
            if isinstance(index_result, dict)
            else None,
            "has_artifact_hint": "artifact_hint" in index_result
            if isinstance(index_result, dict)
            else False,
        },
        "files": {
            "artifact_exists": artifact_path.exists(),
            "artifact_size_gt_0": artifact_path.stat().st_size > 0,
            "metadata_exists": meta_path.exists(),
            "gitattributes_exists": gitattributes_path.exists(),
        },
        "metadata": summarize_artifact_metadata(meta, index_result if isinstance(index_result, dict) else {}),
        "gitattributes": {
            "contains_graph_artifact_rule": "graph.db.zst merge=ours binary" in gitattributes,
            "contains_autogenerated_header": "Auto-generated by codebase-memory-mcp" in gitattributes,
        },
        "graph_summary": export_graph,
        "bootstrap_import": {
            "project": bootstrap_result.get("project")
            if isinstance(bootstrap_result, dict)
            else None,
            "status": bootstrap_result.get("status")
            if isinstance(bootstrap_result, dict)
            else None,
            "nodes": bootstrap_result.get("nodes") if isinstance(bootstrap_result, dict) else None,
            "edges": bootstrap_result.get("edges") if isinstance(bootstrap_result, dict) else None,
            "artifact_present": bootstrap_result.get("artifact_present")
            if isinstance(bootstrap_result, dict)
            else None,
            "has_bootstrap_log": "index.artifact_bootstrap" in bootstrap_stderr,
            "graph_matches_export": bootstrap_graph == export_graph,
        },
        "schema_mismatch": {
            "status": mismatch_result.get("status") if isinstance(mismatch_result, dict) else None,
            "artifact_present": mismatch_result.get("artifact_present")
            if isinstance(mismatch_result, dict)
            else None,
            "skipped_bootstrap": "index.artifact_bootstrap" not in mismatch_stderr,
        },
    }


def build_cli_and_graph(binary: Path, env: dict[str, str], repo: Path) -> tuple[dict[str, Any], dict[str, Any], dict[str, Any]]:
    version_out, _err, _version_ms = run_cmd([str(binary), "--version"], env, timeout=30)
    version_parts = version_out.strip().split()
    index_result, index_ms = run_cli_tool(
        binary,
        env,
        "index_repository",
        {"repo_path": normalize_path(repo), "mode": "fast", "name": PROJECT},
    )
    status_result, _status_ms = run_cli_tool(binary, env, "index_status", {"project": PROJECT})
    list_result, _list_ms = run_cli_tool(binary, env, "list_projects", {})
    schema_result, _schema_ms = run_cli_tool(binary, env, "get_graph_schema", {"project": PROJECT})
    node_labels, _node_ms = run_cli_tool(
        binary,
        env,
        "query_graph",
        {"project": PROJECT, "query": "MATCH (n) RETURN n.label"},
    )
    edge_types, query_ms = run_cli_tool(
        binary,
        env,
        "query_graph",
        {"project": PROJECT, "query": "MATCH ()-[r]->() RETURN type(r)"},
    )

    node_values = scalar_values(query_rows(node_labels))
    edge_values = scalar_values(query_rows(edge_types))
    project_names = sorted(
        str(v)
        for v in scalar_values(list_result)
        if isinstance(v, str) and v == PROJECT
    )

    cli_golden = {
        "version": {
            "program": version_parts[0] if version_parts else "",
            "version_kind": "dev" if "dev" in version_parts else "release",
        },
        "index_repository": {
            "project": index_result.get("project") if isinstance(index_result, dict) else None,
            "status": index_result.get("status") if isinstance(index_result, dict) else None,
            "nodes": index_result.get("nodes") if isinstance(index_result, dict) else None,
            "edges": index_result.get("edges") if isinstance(index_result, dict) else None,
            "artifact_present": index_result.get("artifact_present")
            if isinstance(index_result, dict)
            else None,
            "adr_present": index_result.get("adr_present") if isinstance(index_result, dict) else None,
        },
        "index_status": {
            "status": status_result.get("status") if isinstance(status_result, dict) else None,
            "project": status_result.get("project") if isinstance(status_result, dict) else PROJECT,
        },
        "list_projects": {"contains": project_names},
        "cli_invocation_forms": build_cli_invocation_forms(binary, env, repo.parent),
    }

    graph_golden = {
        "project": PROJECT,
        "node_label_histogram": histogram(node_values),
        "edge_type_histogram": histogram(edge_values),
        "schema_summary": summarize_schema(schema_result),
    }

    perf = {
        "platform": f"{sys.platform}-{platform.machine()}",
        "binary_size_bytes": binary.stat().st_size,
        "fixture": PROJECT,
        "index_wall_ms": round(index_ms, 3),
        "query_edge_types_wall_ms": round(query_ms, 3),
        "child_peak_rss_kb": child_peak_rss_kb(),
        "thresholds": {
            "binary_size_ratio": 1.20,
            "wall_time_ratio": 2.00,
            "wall_time_floor_ms": 500.0,
        },
    }
    return cli_golden, graph_golden, perf


def read_response_line(lines: "queue.Queue[str]", timeout: int) -> dict[str, Any]:
    try:
        line = lines.get(timeout=timeout)
    except queue.Empty as exc:
        raise FixtureError("MCP response timeout") from exc
    return json.loads(line)


def summarize_jsonrpc_error(resp: dict[str, Any]) -> dict[str, Any]:
    error = resp.get("error", {}) if isinstance(resp, dict) else {}
    return {
        "id": resp.get("id") if isinstance(resp, dict) else None,
        "jsonrpc": resp.get("jsonrpc") if isinstance(resp, dict) else None,
        "error": {
            "code": error.get("code") if isinstance(error, dict) else None,
            "message": error.get("message") if isinstance(error, dict) else None,
        },
    }


def summarize_tool_call_response(resp: dict[str, Any]) -> dict[str, Any]:
    result = resp.get("result", {}) if isinstance(resp, dict) else {}
    content = result.get("content", []) if isinstance(result, dict) else []
    content_types: set[str] = set()
    has_unknown_tool_text = False
    if isinstance(content, list):
        for item in content:
            if isinstance(item, dict):
                item_type = item.get("type")
                if isinstance(item_type, str):
                    content_types.add(item_type)
                if isinstance(item.get("text"), str) and "unknown tool: nonexistent_tool" in item["text"]:
                    has_unknown_tool_text = True
    return {
        "id": resp.get("id") if isinstance(resp, dict) else None,
        "jsonrpc": resp.get("jsonrpc") if isinstance(resp, dict) else None,
        "isError": result.get("isError") if isinstance(result, dict) else None,
        "content_type_set": sorted(content_types),
        "has_unknown_tool_text": has_unknown_tool_text,
    }


def mcp_tool_payload(resp: dict[str, Any]) -> Any:
    if not isinstance(resp, dict):
        return None
    result = resp.get("result")
    return parse_tool_value(result)


def mcp_tool_text(resp: dict[str, Any]) -> str:
    result = resp.get("result", {}) if isinstance(resp, dict) else {}
    content = result.get("content", []) if isinstance(result, dict) else []
    if isinstance(content, list) and content and isinstance(content[0], dict):
        text = content[0].get("text")
        if isinstance(text, str):
            return text
    return ""


def mcp_tool_is_error(resp: dict[str, Any]) -> bool | None:
    result = resp.get("result", {}) if isinstance(resp, dict) else {}
    return result.get("isError") if isinstance(result, dict) else None


def summarize_tool_schema(tool: dict[str, Any]) -> dict[str, Any]:
    schema = tool.get("inputSchema") if isinstance(tool, dict) else None
    properties = schema.get("properties") if isinstance(schema, dict) else None
    required = schema.get("required") if isinstance(schema, dict) else None
    return {
        "name": tool.get("name") if isinstance(tool.get("name"), str) else None,
        "inputSchema": {
            "type": schema.get("type") if isinstance(schema, dict) else None,
            "property_keys": sorted(properties.keys()) if isinstance(properties, dict) else [],
            "required": required if isinstance(required, list) else [],
        },
    }


def summarize_index_status_payload(value: Any) -> dict[str, Any]:
    return {
        "project": value.get("project") if isinstance(value, dict) else None,
        "status": value.get("status") if isinstance(value, dict) else None,
        "nodes": value.get("nodes") if isinstance(value, dict) else None,
        "edges": value.get("edges") if isinstance(value, dict) else None,
    }


def summarize_indexed_repo_query(resp: dict[str, Any]) -> dict[str, Any]:
    value = mcp_tool_payload(resp)
    rows = query_rows(value)
    return {
        "tool": "query_graph",
        "isError": mcp_tool_is_error(resp),
        "columns": value.get("columns") if isinstance(value, dict) else [],
        "row_count": len(rows),
        "total": value.get("total") if isinstance(value, dict) else None,
        "rows": rows[:5],
    }


def summarize_trace_alias(canonical: dict[str, Any], alias: dict[str, Any]) -> dict[str, Any]:
    canonical_text = mcp_tool_text(canonical)
    alias_text = mcp_tool_text(alias)
    return {
        "canonical_tool": "trace_path",
        "alias_tool": "trace_call_path",
        "advertised_in_tools_list": False,
        "canonical_isError": mcp_tool_is_error(canonical),
        "alias_isError": mcp_tool_is_error(alias),
        "same_text": canonical_text == alias_text,
        "contains_function_not_found": "function not found" in alias_text,
    }


def summarize_project_arg_aliases(alias_payloads: dict[str, Any]) -> dict[str, Any]:
    return {
        key: summarize_index_status_payload(value)
        for key, value in sorted(alias_payloads.items())
    }


def read_content_length_response(raw: bytes) -> dict[str, Any]:
    sep = raw.find(b"\r\n\r\n")
    if sep < 0:
        raise FixtureError("Content-Length response missing header separator")
    header = raw[:sep].decode("ascii", errors="strict")
    content_length: int | None = None
    for line in header.split("\r\n"):
        if line.lower().startswith("content-length:"):
            content_length = int(line.split(":", 1)[1].strip())
            break
    if content_length is None:
        raise FixtureError("Content-Length response missing length header")
    body = raw[sep + 4 : sep + 4 + content_length]
    if len(body) != content_length:
        raise FixtureError("Content-Length response body shorter than declared length")
    resp = json.loads(body.decode("utf-8"))
    result = resp.get("result", {}) if isinstance(resp, dict) else {}
    return {
        "header_content_length": content_length,
        "body_length": len(body),
        "content_length_matches": content_length == len(body),
        "response": {
            "id": resp.get("id") if isinstance(resp, dict) else None,
            "jsonrpc": resp.get("jsonrpc") if isinstance(resp, dict) else None,
            "result_keys": sorted(result.keys()) if isinstance(result, dict) else None,
        },
    }


def build_content_length_ping(binary: Path, env: dict[str, str]) -> dict[str, Any]:
    request = {"jsonrpc": "2.0", "id": "frame-1", "method": "ping"}
    body = json.dumps(request, separators=(",", ":")).encode("utf-8")
    frame = b"Content-Length: " + str(len(body)).encode("ascii") + b"\r\n\r\n" + body
    proc = subprocess.run(
        [str(binary)],
        cwd=ROOT,
        env=env,
        input=frame,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        timeout=20,
    )
    if proc.returncode != 0:
        raise FixtureError(f"Content-Length MCP subprocess failed rc={proc.returncode}")
    summary = read_content_length_response(proc.stdout)
    summary["request"] = {"id": request["id"], "method": request["method"]}
    return summary


def build_mcp(binary: Path, env: dict[str, str]) -> dict[str, Any]:
    proc = subprocess.Popen(
        [str(binary)],
        cwd=ROOT,
        env=env,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True,
        bufsize=1,
    )
    assert proc.stdin is not None
    assert proc.stdout is not None
    lines: "queue.Queue[str]" = queue.Queue()

    def reader() -> None:
        for line in proc.stdout:
            if line.strip():
                lines.put(line)

    thread = threading.Thread(target=reader, daemon=True)
    thread.start()

    def send(obj: dict[str, Any]) -> None:
        proc.stdin.write(json.dumps(obj, separators=(",", ":")) + "\n")
        proc.stdin.flush()

    try:
        send(
            {
                "jsonrpc": "2.0",
                "id": 1,
                "method": "initialize",
                "params": {
                    "protocolVersion": "2025-06-18",
                    "capabilities": {},
                    "clientInfo": {"name": "rust-refactor-golden", "version": "1"},
                },
            }
        )
        init = read_response_line(lines, 15)
        send({"jsonrpc": "2.0", "method": "notifications/initialized"})

        names: list[str] = []
        schema_transcript: list[dict[str, Any]] = []
        pages: list[dict[str, Any]] = []
        response_ids: list[Any] = [init.get("id") if isinstance(init, dict) else None]
        cursor: str | None = None
        request_id = 2
        for _ in range(10):
            params: dict[str, Any] = {}
            if cursor:
                params["cursor"] = cursor
            page_id = request_id
            requested_cursor = cursor
            send({"jsonrpc": "2.0", "id": page_id, "method": "tools/list", "params": params})
            request_id += 1
            page = read_response_line(lines, 15)
            if not isinstance(page, dict) or page.get("id") != page_id:
                raise FixtureError(
                    f"unexpected MCP response id for tools/list: {page.get('id') if isinstance(page, dict) else None}"
                )
            response_ids.append(page.get("id"))
            result = page.get("result", {}) if isinstance(page, dict) else {}
            tools = result.get("tools", []) if isinstance(result, dict) else []
            page_names: list[str] = []
            page_key_sets: list[list[str]] = []
            for tool in tools:
                if isinstance(tool, dict) and isinstance(tool.get("name"), str):
                    names.append(tool["name"])
                    page_names.append(tool["name"])
                    page_key_sets.append(sorted(tool.keys()))
                    schema_transcript.append(summarize_tool_schema(tool))
            next_cursor = result.get("nextCursor") if isinstance(result, dict) else None
            pages.append(
                {
                    "id": page_id,
                    "cursor": requested_cursor,
                    "tool_names": page_names,
                    "tool_key_sets": page_key_sets,
                    "nextCursor": next_cursor if isinstance(next_cursor, str) else None,
                }
            )
            if isinstance(next_cursor, str) and next_cursor:
                cursor = next_cursor
                continue
            break
        else:
            raise FixtureError("tools/list pagination did not terminate")

        def send_tool_call(name: str, arguments: dict[str, Any]) -> dict[str, Any]:
            nonlocal request_id
            call_id = request_id
            request_id += 1
            send(
                {
                    "jsonrpc": "2.0",
                    "id": call_id,
                    "method": "tools/call",
                    "params": {"name": name, "arguments": arguments},
                }
            )
            resp = read_response_line(lines, 15)
            if not isinstance(resp, dict) or resp.get("id") != call_id:
                raise FixtureError(
                    f"unexpected MCP response id for tools/call: "
                    f"{resp.get('id') if isinstance(resp, dict) else None}"
                )
            return resp

        indexed_query = send_tool_call(
            "query_graph",
            {
                "project": PROJECT,
                "query": "MATCH (n) RETURN n.label ORDER BY n.label LIMIT 5",
            },
        )

        project_arg_payloads: dict[str, Any] = {}
        for key in ("project", "projectName", "project_id", "project_name"):
            project_arg_payloads[key] = mcp_tool_payload(
                send_tool_call("index_status", {key: PROJECT})
            )
        name_not_project_alias = send_tool_call("index_status", {"name": PROJECT})

        trace_args = {"project": PROJECT, "function_name": "does_not_exist_golden", "depth": 1}
        trace_canonical = send_tool_call("trace_path", trace_args)
        trace_alias = send_tool_call("trace_call_path", trace_args)

        send({"jsonrpc": "2.0", "id": 20, "method": "unknown/method"})
        unknown_method = read_response_line(lines, 15)

        send({"jsonrpc": "2.0", "id": 21, "method": "tools/call",
              "params": {"name": "nonexistent_tool", "arguments": {}}})
        unknown_tool = read_response_line(lines, 15)

        proc.stdin.write("not json\n")
        proc.stdin.flush()
        parse_error = read_response_line(lines, 15)
    finally:
        try:
            proc.stdin.close()
        except Exception:
            pass
        try:
            proc.wait(timeout=10)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait(timeout=5)

    result = init.get("result", {}) if isinstance(init, dict) else {}
    server = result.get("serverInfo", {}) if isinstance(result, dict) else {}
    return {
        "initialize": {
            "id": init.get("id") if isinstance(init, dict) else None,
            "jsonrpc": init.get("jsonrpc") if isinstance(init, dict) else None,
            "protocolVersion": result.get("protocolVersion") if isinstance(result, dict) else None,
            "serverName": server.get("name") if isinstance(server, dict) else None,
        },
        "notifications_initialized": {"expect_response": False},
        "tools_list": {
            "count": len(names),
            "names": sorted(names),
            "response_ids": response_ids,
            "pages": pages,
            "schema_transcript": sorted(
                schema_transcript,
                key=lambda item: item["name"] if isinstance(item.get("name"), str) else "",
            ),
        },
        "alias_transcript": {
            "project_argument_compatibility": summarize_project_arg_aliases(project_arg_payloads),
            "name_is_not_project_alias": {
                "isError": mcp_tool_is_error(name_not_project_alias),
                "contains_missing_project_text": "missing required argument: project"
                in mcp_tool_text(name_not_project_alias),
            },
            "tool_name_alias": summarize_trace_alias(trace_canonical, trace_alias),
        },
        "indexed_repo_tool_call": summarize_indexed_repo_query(indexed_query),
        "errors": {
            "unknown_method": summarize_jsonrpc_error(unknown_method),
            "parse_error": summarize_jsonrpc_error(parse_error),
        },
        "tools_call": {
            "unknown_tool": summarize_tool_call_response(unknown_tool),
        },
        "content_length_ping": build_content_length_ping(binary, env),
    }


def compare_exact(name: str, expected: Any, actual: Any) -> None:
    if expected != actual:
        exp = canonical_json(expected)
        act = canonical_json(actual)
        raise FixtureError(f"{name} golden 不一致\n--- expected ---\n{exp}\n--- actual ---\n{act}")


def compare_perf(expected: dict[str, Any], actual: dict[str, Any]) -> None:
    if expected.get("platform") != actual.get("platform"):
        print(
            f"SKIP: performance strict compare platform mismatch "
            f"({expected.get('platform')} != {actual.get('platform')})"
        )
        return
    thresholds = expected.get("thresholds", {})
    size_ratio = float(thresholds.get("binary_size_ratio", 1.20))
    wall_ratio = float(thresholds.get("wall_time_ratio", 2.00))
    wall_floor = float(thresholds.get("wall_time_floor_ms", 500.0))
    if actual["binary_size_bytes"] > expected["binary_size_bytes"] * size_ratio:
        raise FixtureError("binary size 超過 performance baseline 門檻")
    for key in ("index_wall_ms", "query_edge_types_wall_ms"):
        limit = max(float(expected[key]) * wall_ratio, float(expected[key]) + wall_floor)
        if float(actual[key]) > limit:
            raise FixtureError(f"{key} 超過 performance baseline 門檻: {actual[key]} > {limit}")


def main() -> int:
    parser = argparse.ArgumentParser(description="驗證 Rust 重構 golden fixtures")
    parser.add_argument("binary", help="要驗證的 codebase-memory-mcp binary")
    parser.add_argument("--update", action="store_true", help="重寫 golden fixtures")
    args = parser.parse_args()

    binary = Path(args.binary).resolve()
    if not binary.exists():
        raise FixtureError(f"找不到 binary: {binary}")

    with tempfile.TemporaryDirectory(prefix="cbm-rust-baseline-") as tmp_s:
        tmp = Path(tmp_s)
        env = make_env(tmp)
        repo = create_repo(tmp)
        cli_golden, graph_golden, perf = build_cli_and_graph(binary, env, repo)
        artifact_golden = build_artifact_golden(binary, env, repo)
        mcp_golden = build_mcp(binary, env)
        leak_paths: list[Path | str] = [
            tmp,
            repo,
            binary,
            *[value for key, value in env.items() if key in {
                "HOME",
                "USERPROFILE",
                "XDG_CONFIG_HOME",
                "APPDATA",
                "LOCALAPPDATA",
                "CBM_CACHE_DIR",
            }],
        ]

    outputs = {
        "cli-golden.json": cli_golden,
        "mcp-golden.json": mcp_golden,
        "graph-golden.json": graph_golden,
        "artifact-golden.json": artifact_golden,
        "performance-baseline.json": perf,
    }
    assert_no_path_leaks(outputs, leak_paths)

    if args.update:
        for filename, value in outputs.items():
            write_json(GOLDEN_DIR / filename, value)
        print(f"更新 golden fixtures: {GOLDEN_DIR}")
        return 0

    for filename, value in outputs.items():
        path = GOLDEN_DIR / filename
        if not path.exists():
            raise FixtureError(f"缺少 golden fixture: {path}，請先執行 --update")
        expected = load_json(path)
        if filename == "performance-baseline.json":
            compare_perf(expected, value)
        else:
            compare_exact(filename, expected, value)

    print("Rust baseline golden fixtures passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except FixtureError as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
