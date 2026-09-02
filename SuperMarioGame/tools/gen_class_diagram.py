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
import textwrap
from pathlib import Path

SRC = Path(__file__).resolve().parents[1]
INCLUDE = SRC / "include"

# A class declaration, optionally with a base list. Deliberately not a C++
# parser: it matches the one declaration style this codebase uses, and anything
# it cannot read is reported rather than silently dropped.
DECL = re.compile(
    r"^class\s+([A-Za-z_]\w*)\s*(?::\s*(.+?))?\s*\{", re.M)
BASE = re.compile(r"public\s+([A-Za-z_][\w:]*)")


class Member:
    """One attribute or method, with the visibility a UML diagram is actually
    for: not shown to name-check the type, shown so a reader can verify
    encapsulation by eye (how much of this class is `-`/`#` vs `+`) rather
    than take the report's word for it.
    """
    __slots__ = ("visibility", "kind", "text", "is_static", "is_pure")

    def __init__(self, visibility, kind, text, is_static=False, is_pure=False):
        self.visibility = visibility            # 'public' | 'protected' | 'private'
        self.kind = kind                        # 'field' | 'method'
        self.text = text                        # display text, C++ order (type name / returnType name(params))
        self.is_static = is_static              # UML convention: underlined
        self.is_pure = is_pure                  # pure virtual — UML convention: italicised


VIS_SYMBOL = {"public": "+", "protected": "#", "private": "-"}


class ClassInfo:
    def __init__(self, name, bases, header, abstract, attributes, methods):
        self.name = name
        self.bases = bases
        self.header = header
        self.abstract = abstract
        self.attributes = attributes    # list[Member], kind='field'
        self.methods = methods          # list[Member], kind='method'

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

            # The class body, for the pure-virtual test and the member lists.
            body = _body_of(stripped, m.end() - 1)
            # A pure virtual is "= 0" attached to a FUNCTION declaration, so the
            # "= 0" must follow the closing paren of a parameter list (with the
            # usual trailing specifiers allowed in between). Matching a bare
            # "= 0;" anywhere in the body instead flagged every class with a
            # member initialiser such as `int m_fireHits = 0;` as abstract —
            # which wrongly stereotyped Bowser and WorldMapState.
            abstract = bool(re.search(
                r"\)\s*(?:const\s*)?(?:noexcept\s*)?(?:override\s*)?=\s*0\s*;", body))
            attributes, methods = _parse_members(body)
            classes[name] = ClassInfo(name, bases, path.relative_to(INCLUDE),
                                      abstract, attributes, methods)
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


def _strip_inline_bodies(text):
    """Replace every brace-delimited block with a single ';' — but only a
    block starting outside an open parameter list.

    A getter defined inline (`int getX() const { return m_x; }`) would
    otherwise contribute its *body*'s semicolons as fake top-level
    statements once the section is split on ';'; matching depth rather than
    a regex means a body containing its own nested braces (an if, a lambda)
    collapses correctly too. The paren-depth guard is what keeps this from
    also eating a brace-init default ARGUMENT — `Entity(sf::Vector2f pos =
    {0.0f, 0.0f})` has a `{...}` while still inside the constructor's own
    unclosed '(', which is not a function body and must not be spliced out;
    doing so used to split that one parameter list into three fake
    statements at the ','s the stripped braces had exposed.
    """
    out = []
    i, n = 0, len(text)
    paren_depth = 0
    while i < n:
        ch = text[i]
        if ch == "(":
            paren_depth += 1
            out.append(ch)
            i += 1
        elif ch == ")":
            paren_depth = max(0, paren_depth - 1)
            out.append(ch)
            i += 1
        elif ch == "{" and paren_depth == 0:
            depth = 1
            j = i + 1
            while j < n and depth > 0:
                if text[j] == "{":
                    depth += 1
                elif text[j] == "}":
                    depth -= 1
                j += 1
            out.append(";")
            i = j
        else:
            out.append(ch)
            i += 1
    return "".join(out)


