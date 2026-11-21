#!/usr/bin/env python3
"""Spec <-> test traceability check.

Source functions carry a Doxygen anchor ``@anchor spec_<function>``. A unit
test file named ``<function>.c`` under tests/unit/src/{dwm,drw,util}/ verifies
that function. Integration suites cover subsystem sections in
docs/ARCHITECTURE.md and are not matched by stem.

It also enforces the expanded contract convention: every anchored Doxygen
block must contain @details, @par Side Effects: and @par Threading and
Reentrancy:, so documentation regressions fail the check.

Exit status:
  0  every unit test names an existing anchor, anchors are unique, and every
     contract has the required tags
  1  a unit test names a missing anchor, an anchor is duplicated, or a contract
     is missing a required tag

Missing tests for anchors are reported as warnings: some anchors belong to
helpers, ``main``, macros, or functions only exercised through integration
suites.

Usage: python3 tests/spec_check.py [--strict]
"""

from __future__ import annotations

import argparse
import re
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SOURCE_FILES = ["dwm.c", "drw.c", "util.c", "dwm.h", "drw.h", "util.h"]
UNIT_ROOTS = [ROOT / "tests" / "unit" / "src" / d for d in ("dwm", "drw", "util")]

ANCHOR_RE = re.compile(r"@anchor\s+spec_([A-Za-z0-9_]+)")
BLOCK_RE = re.compile(r"/\*\*(.*?)\*/", re.S)
BLOCK_ANCHOR_RE = re.compile(r"^\s*\*\s*@anchor\s+spec_([A-Za-z0-9_]+)", re.M)

# Tags required in each expanded function contract (docs/ARCHITECTURE.md §2).
CONTRACT_TAGS = ("@brief", "@details", "@par Side Effects:",
                 "@par Threading and Reentrancy:")

# Test files whose stem is not a plain function name.
EXEMPT_TESTS = {
    "selfcontained_drw_cache",   # standalone replica of the glyph cache
    "selfcontained_emoji_cache", # standalone replica of the emoji cache
}
# Anchors that intentionally have no same-named unit test.
EXEMPT_ANCHORS = {"main"}


def collect_anchors() -> tuple[dict[str, list[str]], list[str]]:
    anchors: dict[str, list[str]] = defaultdict(list)
    for name in SOURCE_FILES:
        path = ROOT / name
        if not path.exists():
            continue
        for lineno, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            m = ANCHOR_RE.search(line)
            if m:
                anchors[m.group(1)].append(f"{name}:{lineno}")
    duplicates = sorted(a for a, where in anchors.items() if len(where) > 1)
    return anchors, duplicates


def collect_contracts() -> list[tuple[str, str, list[str]]]:
    """Return (name, location, missing-tags) for anchored Doxygen blocks."""
    missing: list[tuple[str, str, list[str]]] = []
    for name in SOURCE_FILES:
        path = ROOT / name
        if not path.exists():
            continue
        text = path.read_text(encoding="utf-8")
        for match in BLOCK_RE.finditer(text):
            block = match.group(1)
            anchor = BLOCK_ANCHOR_RE.search(block)
            if not anchor:
                continue
            absent = [tag for tag in CONTRACT_TAGS if tag not in block]
            if absent:
                lineno = text.count("\n", 0, match.start()) + 1
                missing.append((anchor.group(1), f"{name}:{lineno}", absent))
    return missing


def collect_tests() -> list[tuple[str, Path]]:
    tests: list[tuple[str, Path]] = []
    for root in UNIT_ROOTS:
        if not root.exists():
            continue
        for path in sorted(root.glob("*.c")):
            tests.append((path.stem, path))
    return tests


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--strict",
        action="store_true",
        help="also fail when an anchor has no same-named unit test",
    )
    args = parser.parse_args()

    anchors, duplicates = collect_anchors()
    tests = collect_tests()
    contract_gaps = collect_contracts()

    errors: list[str] = []
    warnings: list[str] = []

    if duplicates:
        for name in duplicates:
            errors.append(f"duplicate anchor spec_{name}: {', '.join(anchors[name])}")

    for name, loc, absent in contract_gaps:
        errors.append(f"{loc}: contract spec_{name} missing {', '.join(absent)}")

    covered: set[str] = set()
    for stem, path in tests:
        rel = path.relative_to(ROOT)
        if stem in EXEMPT_TESTS:
            continue
        if stem.startswith("integration_") or stem.startswith("contradiction_"):
            continue
        if stem not in anchors:
            errors.append(f"{rel}: no source anchor spec_{stem}")
        else:
            covered.add(stem)

    untested = sorted(set(anchors) - covered - EXEMPT_ANCHORS)
    if untested:
        warnings.append(
            f"{len(untested)} anchor(s) without a same-named unit test: "
            + ", ".join(untested)
        )

    total = len(anchors)
    print(f"spec_check: {total} anchors, {len(tests)} unit test files, "
          f"{len(covered)} verified by name, {len(contract_gaps)} contract gaps")

    for warning in warnings:
        print(f"warning: {warning}")
    for error in errors:
        print(f"error: {error}", file=sys.stderr)

    if errors:
        return 1
    if args.strict and untested:
        return 1
    print("spec_check: OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
