#!/usr/bin/env python3
"""驗證代表性語言 graph 在 C path 與 Rust pipeline opt-in path 下等價。"""

from __future__ import annotations

import argparse
import json
import os
import shutil
import sqlite3
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
GOLDEN_PATH = ROOT / "tests" / "golden" / "rust-refactor" / "language-graph-parity.json"
PROJECT = "rust_refactor_language_parity"
TIMEOUT = 180


class ParityError(RuntimeError):
    pass


def canonical_json(value: Any) -> str:
    return json.dumps(value, ensure_ascii=False, indent=2, sort_keys=True) + "\n"


def load_json(path: Path) -> Any:
    if not path.exists():
        raise ParityError(f"golden 不存在，請先執行 --update 產生: {path.relative_to(ROOT)}")
    with path.open("r", encoding="utf-8") as f:
        return json.load(f)


def write_json(path: Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(canonical_json(value), encoding="utf-8")


def normalize_path(path: Path) -> str:
    return str(path).replace("\\", "/")


def make_env(tmp: Path) -> dict[str, str]:
    env = {key: value for key, value in os.environ.items() if not key.startswith("CBM_")}
    home = tmp / "home"
    config = tmp / "config"
    appdata = tmp / "appdata"
    local = tmp / "localappdata"
    cache = tmp / "cache"
    for path in (home, config, appdata, local, cache):
        path.mkdir(parents=True, exist_ok=True)
    env.update(
        {
            "HOME": str(home),
            "USERPROFILE": str(home),
            "XDG_CONFIG_HOME": str(config),
            "APPDATA": str(appdata),
            "LOCALAPPDATA": str(local),
            "CBM_CACHE_DIR": str(cache),
            "CBM_LOG_FORMAT": "text",
            "CBM_WORKERS": "4",
            "LC_ALL": "C",
            "TZ": "UTC",
        }
    )
    return env


def run_cmd(cmd: list[str], env: dict[str, str]) -> str:
    proc = subprocess.run(
        cmd,
        cwd=ROOT,
        env=env,
        text=True,
        capture_output=True,
        timeout=TIMEOUT,
    )
    if proc.returncode != 0:
        raise ParityError(
            "命令失敗: {}\nrc={}\nstdout={}\nstderr={}".format(
                " ".join(cmd), proc.returncode, proc.stdout[-2000:], proc.stderr[-4000:]
            )
        )
    return proc.stdout


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
        first = value["content"][0]
        text = first.get("text") if isinstance(first, dict) else None
        if isinstance(text, str):
            try:
                return json.loads(text)
            except json.JSONDecodeError:
                return text
    return value


def run_cli_tool(binary: Path, env: dict[str, str], tool: str, args: dict[str, Any]) -> Any:
    raw = run_cmd(
        [str(binary), "cli", "--json", tool, json.dumps(args, separators=(",", ":"))],
        env,
    )
    return parse_tool_result(raw)


def write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def create_repo(root: Path) -> Path:
    repo = root / "repo"
    write(
        repo / "py" / "greeter.py",
        "\n".join(
            [
                "class Greeter:",
                "    def hello(self):",
                "        return 'hi'",
                "",
                "def make_greeter():",
                "    return Greeter()",
                "",
                "def log_call(fn):",
                "    return fn",
                "",
                "@log_call",
                "def decorated_helper():",
                "    return 'decorated'",
                "",
            ]
        ),
    )
    write(
        repo / "py" / "app.py",
        "\n".join(
            [
                "from greeter import Greeter, make_greeter",
                "",
                "def main():",
                "    g = make_greeter()",
                "    direct = Greeter()",
                "    return g.hello() + direct.hello()",
                "",
            ]
        ),
    )
    write(
        repo / "py" / "routes.py",
        "\n".join(
            [
                "from fastapi import Depends, FastAPI",
                "",
                "app = FastAPI()",
                "",
                "def decode_token(token: str):",
                "    return token",
                "",
                "def get_current_user(token: str):",
                "    return decode_token(token)",
                "",
                "@app.get('/profiles/{user_id}')",
                "def get_profile(user_id: str, user = Depends(get_current_user)):",
                "    return {'user': user, 'id': user_id}",
                "",
                "def fetch_profile():",
                "    return get_profile('42')",
                "",
            ]
        ),
    )
    write(
        repo / "ts" / "service.ts",
        "\n".join(
            [
                "export class Worker {",
                "  run(): string {",
                "    return helper();",
                "  }",
                "}",
                "",
                "export function helper(): string {",
                "  return 'ok';",
                "}",
                "",
            ]
        ),
    )
    write(
        repo / "ts" / "server.ts",
        "\n".join(
            [
                "import express from 'express';",
                "import { Worker } from './service';",
                "",
                "const app = express();",
                "",
                "app.get('/orders/:id', (_req, res) => {",
                "  const worker = new Worker();",
                "  res.send(worker.run());",
                "});",
                "",
                "export function start() {",
                "  return app;",
                "}",
                "",
            ]
        ),
    )
    write(
        repo / "go" / "main.go",
        "\n".join(
            [
                "package main",
                "",
                'import "net/http"',
                "",
                "func StartServer() {",
                '    http.HandleFunc("/healthz", HealthHandler)',
                "    ProcessOrder()",
                '    var n Namer = Entity{name: "api"}',
                "    n.Name()",
                "}",
                "",
                "func HealthHandler(w http.ResponseWriter, r *http.Request) {",
                "    w.WriteHeader(200)",
                "}",
                "",
            ]
        ),
    )
    write(
        repo / "go" / "orders.go",
        "\n".join(
            [
                "package main",
                "",
                "func ProcessOrder() string {",
                "    return Helper()",
                "}",
                "",
                "func Helper() string {",
                '    return "done"',
                "}",
                "",
                "type Namer interface {",
                "    Name() string",
                "}",
                "",
                "type Entity struct {",
                "    name string",
                "}",
                "",
                "func (e Entity) Name() string {",
                "    return e.name",
                "}",
                "",
            ]
        ),
    )
    write(
        repo / "config" / "deploy.yaml",
        "\n".join(
            [
                "apiVersion: apps/v1",
                "kind: Deployment",
                "metadata:",
                "  name: parity-api",
                "spec:",
                "  template:",
                "    spec:",
                "      containers:",
                "        - name: app",
                "          image: parity:latest",
                "",
            ]
        ),
    )
    for i in range(55):
        write(
            repo / "filler" / f"file_{i:02d}.py",
            "\n".join(
                [
                    f"def filler_{i:02d}():",
                    f"    return {i}",
                    "",
                ]
            ),
        )
    return repo


def db_path_for(env: dict[str, str]) -> Path:
    direct = Path(env["CBM_CACHE_DIR"]) / f"{PROJECT}.db"
    if direct.exists():
        return direct
    matches = sorted(Path(env["CBM_CACHE_DIR"]).glob("*.db"))
    if len(matches) == 1:
        return matches[0]
    raise ParityError(f"找不到 index DB: {env['CBM_CACHE_DIR']}")


def query_pairs(conn: sqlite3.Connection, sql: str, args: tuple[Any, ...]) -> list[list[Any]]:
    return [list(row) for row in conn.execute(sql, args).fetchall()]


def histogram(conn: sqlite3.Connection, table: str, column: str) -> dict[str, int]:
    rows = conn.execute(
        f"SELECT {column}, COUNT(*) FROM {table} WHERE project = ? GROUP BY {column} ORDER BY {column}",
        (PROJECT,),
    ).fetchall()
    return {str(key): int(count) for key, count in rows}


def summarize_db(db_path: Path) -> dict[str, Any]:
    conn = sqlite3.connect(str(db_path))
    try:
        conn.row_factory = sqlite3.Row
        node_hist = histogram(conn, "nodes", "label")
        edge_hist = histogram(conn, "edges", "type")
        interesting = (
            "Greeter",
            "hello",
            "main",
            "get_profile",
            "get_current_user",
            "decode_token",
            "Worker",
            "run",
            "helper",
            "start",
            "StartServer",
            "HealthHandler",
            "ProcessOrder",
            "Helper",
            "decorated_helper",
            "log_call",
            "Namer",
            "Entity",
            "Name",
        )
        placeholders = ",".join("?" for _ in interesting)
        relevant_edges = (
            "CALLS",
            "DATA_FLOWS",
            "DECORATES",
            "DEFINES",
            "DEFINES_METHOD",
            "HANDLES",
            "HTTP_CALLS",
            "IMPLEMENTS",
            "IMPORTS",
            "INHERITS",
            "OVERRIDE",
        )
        edge_placeholders = ",".join("?" for _ in relevant_edges)
        named_nodes = query_pairs(
            conn,
            f"""
            SELECT label, name, file_path
            FROM nodes
            WHERE project = ? AND name IN ({placeholders})
            ORDER BY label, name, file_path
            """,
            (PROJECT, *interesting),
        )
        edge_details = query_pairs(
            conn,
            f"""
            SELECT
              e.type,
              s.label,
              s.name,
              s.file_path,
              t.label,
              t.name,
              t.file_path,
              COALESCE(json_extract(e.properties, '$.strategy'), ''),
              COALESCE(json_extract(e.properties, '$.url_path'), '')
            FROM edges e
            JOIN nodes s ON s.id = e.source_id
            JOIN nodes t ON t.id = e.target_id
            WHERE e.project = ?
              AND e.type IN ({edge_placeholders})
              AND (
                s.name IN ({placeholders})
                OR t.name IN ({placeholders})
                OR json_extract(e.properties, '$.url_path') IS NOT NULL
                OR e.type IN ('HANDLES', 'HTTP_CALLS', 'DATA_FLOWS', 'IMPORTS', 'INHERITS', 'IMPLEMENTS', 'DECORATES')
              )
            ORDER BY e.type, s.file_path, s.name, t.file_path, t.name
            """,
            (PROJECT, *relevant_edges, *interesting, *interesting),
        )
        route_nodes = conn.execute(
            "SELECT COUNT(*) FROM nodes WHERE project = ? AND label = 'Route'",
            (PROJECT,),
        ).fetchone()[0]
        url_path_edges = conn.execute(
            """
            SELECT COUNT(*) FROM edges
            WHERE project = ? AND json_extract(properties, '$.url_path') IS NOT NULL
            """,
            (PROJECT,),
        ).fetchone()[0]
        lsp_call_edges = conn.execute(
            """
            SELECT COUNT(*) FROM edges
            WHERE project = ?
              AND type = 'CALLS'
              AND json_extract(properties, '$.strategy') LIKE 'lsp%'
            """,
            (PROJECT,),
        ).fetchone()[0]
        coverage = {
            "definitions": edge_hist.get("DEFINES", 0) > 0,
            "calls": edge_hist.get("CALLS", 0) > 0,
            "imports": edge_hist.get("IMPORTS", 0) > 0,
            "routes": int(route_nodes) > 0 or int(url_path_edges) > 0,
            "semantic": any(edge_hist.get(edge, 0) > 0 for edge in ("INHERITS", "IMPLEMENTS", "DECORATES")),
            "lsp_calls": int(lsp_call_edges) > 0,
        }
        return {
            "coverage": coverage,
            "edge_type_histogram": edge_hist,
            "node_label_histogram": node_hist,
            "interesting_nodes": named_nodes,
            "interesting_edges": edge_details,
        }
    finally:
        conn.close()


def assert_coverage(summary: dict[str, Any]) -> None:
    missing = [name for name, ok in summary["coverage"].items() if not ok]
    if missing:
        raise ParityError(f"代表性語言 fixture 未覆蓋必要 graph 面向: {', '.join(missing)}")


def run_index(binary: Path, tmp: Path, label: str) -> dict[str, Any]:
    env = make_env(tmp / f"env-{label}")
    repo = create_repo(tmp / f"repo-{label}")
    result = run_cli_tool(
        binary,
        env,
        "index_repository",
        {"repo_path": normalize_path(repo), "mode": "fast", "name": PROJECT},
    )
    if not isinstance(result, dict) or result.get("status") != "indexed":
        raise ParityError(f"{label} index_repository 未回傳 indexed: {result!r}")
    summary = summarize_db(db_path_for(env))
    assert_coverage(summary)
    return {
        "index": {
            "project": result.get("project"),
            "status": result.get("status"),
            "nodes": result.get("nodes"),
            "edges": result.get("edges"),
        },
        "graph": summary,
    }


def assert_no_path_leaks(value: Any, paths: list[Path]) -> None:
    blob = canonical_json(value)
    leaks = [normalize_path(path) for path in paths if normalize_path(path) in blob]
    if leaks:
        raise ParityError("golden 含有本機或暫存路徑:\n" + "\n".join(sorted(set(leaks))))


def compare_exact(name: str, expected: Any, actual: Any) -> None:
    if expected != actual:
        raise ParityError(
            f"{name} 不一致\n--- expected ---\n{canonical_json(expected)}"
            f"--- actual ---\n{canonical_json(actual)}"
        )


def build_actual(default_binary: Path, rust_binary: Path) -> dict[str, Any]:
    with tempfile.TemporaryDirectory(prefix="cbm-rust-lang-parity-") as td:
        tmp = Path(td)
        default = run_index(default_binary, tmp, "default")
        rust = run_index(rust_binary, tmp, "rust-pipeline")
        compare_exact("Rust pipeline opt-in graph parity", default, rust)
        actual = {
            "fixture": PROJECT,
            "opt_in": {
                "CBM_USE_RUST_PIPELINE_PLAN": "1",
                "CBM_USE_RUST_PIPELINE_REGISTRY": "1",
            },
            "summary": default,
        }
        assert_no_path_leaks(actual, [tmp, default_binary.parent, rust_binary.parent])
        return actual


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("default_binary", type=Path)
    parser.add_argument("rust_pipeline_binary", type=Path)
    parser.add_argument("--update", action="store_true")
    args = parser.parse_args()
    for binary in (args.default_binary, args.rust_pipeline_binary):
        if not binary.exists():
            raise ParityError(f"binary 不存在: {binary}")

    actual = build_actual(args.default_binary.resolve(), args.rust_pipeline_binary.resolve())
    if args.update:
        write_json(GOLDEN_PATH, actual)
    else:
        compare_exact("language graph parity golden", load_json(GOLDEN_PATH), actual)
    print(f"language graph parity ok: {GOLDEN_PATH.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ParityError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise SystemExit(1)