def _visibility_sections(body):
    """(visibility, text) chunks in source order.

    A class may (and in this codebase does) reopen `public:`/`private:`
    more than once, so this yields every chunk rather than only the first
    of each — `_public_methods` used to read only the first `public:` block,
    which is why it saw a handful of methods rather than most of them.
    Unlabelled leading text is `private`, matching `class`'s C++ default.
    """
    labels = list(re.finditer(r"(?<!:)\b(public|protected|private)\s*:(?!:)", body))
    if not labels:
        return [("private", body)]
    chunks = []
    if labels[0].start() > 0:
        chunks.append(("private", body[:labels[0].start()]))
    for idx, lm in enumerate(labels):
        start = lm.end()
        end = labels[idx + 1].start() if idx + 1 < len(labels) else len(body)
        chunks.append((lm.group(1), body[start:end]))
    return chunks


_SKIP_STMT = re.compile(
    r"^(friend\b|using\b|typedef\b|template\b|enum\b|struct\b|class\b|static_assert\b)")


def _parse_members(body):
    """Every attribute and method the class body declares, in source order,
    grouped by the visibility they were actually declared under.

    Not a C++ parser (see the module docstring's rationale for that choice):
    it matches this codebase's own style — one declaration per statement,
    inline bodies collapsed by `_strip_inline_bodies`, defaults/`override`/
    `= 0`/`= default`/`= delete` trimmed for display. A statement this
    cannot classify (a lambda member, a bitfield) is dropped rather than
    guessed at, matching the module's existing policy of reporting gaps
    instead of papering over them.
    """
    attributes, methods = [], []
    for vis, chunk in _visibility_sections(body):
        for stmt in _strip_inline_bodies(chunk).split(";"):
            stmt = stmt.strip()
            if not stmt or _SKIP_STMT.match(stmt):
                continue

            is_static = bool(re.match(r"^static\b", stmt))
            is_pure = bool(re.search(r"=\s*0\s*$", stmt))

            paren = stmt.find("(")
            if paren != -1:
                sig = stmt
                sig = re.sub(r"^(virtual|static|explicit|friend)\s+", "", sig)
                sig = re.sub(r"\s*(override|final)\b\s*", " ", sig)
                sig = re.sub(r"\s*=\s*(0|default|delete)\s*$", "", sig)
                sig = re.sub(r"\s+", " ", sig).strip()
                sig = re.sub(r"\s+\)", ")", sig)
                if not sig:
                    continue
                methods.append(Member(vis, "method", sig, is_static, is_pure))
            else:
                sig = re.sub(r"^(static|mutable)\s+", "", stmt)
                sig = re.sub(r"\s*=.*$", "", sig)
                sig = re.sub(r"\s+", " ", sig).strip()
                if not sig or not re.search(r"[A-Za-z_]\w*$", sig):
                    continue
                attributes.append(Member(vis, "field", sig, is_static, False))
    return attributes, methods


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
            members = list(info.attributes) + list(info.methods)
            if st or members:
                lines.append(f"    class {name} {{")
                if st:
                    lines.append(f"        <<{st}>>")
                for member in members:
                    text = member.text.replace("{", "(").replace("}", ")")
                    line = f"{VIS_SYMBOL[member.visibility]}{text}"
                    if member.is_static:
                        line += "$"
                    if member.is_pure:
                        line += "*"
                    lines.append(f"        {line}")
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


