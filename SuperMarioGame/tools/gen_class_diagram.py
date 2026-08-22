#!/usr/bin/env python3
"""Derive the project's UML class diagrams from the headers.

Why generated
-------------
class_diagram.md was hand-maintained and had rotted: by August 2026 it was
missing Boss, Bowser, BoomBoom, Spiny, Lakitu, ShadowMario, AIController,
ObjectPool, TimeRewindManager, Castle, BridgeAxe and EntityCatalogue - most of
the architecture worth diagramming - while still reading as the authoritative
picture of the codebase. g-rule-22: a document that restates facts the code
already contains must be generated from the code or must not exist.

So the inheritance graph, the abstract/interface stereotypes and the pattern
groupings are all read out of include/**/*.hpp. Emits two forms of the same
truth:

    --mermaid   Mermaid classDiagram source, for class_diagram.md
    --svg NAME  a self-contained SVG, for the report (which must open offline,
                so a Mermaid runtime is not an option)

Usage:
    python3 tools/gen_class_diagram.py --mermaid > ../class_diagram.md
    python3 tools/gen_class_diagram.py --svg entities
"""

import argparse
import html
import re
import sys
from pathlib import Path

SRC = Path(__file__).resolve().parents[1]
INCLUDE = SRC / "include"

# A class declaration, optionally with a base list. Deliberately not a C++
# parser: it matches the one declaration style this codebase uses, and anything
# it cannot read is reported rather than silently dropped.
DECL = re.compile(
    r"^class\s+([A-Za-z_]\w*)\s*(?::\s*(.+?))?\s*\{", re.M)
BASE = re.compile(r"public\s+([A-Za-z_][\w:]*)")


class ClassInfo:
    def __init__(self, name, bases, header, abstract, methods):
        self.name = name
        self.bases = bases
        self.header = header
        self.abstract = abstract
        self.methods = methods

    def stereotype(self):
        if self.name.startswith("I") and self.name[1:2].isupper() and self.abstract:
            return "interface"
        return "abstract" if self.abstract else None


def parse_headers():
    classes = {}
    for path in sorted(INCLUDE.rglob("*.hpp")):
        text = path.read_text(encoding="utf-8", errors="ignore")
        # Strip comments so a class named in prose is not mistaken for a decl.
        stripped = re.sub(r"//[^\n]*", "", text)
        stripped = re.sub(r"/\*.*?\*/", "", stripped, flags=re.S)

        for m in DECL.finditer(stripped):
            name, baselist = m.group(1), m.group(2) or ""
            bases = BASE.findall(baselist)

            # The class body, for the pure-virtual test and the method list.
            body = _body_of(stripped, m.end() - 1)
            # A pure virtual is "= 0" attached to a FUNCTION declaration, so the
            # "= 0" must follow the closing paren of a parameter list (with the
            # usual trailing specifiers allowed in between). Matching a bare
            # "= 0;" anywhere in the body instead flagged every class with a
            # member initialiser such as `int m_fireHits = 0;` as abstract —
            # which wrongly stereotyped Bowser and WorldMapState.
            abstract = bool(re.search(
                r"\)\s*(?:const\s*)?(?:noexcept\s*)?(?:override\s*)?=\s*0\s*;", body))
            methods = _public_methods(body)
            classes[name] = ClassInfo(name, bases, path.relative_to(INCLUDE),
                                      abstract, methods)
    return classes


