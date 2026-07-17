#!/usr/bin/env python3
"""掃描 Rust 高風險 API，並要求以 allowlist 明確放行。"""

from __future__ import annotations

import re
import sys
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ALLOWLIST = ROOT / "scripts" / "rust-security-allowlist.txt"


@dataclass(frozen=True)
class Finding:
    kind: str
    path: str
    line: int
    text: str


@dataclass(frozen=True)
class AllowEntry:
    kind: str
    path: str
    needle: str
    reason: str


PATTERNS: list[tuple[str, re.Pattern[str]]] = [
    ("unsafe_allow", re.compile(r"allow\s*\(\s*unsafe_code\s*\)")),
    # 只辨識 Rust 關鍵字的實際語法，避免文件或 JSON 字串的自然語言誤報。
    (
        "unsafe",
        re.compile(r"\bunsafe\b(?=\s*(?:\{|(?:extern|fn|impl|trait)\b))"),
    ),
    ("process", re.compile(r"\bstd::process\b|\bCommand::|\bCommand\b")),
    ("network", re.compile(r"\bstd::net\b|\bTcp(Stream|Listener)\b|\bUdpSocket\b")),
    (
        "filesystem",
        re.compile(
            r"\bstd::fs\b|\bFile::(open|create)\b|\bOpenOptions\b|"
            r"\bfs::(write|remove_file|remove_dir|remove_dir_all|create_dir|create_dir_all)\b"
        ),
    ),
]


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def load_allowlist() -> list[AllowEntry]:
    if not ALLOWLIST.exists():
        raise RuntimeError(f"找不到 allowlist: {ALLOWLIST}")
    entries: list[AllowEntry] = []
    for lineno, raw in enumerate(ALLOWLIST.read_text(encoding="utf-8").splitlines(), 1):
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        parts = raw.split("\t", 3)
        if len(parts) != 4:
            raise RuntimeError(f"{rel(ALLOWLIST)}:{lineno}: allowlist 格式必須是 4 欄 TSV")
        entries.append(AllowEntry(*parts))
    return entries


def update_test_context(
    line: str,
    brace_depth: int,
    cfg_test_pending: bool,
    test_depth: int | None,
) -> tuple[bool, int, bool, int | None]:
    stripped = line.strip()
    pending_test_module = False
    if stripped.startswith("#[cfg(test)]"):
        cfg_test_pending = True
    elif cfg_test_pending and re.search(r"\bmod\s+tests\b", stripped):
        pending_test_module = True
        cfg_test_pending = False
    elif stripped and not stripped.startswith("#"):
        cfg_test_pending = False

    delta = line.count("{") - line.count("}")
    brace_depth += delta
    if pending_test_module:
        test_depth = brace_depth
    if test_depth is not None and brace_depth < test_depth:
        test_depth = None
    return pending_test_module, brace_depth, cfg_test_pending, test_depth


def scan_file(path: Path) -> list[Finding]:
    findings: list[Finding] = []
    brace_depth = 0
    cfg_test_pending = False
    test_depth: int | None = None
    rel_path = rel(path)
    for lineno, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        in_test_module = test_depth is not None
        for kind, pattern in PATTERNS:
            if kind == "unsafe" and PATTERNS[0][1].search(raw):
                continue
            if kind == "filesystem" and in_test_module:
                continue
            if pattern.search(raw):
                findings.append(Finding(kind, rel_path, lineno, raw.strip()))
        _entered, brace_depth, cfg_test_pending, test_depth = update_test_context(
            raw, brace_depth, cfg_test_pending, test_depth
        )
    return findings


def is_allowed(finding: Finding, entry: AllowEntry) -> bool:
    if finding.kind != entry.kind or finding.path != entry.path:
        return False
    return entry.needle == "*" or entry.needle in finding.text


def main() -> int:
    entries = load_allowlist()
    findings: list[Finding] = []
    for path in sorted((ROOT / "rust").glob("**/*.rs")):
        findings.extend(scan_file(path))

    unexpected: list[Finding] = []
    used_entries: set[int] = set()
    for finding in findings:
        matched = False
        for idx, entry in enumerate(entries):
            if is_allowed(finding, entry):
                used_entries.add(idx)
                matched = True
                break
        if not matched:
            unexpected.append(finding)

    stale = [entry for idx, entry in enumerate(entries) if idx not in used_entries]
    if unexpected or stale:
        if unexpected:
            print("FAIL: Rust 高風險 API 未列入 allowlist", file=sys.stderr)
            for item in unexpected:
                print(f"{item.path}:{item.line}: {item.kind}: {item.text}", file=sys.stderr)
        if stale:
            print("FAIL: Rust security allowlist 有未使用項目", file=sys.stderr)
            for item in stale:
                print(f"{item.path}: {item.kind}: {item.needle}", file=sys.stderr)
        return 1

    print(f"Rust security allowlist passed ({len(findings)} findings)")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
