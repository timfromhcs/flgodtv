#!/usr/bin/env python3
"""Generate machine-readable V1 repository inventory (Phase 1).
Scans the working tree; all counts are measured, never hardcoded.
"""
import json, os, re, hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

def sha256_file(p):
    h = hashlib.sha256()
    with open(p, "rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()

def files_under(rel, suffixes=None):
    base = ROOT / rel
    out = []
    if not base.exists():
        return out
    for p in sorted(base.rglob("*")):
        if p.is_file():
            if suffixes and p.suffix not in suffixes:
                continue
            out.append(p.relative_to(ROOT).as_posix())
    return out

headers = files_under("include", {".hpp"})
src = files_under("src", {".cpp", ".hpp"})
tests = files_under("tests", {".cpp"})
benchmarks = files_under("benchmarks", {".cpp"})
scripts = files_under("scripts", {".py"})
gd_scripts = files_under("godot/scripts", {".gd"})
docs = files_under("docs", {".md"})

cmake_text = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
add_tests = re.findall(r"add_test\s*\(\s*NAME\s+(\w+)", cmake_text)
executables = re.findall(r"add_executable\(\s*(\S+)", cmake_text)

inv = {
    "generated_by": "scripts/generate_v1_inventory.py",
    "counts": {
        "headers": len(headers),
        "src_files": len(src),
        "test_files": len(tests),
        "benchmark_files": len(benchmarks),
        "script_files": len(scripts),
        "godot_scripts": len(gd_scripts),
        "doc_files": len(docs),
        "cmake_add_test": len(add_tests),
        "cmake_add_executable": len(executables),
    },
    "headers": headers,
    "src": src,
    "tests": tests,
    "benchmarks": benchmarks,
    "scripts": scripts,
    "godot_scripts": gd_scripts,
    "docs": docs,
    "cmake_tests": sorted(add_tests),
    "cmake_executables": sorted(executables),
    "mpe_related": sorted([f for f in headers + tests
                           if any(k in f for k in ("agent", "brain", "world", "evolution",
                                                  "learning", "social", "language",
                                                  "technology", "ecosystem", "colony"))]),
    "v1_gaps": {
        "scenarios_dir": (ROOT / "scenarios").exists(),
        "python_package": any((ROOT / n).exists() for n in ("pyproject.toml", "setup.py")),
        "tests_edge": (ROOT / "tests" / "edge").exists(),
        "cmake_install_rules": bool(re.search(r"^\s*install\s*\(", cmake_text, re.M)),
        "cpack": "CPack" in cmake_text,
    },
}
out = ROOT / "evidence" / "v1" / "repository_inventory.json"
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(json.dumps(inv, indent=2), encoding="utf-8")
print(f"wrote {out} ({len(headers)} headers, {len(tests)} tests, {len(add_tests)} ctest)")