def _body_of(text, brace_index):
    """Text between the matching braces starting at brace_index."""
    depth = 0
    for i in range(brace_index, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[brace_index + 1:i]
    return ""


def _public_methods(body, limit=5):
    """A few representative public methods, for the diagram's method compartment."""
    section = body.split("protected:")[0].split("private:")[0]
    if "public:" in section:
        section = section.split("public:", 1)[1]
    found = []
    for m in re.finditer(r"^\s*(?:virtual\s+)?(?:static\s+)?"
                         r"(?:[\w:<>&*~\s]+?)\s+([a-z]\w*)\s*\(([^)]*)\)", section, re.M):
        name = m.group(1)
        if name in ("if", "for", "while", "return", "switch"):
            continue
        if name not in found:
            found.append(name)
        if len(found) >= limit:
            break
    return found


# --------------------------------------------------------------------------
# Groupings. The only hand-written part of this file, because "which pattern
# does this class participate in" is intent, not something the headers state.
# Every member is CHECKED to exist, so a rename breaks the build of the diagram
# rather than quietly dropping a box.
# --------------------------------------------------------------------------
GROUPS = [
    ("Entity hierarchy", "Entity"),
    ("Game states (State)", "IGameState"),
    ("Input commands (Command)", "ICommand"),
    ("Movement strategies (Strategy)", "IMovementStrategy"),
    ("Player forms (State + Decorator)", "IPlayerState"),
    ("Difficulty (Strategy)", "IDifficultyStrategy"),
]


def descendants(classes, root):
    """Every class reachable from `root` by public inheritance, with its depth."""
    children = {}
    for c in classes.values():
        for b in c.bases:
            children.setdefault(b, []).append(c.name)

    out, seen = [], set()

    def walk(name, depth):
        if name in seen:
            return
        seen.add(name)
        out.append((name, depth))
        for kid in sorted(children.get(name, [])):
            walk(kid, depth + 1)

    walk(root, 0)
    return out


def emit_mermaid(classes):
    lines = [
        "# Super Mario Game — UML class diagrams",
        "",
        "> **GENERATED — do not edit by hand.** Produced by",
        "> `SuperMarioGame/tools/gen_class_diagram.py` from `include/**/*.hpp`.",
        "> Regenerate with:",
        "> ```bash",
        "> cd SuperMarioGame && python3 tools/gen_class_diagram.py --mermaid > ../class_diagram.md",
        "> ```",
        "> The previous hand-written version of this file had gone stale: it was",
        "> missing `Boss`, `Bowser`, `BoomBoom`, `Spiny`, `Lakitu`, `ShadowMario`,",
        "> `AIController`, `ObjectPool` and `TimeRewindManager` while still reading",
        "> as the authoritative picture of the codebase (g-rule-22).",
        "",
    ]
    for title, root in GROUPS:
        if root not in classes:
            print(f"WARNING: group root {root!r} not found", file=sys.stderr)
            continue
        nodes = descendants(classes, root)
        lines += [f"## {title}", "", "```mermaid", "classDiagram"]
        for name, _ in nodes:
            info = classes[name]
            st = info.stereotype()
            if st:
                lines.append(f"    class {name} {{")
                lines.append(f"        <<{st}>>")
                for meth in info.methods:
                    lines.append(f"        +{meth}()")
                lines.append("    }")
            elif info.methods:
                lines.append(f"    class {name} {{")
                for meth in info.methods:
                    lines.append(f"        +{meth}()")
                lines.append("    }")
            else:
                lines.append(f"    class {name}")
        for name, _ in nodes:
            for b in classes[name].bases:
                if b in dict(nodes):
                    lines.append(f"    {b} <|-- {name}")
        lines += ["```", ""]
    return "\n".join(lines) + "\n"


# --------------------------------------------------------------------------
# SVG. Hand-laid-out would rot as fast as the markdown did, so the geometry is
# computed: a tidy tree layout, boxes sized to their text.
# --------------------------------------------------------------------------
BOX_H = 28
V_GAP = 10
H_GAP = 10
INDENT = 22
CHAR_W = 7.2
PAD = 12
ST_W = 56       # room for the «abstract» / «interface» tag
LEAF_COLS = 3   # wrap a wide row of leaf classes into this many columns


def _children_map(classes):
    kids = {}
    for c in classes.values():
        for b in c.bases:
            kids.setdefault(b, []).append(c.name)
    for v in kids.values():
        v.sort()
    return kids


def _box_width(info):
    return PAD * 2 + len(info.name) * CHAR_W + (ST_W if info.stereotype() else 0)


def emit_svg(classes, root, title, max_depth=None):
    """Lay the inheritance tree out as indented rows, packing wide groups of
    leaf classes into columns.

    One box per row is the obvious layout and it is the wrong one here: the
    Entity tree has 53 classes, thirteen of which are sibling enemies, so it
    came out as a 280x2000 ribbon that no page can show. Leaves are therefore
    gridded, which is also how a reader actually wants to see "these thirteen
    are the same kind of thing".
    """
    if root not in classes:
        raise SystemExit(f"unknown root: {root}")
    kids = _children_map(classes)

    rows = []          # laid-out boxes
    edges = []         # (parent_name, child_name)
    cursor = {"y": 0}

    def place(name, depth):
        info = classes[name]
        x = depth * INDENT
        box = dict(name=name, x=x, y=cursor["y"], w=_box_width(info),
                   st=info.stereotype())
        rows.append(box)
        cursor["y"] += BOX_H + V_GAP

        children = kids.get(name, [])
        if not children or (max_depth is not None and depth + 1 > max_depth):
            return box

        leaves = [c for c in children if not kids.get(c)]
        branches = [c for c in children if kids.get(c)]

        # Branch children first, each recursing; they carry structure and
        # deserve their own row.
        for child in branches:
            edges.append((name, place(child, depth + 1)["name"]))

        if leaves:
            colw = max(_box_width(classes[c]) for c in leaves) + H_GAP
            for i, child in enumerate(leaves):
                col, row = i % LEAF_COLS, i // LEAF_COLS
                leafbox = dict(name=child,
                               x=(depth + 1) * INDENT + col * colw,
                               y=cursor["y"] + row * (BOX_H + V_GAP),
                               w=colw - H_GAP,
                               st=classes[child].stereotype())
                rows.append(leafbox)
                edges.append((name, child))
            used = (len(leaves) + LEAF_COLS - 1) // LEAF_COLS
            cursor["y"] += used * (BOX_H + V_GAP)
        return box

    place(root, 0)

    by_name = {r["name"]: r for r in rows}
    width = max(r["x"] + r["w"] for r in rows) + 16
    height = cursor["y"] + 8

    parts = [
        f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width:.0f} {height:.0f}" '
        f'role="img" aria-label="{html.escape(title)} inheritance diagram" '
        f'style="width:100%;height:auto;font-family:var(--ui,sans-serif)">',
        '<style>'
        '.uml-box{fill:var(--surface,#fff);stroke:var(--line,#ccc);stroke-width:1.2}'
        '.uml-abs{stroke-dasharray:5 3}'
        '.uml-name{font-size:12px;fill:var(--tx,#111);font-weight:600}'
        '.uml-st{font-size:9.5px;fill:var(--accent,#b5322a);font-style:italic}'
        '.uml-edge{stroke:var(--dim,#999);stroke-width:1.1;fill:none}'
        '.uml-tri{fill:var(--surface,#fff);stroke:var(--dim,#999);stroke-width:1.1}'
        '</style>',
    ]

    # Generalization: elbow down the parent's left rail into the child's left
    # edge, with UML's hollow triangle at the parent end.
    for parent, child in edges:
        p, c = by_name[parent], by_name[child]
        x1 = p["x"] + 11
        y1 = p["y"] + BOX_H
        y2 = c["y"] + BOX_H / 2
        x2 = c["x"]
        parts.append(f'<path class="uml-edge" d="M {x1} {y1} V {y2} H {x2}"/>')
    for parent in {p for p, _ in edges}:
        p = by_name[parent]
        x1, y1 = p["x"] + 11, p["y"] + BOX_H
        parts.append(f'<polygon class="uml-tri" points="{x1-4},{y1+6} {x1+4},{y1+6} {x1},{y1}"/>')

    for r in rows:
        cls = "uml-box uml-abs" if r["st"] else "uml-box"
        parts.append(f'<rect class="{cls}" x="{r["x"]:.0f}" y="{r["y"]:.0f}" '
                     f'width="{r["w"]:.0f}" height="{BOX_H}" rx="4"/>')
        parts.append(f'<text class="uml-name" x="{r["x"] + PAD:.0f}" '
                     f'y="{r["y"] + 18:.0f}">{html.escape(r["name"])}</text>')
        if r["st"]:
            parts.append(f'<text class="uml-st" x="{r["x"] + r["w"] - PAD:.0f}" '
                         f'y="{r["y"] + 18:.0f}" text-anchor="end">'
                         f'&#171;{r["st"]}&#187;</text>')

    parts.append("</svg>")
    return "\n".join(parts)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--mermaid", action="store_true")
    ap.add_argument("--svg", metavar="ROOT")
    ap.add_argument("--depth", type=int, default=None,
                    help="stop descending past this depth below the root")
    ap.add_argument("--list", action="store_true")
    args = ap.parse_args()

    classes = parse_headers()
    if args.list:
        for name in sorted(classes):
            info = classes[name]
            print(f"{name:28s} {'abstract' if info.abstract else '':9s} "
                  f"<- {', '.join(info.bases) or '-'}")
        print(f"\n{len(classes)} classes", file=sys.stderr)
    elif args.mermaid:
        sys.stdout.write(emit_mermaid(classes))
    elif args.svg:
        sys.stdout.write(emit_svg(classes, args.svg, args.svg, args.depth))
    else:
        ap.error("choose --mermaid, --svg ROOT or --list")


if __name__ == "__main__":
    main()