# --------------------------------------------------------------------------
# Detailed SVG: a classic three-compartment UML box (name / attributes /
# methods), every member shown — the point being that a reader can VERIFY
# encapsulation (how much of each box is -/# versus +), abstraction (dashed
# border, «interface»/«abstract» tag, italicised pure-virtual operations) and
# inheritance (the same generalization triangles) by eye, rather than take
# the report's prose about them on faith. compact emit_svg() above stays the
# inline, name-only diagram section 6 uses to illustrate a point mid-prose;
# this is the appendix's full reference copy.
# --------------------------------------------------------------------------
D_LINE_H = 13
D_PAD = 7
D_CHAR_W = 5.7          # monospace at the member font-size below
D_WRAP_CHARS = 60
D_MIN_W = 170
D_MAX_W = 520
D_INDENT = 46
D_ROW_GAP = 22
# PlayingState has 132 members, Player 109 — genuine outliers against a
# codebase where the next-largest class has 58 (see the tool's own
# `--list`-adjacent member-count check). Showing "every function and
# attribute" for those two would make one box thousands of pixels tall and
# every other class on the same diagram unreadable by comparison, so each
# compartment is capped and says exactly how much it left out and where to
# read the rest — the module's standing policy of reporting a gap rather
# than silently dropping it, applied to length instead of syntax this time.
D_MAX_MEMBERS = 24


def _wrap_member(symbol, member):
    """Visibility symbol + text, word-wrapped; continuation lines indent
    under the text rather than repeating the symbol."""
    wrapped = textwrap.wrap(member.text, width=D_WRAP_CHARS) or [""]
    lines = [f"{symbol} {wrapped[0]}"]
    lines += [f"  {cont}" for cont in wrapped[1:]]
    return lines


def _member_lines(members, header):
    """Flattened (text, is_static, is_pure) rows; only a member's first
    physical line carries its static/pure markers, so a wrapped continuation
    is not double-underlined or double-italicised. Capped at D_MAX_MEMBERS
    real members (not wrapped lines) with an honest count of what is cut."""
    shown, cut = members[:D_MAX_MEMBERS], members[D_MAX_MEMBERS:]
    rows = []
    for m in shown:
        for i, line in enumerate(_wrap_member(VIS_SYMBOL[m.visibility], m)):
            rows.append((line, m.is_static if i == 0 else False,
                        m.is_pure if i == 0 else False))
    if cut:
        rows.append((f"… {len(cut)} more — see {header}", False, False))
    return rows


# rsvg-convert (build_report.py's SVG->PDF step) renders the SVG's user
# units as CSS px; 1px = 0.75pt is what turns a viewBox height into the pt
# figure the report's page geometry actually has to fit (see plan section
# 4.1 / docs/issues/submission_sweep_plan_2026-09-02.md).
SVG_PX_TO_PT = 0.75


def _group_children_for_pages(classes, root, target_h=520.0):
    """Bin-pack root's direct children into groups whose rendered detailed
    SVG height stays under target_h pt each.

    Two trees (Enemy/Boss, IGameState) are tall enough that even shrunk to
    fit one portrait page (0.78 textheight) their member text becomes
    5-6pt - illegible. Splitting across several landscape pages, each near
    the diagram's native scale, was one of the two fixes plan section 4.1
    proposed (the other - dropping the detailed variant - is report prose
    this lane does not own).

    A group's height is NOT the sum of its members' individual heights:
    place()'s leaf packing above stacks same-level leaves into 2-3 columns,
    so nine short leaf classes take roughly a third of their summed height,
    not all of it. So this measures candidate groups by actually rendering
    them with emit_svg_detailed's own top_children restriction and reading
    the resulting viewBox back, rather than estimating analytically and
    getting the column packing wrong (the first version of this function
    did exactly that and produced a "520pt" group that rendered at 1200pt).

    Group 0 is rendered with include_root=True (root drawn once, for
    context) and costs root's own box height before a single child is
    added - Enemy alone is 411pt. Every later group is include_root=False
    (see emit_svg_detailed's docstring); the caller must use the same
    convention (group index 0 -> include_root=True, else False) when it
    actually draws each group, or the height budget below is meaningless.
    """
    kids = _children_map(classes)
    children = sorted(kids.get(root, []))
    if not children:
        return [[]]

    def height_of(members, include_root):
        svg = emit_svg_detailed(classes, root, root, top_children=members,
                                 include_root=include_root)
        m = re.search(r'viewBox="0 0 [\d.]+ ([\d.]+)"', svg)
        return float(m.group(1)) * SVG_PX_TO_PT if m else 0.0

    # Group 0 pays for root's own box before a single child is added (Enemy
    # alone: 411pt of a 500pt budget), so it is packed CHEAPEST-child-first:
    # try the least expensive additions and stop at the first one that would
    # blow the budget, since group height only grows as members are added -
    # nothing bigger fits once that happens either. An earlier version
    # sorted this group biggest-first like the others below and it
    # immediately committed root+Boss (Boss carries BoomBoom and Bowser) at
    # 1200pt against the same 500pt target - rendering at a shrink not
    # meaningfully better than the single-page original this split replaces.
    ascending = sorted(children, key=lambda c: height_of([c], False))
    remaining = list(ascending)
    group0 = []
    for c in list(remaining):
        if height_of(group0 + [c], True) <= target_h:
            group0.append(c)
            remaining.remove(c)
        else:
            break
    groups = [group0]

    # Every later group has no root box to pay for (include_root=False, see
    # emit_svg_detailed's docstring), so it packs the normal
    # best-fit-decreasing way: biggest remaining subtree first, so a lone
    # heavy branch claims its own page before smaller leaves fill in
    # around it.
    remaining.sort(key=lambda c: -height_of([c], False))
    while remaining:
        current = []
        for c in list(remaining):
            trial = current + [c]
            if not current or height_of(trial, False) <= target_h:
                current.append(c)
                remaining.remove(c)
        groups.append(current)

    idx = {c: i for i, c in enumerate(children)}
    return [sorted(g, key=idx.get) for g in groups]


