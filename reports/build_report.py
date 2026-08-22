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
# The same content without a document skeleton, for publishing as an Artifact
# (the publisher supplies its own <head>/<body>).
OUT_ARTIFACT = HERE / "artifact_final_report.html"
# The LaTeX build: same prose, HCMUS format, for the submitted PDF.
TEX_DIR = REPO / "Report" / "SuperMarioGame"


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

# Project history, counted rather than remembered. The narrative in the report's
# process section quotes these, and a hand-typed count is exactly the kind of
# claim that is wrong by the next commit.
FACTS["sessions"] = sum(
    1 for line in (REPO / "logs" / "agent_history.log").open(encoding="utf-8", errors="ignore")
    if re.match(r"^\[20\d\d-\d\d-\d\d ", line))
FACTS["weeklies"] = len(sorted((REPO / "docs").glob("Group52_*/52.md")))


# Every uml() call, in the order the page makes them. The LaTeX pass replays
# this list to produce one PDF per diagram, so the two editions cannot end up
# with different figures in different orders — which a hand-kept parallel list
# would eventually do.
UML_CALLS = []


def uml(root, depth, caption, note=None):
    """A UML class diagram, generated from the headers at build time.

    Inline SVG rather than Mermaid: this document has to open from disk with no
    network (g-rule-14), so a diagram that needs a JS runtime to render would be
    a block of source text on the page. The SVG styles itself from the report's
    own CSS variables, so it follows the light/dark theme like everything else.
    """
    UML_CALLS.append((root, depth))
    svg = subprocess.run(
        [sys.executable, str(GAME / "tools" / "gen_class_diagram.py"),
         "--svg", root, "--depth", str(depth)],
        cwd=GAME, capture_output=True, text=True, check=True).stdout
    body = f'<figure class="uml"><div class="uml-scroll">{svg}</div>'
    if note:
        body += f'<figcaption>{caption}<br><span class="note">{note}</span></figcaption>'
    else:
        body += f'<figcaption>{caption}</figcaption>'
    return body + "</figure>"


def img(name, alt, caption):
    data = base64.b64encode((HERE / "assets" / name).read_bytes()).decode()
    return (f'<figure><img src="data:image/png;base64,{data}" alt="{alt}">'
            f'<figcaption>{caption}</figcaption></figure>')


def build_latex(content):
    """Emit the LaTeX body and the figures it includes.

    Same `content` the HTML page is built from - see reports/html_to_latex.py
    for why this is a conversion rather than a second manuscript.
    """
    sys.path.insert(0, str(HERE))
    import html_to_latex

    img_dir = TEX_DIR / "img"
    img_dir.mkdir(parents=True, exist_ok=True)

    # Screenshots: copied as-is.
    figure_map = {"img": {}, "svg": {}}
    for i, name in enumerate(sorted(p.name for p in (HERE / "assets").glob("*.png"))):
        target = img_dir / name
        target.write_bytes((HERE / "assets" / name).read_bytes())
        figure_map["img"][i] = f"img/{name}"

    # UML: the SVG is regenerated as PDF, so the diagrams stay vector in print
    # and still come from the headers rather than a checked-in picture.
    for i, (root, depth) in enumerate(UML_CALLS):
        svg = subprocess.run(
            [sys.executable, str(GAME / "tools" / "gen_class_diagram.py"),
             "--svg", root, "--depth", str(depth)],
            cwd=GAME, capture_output=True, text=True, check=True).stdout
        # The SVG styles itself from the report's CSS variables, which mean
        # nothing to a rasteriser; substitute the light-theme literals.
        for var, lit in (("var(--surface,#fff)", "#ffffff"),
                         ("var(--line,#ccc)", "#c9c4ba"),
                         ("var(--tx,#111)", "#1b1a18"),
                         ("var(--dim,#999)", "#8b867d"),
                         ("var(--accent,#b5322a)", "#b5322a"),
                         ("var(--ui,sans-serif)", "Helvetica,Arial,sans-serif")):
            svg = svg.replace(var, lit)
        svg_path = img_dir / f"uml_{root.lower()}_{depth}.svg"
        pdf_path = img_dir / f"uml_{root.lower()}_{depth}.pdf"
        svg_path.write_text(svg, encoding="utf-8")
        subprocess.run(["rsvg-convert", "-f", "pdf", "-o", str(pdf_path), str(svg_path)],
                       check=True)
        svg_path.unlink()
        figure_map["svg"][i] = f"img/{pdf_path.name}"

    body = html_to_latex.convert(content, figure_map)
    (TEX_DIR / "content").mkdir(parents=True, exist_ok=True)
    (TEX_DIR / "content" / "report_body.tex").write_text(
        "% GENERATED by reports/build_report.py - do not edit by hand.\n"
        "% The source is reports/report_content.py, which also produces the\n"
        "% HTML edition. Regenerate with: python3 reports/build_report.py\n\n"
        + body, encoding="utf-8")
    print(f"wrote {(TEX_DIR / 'content' / 'report_body.tex').relative_to(REPO)}  "
          f"({len(body) // 1024} KB, {len(figure_map['img'])} screenshots, "
          f"{len(figure_map['svg'])} UML PDFs)")


if __name__ == "__main__":
    from report_content import render
    content = render(FACTS, img, uml)

    # Explicit head/body boundary. A browser would infer it at the <main>, but
    # an inferred boundary is not one a reader of the file can see.
    head, _, body = content.partition("\n<main>")
    OUT.write_text(
        '<!doctype html>\n<html lang="en"><head><meta charset="utf-8">\n'
        '<meta name="viewport" content="width=device-width,initial-scale=1">\n'
        + head + "\n</head>\n<body>\n<main>" + body + "</body></html>\n",
        encoding="utf-8")
    OUT_ARTIFACT.write_text(content, encoding="utf-8")

    build_latex(content)

    kb = OUT.stat().st_size / 1024
    print(f"wrote {OUT.relative_to(REPO)}  ({kb:.0f} KB)")
    print(f"wrote {OUT_ARTIFACT.relative_to(REPO)}  (same content, no skeleton)")
    for k, v in FACTS.items():
        print(f"  {k:9s} {v}")
