#!/usr/bin/env python3
"""Build the CS202 final report as ONE self-contained HTML file.

Why a generator rather than a hand-written .html
-----------------------------------------------
g-rule-14 class B: a report is an AUTHORED artifact — the HTML *is* the
document, and it is committed. But it has to open from disk with no network
(and print to PDF cleanly), which means every screenshot must be embedded as a
data: URI. Doing that by hand would make the file unreadable and unmaintainable.

So the prose lives here, the images are inlined at build time, and a handful of
figures are read straight out of the repository rather than typed in — a number
that is counted cannot be stale (g-rule-22).

    python3 reports/build_report.py

Writes reports/Group52_SuperMarioGame_FinalReport.html
"""

import base64
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parent
GAME = REPO / "SuperMarioGame"
OUT  = HERE / "Group52_SuperMarioGame_FinalReport.html"


# --------------------------------------------------------------------------
# Figures counted from the tree, so the report cannot claim a stale number.
# --------------------------------------------------------------------------
def count_files(root, *suffixes):
    return sum(1 for p in root.rglob("*") if p.suffix in suffixes)


def count_lines(root, *suffixes):
    total = 0
    for p in root.rglob("*"):
        if p.suffix in suffixes:
            total += sum(1 for _ in p.open(encoding="utf-8", errors="ignore"))
    return total


def git(*args):
    return subprocess.run(["git", *args], cwd=REPO, capture_output=True,
                          text=True, check=True).stdout.strip()


def subclasses_of(base):
    """Concrete classes deriving from `base`, counted from the headers.

    The word boundary matters: a plain substring test for ": public Player"
    also matches ": public PlayerStateDecorator", which inflated the player
    count by one.
    """
    pattern = re.compile(r":\s*public\s+" + re.escape(base) + r"\b")
    names = []
    for p in (GAME / "include" / "Entities").rglob("*.hpp"):
        text = p.read_text(encoding="utf-8", errors="ignore")
        if pattern.search(text):
            names.append(p.stem)
    return sorted(names)


def ctest_targets():
    """Harnesses actually registered as CTest cases.

    add_verify_test() builds every harness but takes NO_CTEST for the ones that
    open a window and cannot run headless, so "number of test .cpp files" and
    "number of things CI runs" are different figures. Reporting the first as the
    second would overstate what is verified automatically.
    """
    text = (GAME / "CMakeLists.txt").read_text(encoding="utf-8")
    calls = re.findall(r"add_verify_test\(([a-z_0-9]+)[^)]*\)", text)
    headless = re.findall(r"add_verify_test\(([a-z_0-9]+)(?![^)]*NO_CTEST)[^)]*\)", text)
    return len(calls), len(headless)


FACTS = {
    "sources":   count_files(GAME / "src", ".cpp"),
    "headers":   count_files(GAME / "include", ".hpp"),
    "loc":       count_lines(GAME / "src", ".cpp") + count_lines(GAME / "include", ".hpp"),
    "harnesses": count_files(GAME / "tests", ".cpp"),
    "commits":   git("rev-list", "--count", "HEAD"),
    "head":      git("rev-parse", "--short", "HEAD"),
    "enemies":   len(subclasses_of("Enemy")) + len(subclasses_of("Boss")),
    "items":     len(subclasses_of("Item")),
    "blocks":    len(subclasses_of("Block")),
    "players":   len(subclasses_of("Player")),
}
FACTS["targets"], FACTS["ctests"] = ctest_targets()


def img(name, alt, caption):
    data = base64.b64encode((HERE / "assets" / name).read_bytes()).decode()
    return (f'<figure><img src="data:image/png;base64,{data}" alt="{alt}">'
            f'<figcaption>{caption}</figcaption></figure>')


if __name__ == "__main__":
    from report_content import render
    OUT.write_text(render(FACTS, img), encoding="utf-8")
    kb = OUT.stat().st_size / 1024
    print(f"wrote {OUT.relative_to(REPO)}  ({kb:.0f} KB)")
    for k, v in FACTS.items():
        print(f"  {k:9s} {v}")