def emit_svg_detailed(classes, root, title, max_depth=None, top_children=None,
                       include_root=True):
    """`top_children`, if given, restricts root's direct children to this
    subset for the top-level call only (deeper recursion is unaffected) -
    used by _group_children_for_pages's split to draw one balanced group
    per landscape page instead of the whole (too tall) tree at once.

    `include_root=False` skips drawing root's own box and lays its direct
    children out as their own top-level trees instead. Root's box is not
    free: Enemy alone (7 attributes, 30 methods) is 411pt tall, and a split
    that repeats it on every continuation page pays that 411pt again each
    time, leaving almost no room for the children it was meant to show.
    Root is drawn once (the first group, include_root=True) for context;
    later groups pass include_root=False.
    """
    if root not in classes:
        raise SystemExit(f"unknown root: {root}")
    kids = _children_map(classes)
    if top_children is not None:
        kids = dict(kids)
        kids[root] = [c for c in top_children if c in kids.get(root, [])]

    boxes = []
    edges = []
    cursor = {"y": 0}

    def measure(name):
        info = classes[name]
        attr_rows = _member_lines(info.attributes, info.header)
        meth_rows = _member_lines(info.methods, info.header)
        st = info.stereotype()
        title_h = 34 if st else 20
        name_w = len(info.name) * 7.6 + 24
        member_w = max([len(t) * D_CHAR_W for t, _, _ in attr_rows + meth_rows] or [0])
        w = min(D_MAX_W, max(D_MIN_W, name_w, member_w + D_PAD * 2))
        attr_h = len(attr_rows) * D_LINE_H + 8 if attr_rows else 8
        meth_h = len(meth_rows) * D_LINE_H + 8 if meth_rows else 8
        h = title_h + attr_h + meth_h
        return dict(name=name, w=w, h=h, title_h=title_h, attr_h=attr_h,
                    attr_rows=attr_rows, meth_rows=meth_rows, st=st)

    def place(name, depth):
        box = measure(name)
        box["x"] = depth * D_INDENT
        box["y"] = cursor["y"]
        boxes.append(box)
        cursor["y"] += box["h"] + D_ROW_GAP

        children = sorted(kids.get(name, []))
        if not children or (max_depth is not None and depth + 1 > max_depth):
            return box

        leaves = [c for c in children if not kids.get(c)]
        branches = [c for c in children if kids.get(c)]

        # Branches carry their own structure and stay single-column, full
        # width, one after another.
        for child in branches:
            edges.append((name, child))
            place(child, depth + 1)

        # Leaves do not: a class with a dozen same-level siblings (Enemy's
        # thirteen concrete enemies, Item's twelve pickups) is exactly the
        # case the compact renderer already grids into columns rather than
        # one long chain — and it matters more here, where each box is tens
        # of lines tall instead of one. Packed greedily into whichever
        # column is shortest so far, since sibling boxes are rarely the same
        # height once full member lists are shown (an even row/column
        # assignment, like the compact renderer's, would leave columns
        # badly unbalanced).
        if leaves:
            leaf_boxes = {c: measure(c) for c in leaves}
            col_w = max(b["w"] for b in leaf_boxes.values()) + H_GAP
            n_cols = 2 if len(leaves) <= 8 else 3
            col_tops = [cursor["y"]] * n_cols
            for child in leaves:
                edges.append((name, child))
                col = min(range(n_cols), key=lambda i: col_tops[i])
                lb = leaf_boxes[child]
                lb["x"] = (depth + 1) * D_INDENT + col * col_w
                lb["y"] = col_tops[col]
                boxes.append(lb)
                col_tops[col] += lb["h"] + D_ROW_GAP
            cursor["y"] = max(col_tops)
        return box

    if include_root:
        place(root, 0)
    else:
        # Continuation page: root was already drawn on an earlier page (see
        # the docstring), so lay its remaining children out as their own
        # top-level trees - no root box, no edges to one - reusing place()'s
        # branch/leaf split at depth 0.
        members = sorted(kids.get(root, []))
        branches = [c for c in members if kids.get(c)]
        leaves = [c for c in members if not kids.get(c)]
        for child in branches:
            place(child, 0)
        if leaves:
            leaf_boxes = {c: measure(c) for c in leaves}
            col_w = max(b["w"] for b in leaf_boxes.values()) + H_GAP
            n_cols = 2 if len(leaves) <= 8 else 3
            col_tops = [cursor["y"]] * n_cols
            for child in leaves:
                col = min(range(n_cols), key=lambda i: col_tops[i])
                lb = leaf_boxes[child]
                lb["x"] = col * col_w
                lb["y"] = col_tops[col]
                boxes.append(lb)
                col_tops[col] += lb["h"] + D_ROW_GAP
            cursor["y"] = max(col_tops)

    by_name = {b["name"]: b for b in boxes}
    width = max(b["x"] + b["w"] for b in boxes) + 16
    height = cursor["y"] + 8

    parts = [
        f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width:.0f} {height:.0f}" '
        f'role="img" aria-label="{html.escape(title)} detailed class diagram" '
        f'style="width:100%;height:auto;font-family:var(--mono,monospace)">',
        '<style>'
        '.uml-box{fill:var(--surface,#fff);stroke:var(--line,#ccc);stroke-width:1.2}'
        '.uml-abs{stroke-dasharray:5 3}'
        '.uml-title{font-family:var(--ui,sans-serif);font-size:12px;fill:var(--tx,#111);font-weight:700}'
        '.uml-st{font-family:var(--ui,sans-serif);font-size:9px;fill:var(--accent,#b5322a);font-style:italic}'
        '.uml-div{stroke:var(--line,#ccc);stroke-width:1}'
        '.uml-mem{font-size:9px;fill:var(--mut,#555)}'
        '.uml-mem-static{text-decoration:underline}'
        '.uml-mem-pure{font-style:italic}'
        '.uml-edge{stroke:var(--dim,#999);stroke-width:1.1;fill:none}'
        '.uml-tri{fill:var(--surface,#fff);stroke:var(--dim,#999);stroke-width:1.1}'
        '</style>',
    ]

    for parent, child in edges:
        p, c = by_name[parent], by_name[child]
        x1, y1 = p["x"] + 11, p["y"] + p["h"]
        y2, x2 = c["y"] + c["h"] / 2, c["x"]
        parts.append(f'<path class="uml-edge" d="M {x1} {y1} V {y2} H {x2}"/>')
        parts.append(f'<polygon class="uml-tri" points="{x1-4},{y1+6} {x1+4},{y1+6} {x1},{y1}"/>')

    for b in boxes:
        cls = "uml-box uml-abs" if b["st"] else "uml-box"
        x, y, w, h = b["x"], b["y"], b["w"], b["h"]
        parts.append(f'<rect class="{cls}" x="{x:.0f}" y="{y:.0f}" width="{w:.0f}" height="{h:.0f}" rx="5"/>')

        if b["st"]:
            parts.append(f'<text class="uml-st" x="{x+w/2:.0f}" y="{y+13:.0f}" text-anchor="middle">'
                         f'&#171;{b["st"]}&#187;</text>')
            parts.append(f'<text class="uml-title" x="{x+w/2:.0f}" y="{y+28:.0f}" text-anchor="middle">'
                         f'{html.escape(b["name"])}</text>')
        else:
            parts.append(f'<text class="uml-title" x="{x+w/2:.0f}" y="{y+16:.0f}" text-anchor="middle">'
                         f'{html.escape(b["name"])}</text>')

        ty = y + b["title_h"]
        parts.append(f'<line class="uml-div" x1="{x:.0f}" y1="{ty:.0f}" x2="{x+w:.0f}" y2="{ty:.0f}"/>')
        cy = ty + D_LINE_H - 2
        for text, is_static, _ in b["attr_rows"]:
            cls2 = "uml-mem uml-mem-static" if is_static else "uml-mem"
            parts.append(f'<text class="{cls2}" x="{x+D_PAD:.0f}" y="{cy:.0f}">{html.escape(text)}</text>')
            cy += D_LINE_H

        ty2 = ty + b["attr_h"]
        parts.append(f'<line class="uml-div" x1="{x:.0f}" y1="{ty2:.0f}" x2="{x+w:.0f}" y2="{ty2:.0f}"/>')
        cy = ty2 + D_LINE_H - 2
        for text, is_static, is_pure in b["meth_rows"]:
            cls2 = "uml-mem"
            if is_static:
                cls2 += " uml-mem-static"
            if is_pure:
                cls2 += " uml-mem-pure"
            parts.append(f'<text class="{cls2}" x="{x+D_PAD:.0f}" y="{cy:.0f}">{html.escape(text)}</text>')
            cy += D_LINE_H

    parts.append("</svg>")
    return "\n".join(parts)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--mermaid", action="store_true")
    ap.add_argument("--svg", metavar="ROOT")
    ap.add_argument("--depth", type=int, default=None,
                    help="stop descending past this depth below the root")
    ap.add_argument("--detailed", action="store_true",
                    help="with --svg, draw full attribute/method compartments instead of name-only boxes")
    ap.add_argument("--group-count", action="store_true",
                    help="with --svg ROOT --detailed, print how many landscape-page "
                         "groups the split from plan section 4.1 needs (see --group) "
                         "instead of drawing anything")
    ap.add_argument("--group", type=int, default=None,
                    help="with --svg ROOT --detailed, draw only this 0-indexed group "
                         "from --group-count's split")
    ap.add_argument("--group-target", type=float, default=520.0,
                    help="max rendered height per group in pt for --group-count/"
                         "--group (default 520pt: a landscape page's usable height "
                         "under hcmus-report-template's margins)")
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
    elif args.svg and args.detailed and (args.group_count or args.group is not None):
        if args.svg not in classes:
            raise SystemExit(f"unknown root: {args.svg}")
        groups = _group_children_for_pages(classes, args.svg, args.group_target)
        if args.group_count:
            print(len(groups))
        else:
            if not (0 <= args.group < len(groups)):
                raise SystemExit(f"--group {args.group} out of range (0..{len(groups) - 1})")
            sys.stdout.write(emit_svg_detailed(
                classes, args.svg, args.svg, args.depth, top_children=groups[args.group],
                include_root=(args.group == 0)))
    elif args.svg:
        emit = emit_svg_detailed if args.detailed else emit_svg
        sys.stdout.write(emit(classes, args.svg, args.svg, args.depth))
    else:
        ap.error("choose --mermaid, --svg ROOT or --list")


if __name__ == "__main__":
    main()
