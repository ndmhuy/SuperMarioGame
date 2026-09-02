#!/usr/bin/env python3
"""Turn the report's generated HTML body into LaTeX.

Why a converter and not a second document
-----------------------------------------
The project needs the report in two forms: a self-contained HTML page that
opens from disk, and an HCMUS-formatted PDF for submission. Writing the prose
twice would put fifteen sections of text in two files that drift apart - which
is the exact failure this report spends section 12 describing, and it would be
embarrassing to commit it in the same change.

So report_content.render() stays the single source and this converts its
output. That is only safe because the HTML is itself generated: it uses a
small, regular vocabulary of tags, listed in TAGS below. Anything outside that
vocabulary raises rather than being silently dropped - a converter that quietly
skips what it does not understand produces a PDF that is missing paragraphs
nobody notices.
"""

import html.parser
import re
import subprocess
import sys
from pathlib import Path

# Every tag the report generator can emit. Extend deliberately.
TAGS = {
    "main", "h2", "h3", "h4", "p", "strong", "em", "code", "pre", "a", "ul",
    "ol", "li", "table", "thead", "tbody", "tr", "th", "td", "div", "span",
    "figure", "figcaption", "img", "blockquote", "br", "footer", "b", "svg",
    "h1", "hr",
}

# Tags that never have a closing tag, so they must not move the nesting depth.
VOID = {"img", "br", "hr", "meta", "link", "input"}

# LaTeX's ten special characters, plus the ones that only bite inside text.
ESCAPES = {
    "\\": r"\textbackslash{}", "{": r"\{", "}": r"\}", "$": r"\$",
    "&": r"\&", "#": r"\#", "^": r"\textasciicircum{}", "_": r"\_",
    "%": r"\%", "~": r"\textasciitilde{}",
    # Not special to LaTeX, but not in the T1 font either.
    "\u00a7": r"\S{}",       # section sign, used for cross-references
    "\u00b7": r"\textperiodcentered{}",
    "\u2014": "---",
    "\u2013": "--",
    "\u2018": "`", "\u2019": "'",
    "\u201c": "``", "\u201d": "''",
    "\u00d7": r"$\times$",
    "\u2192": r"$\rightarrow$",
    "\u2190": r"$\leftarrow$",
    "\u2191": r"$\uparrow$",
    "\u2193": r"$\downarrow$",
    "\u2264": r"$\leq$", "\u2265": r"$\geq$",
    "\u00ab": r"$\ll$", "\u00bb": r"$\gg$",
    "\u2026": r"\ldots{}",
    "\u00b2": r"$^2$", "\u00bd": r"$\frac{1}{2}$",
    "\u2011": "-",
    "\u00a0": "~",
    "\u2194": r"$\leftrightarrow$",
    "\u2500": "-", "\u2502": "|", "\u251c": "+", "\u2514": "+", "\u2510": "+",
    "\u250c": "+", "\u2518": "+", "\u253c": "+", "\u252c": "+", "\u2534": "+",
    "\u00b0": r"$^{\circ}$",
    "\u2022": r"\textbullet{}",
    "\u2032": "'", "\u2033": "''",
    "\u00e9": r"\'e",
}


def _unmapped(text):
    """Non-ASCII characters with no mapping and no Vietnamese diacritic.

    Reported rather than passed through: pdflatex stops on an unknown Unicode
    character, and finding out at compile time which of 60KB of prose contains
    it is slower than being told here.
    """
    import unicodedata
    bad = set()
    for ch in text:
        if ord(ch) < 128 or ch in ESCAPES:
            continue
        # Vietnamese letters are handled by the vietnam package.
        if unicodedata.category(ch).startswith("L"):
            continue
        bad.add(ch)
    return bad


def esc(text):
    out = []
    for ch in text:
        out.append(ESCAPES.get(ch, ch))
    return "".join(out)


def esc_verbatim(text):
    """For lstlisting bodies, which take the text raw."""
    return text


# --------------------------------------------------------------------------
# Breaking long code tokens (plan section 4.1, defect 2)
#
# \codebreak (main.tex) already permits a line break after / _ . : - inside
# \texttt, which handles most identifiers in this report (paths, ::-scoped
# names). It does nothing for a token that contains NONE of those five
# characters - a plain camelCase identifier such as IMovementStrategy
# (18 chars) - which \texttt then refuses to break at all, so it either
# blows out into the next table column or overflows the line in prose.
#
# esc_code() finds every such run inside a <code> span and, ONLY when the
# run is longer than CODE_BREAK_THRESHOLD characters, inserts \allowbreak{}
# between every character of it. This is deliberately driven by run length
# alone, not by a list of known-bad identifiers: 5A/5B/5C add new prose and
# new tables with names that do not exist yet (a design-pattern subsection,
# a SOLID walkthrough, a figure catalogue), and any of them can introduce
# the next 18-character identifier with no punctuation in it. A fix keyed
# to "IMovementStrategy" specifically would not catch it; a fix keyed to
# "any unbroken run longer than N chars" does, by construction.
# --------------------------------------------------------------------------
CODE_BREAK_THRESHOLD = 10          # chars; matches plan section 4.1
_CODE_BREAK_CHARS = set("/_.:-")   # the punctuation \codebreak already breaks after


def _code_runs(text):
    """Yield (run_text, is_breakable_run) pairs splitting on whitespace and
    _CODE_BREAK_CHARS. Whitespace/punctuation characters are yielded singly
    with is_breakable_run=False (nothing to insert inside a single char);
    everything else is grouped into maximal runs."""
    i, n = 0, len(text)
    while i < n:
        ch = text[i]
        if ch.isspace() or ch in _CODE_BREAK_CHARS:
            yield ch, False
            i += 1
            continue
        j = i
        while j < n and not text[j].isspace() and text[j] not in _CODE_BREAK_CHARS:
            j += 1
        yield text[i:j], True
        i = j


def esc_code(text):
    """Like esc(), but for text inside a <code> span: any run of
    CODE_BREAK_THRESHOLD+1 or more non-whitespace, non-punctuation characters
    gets \\allowbreak{} inserted between every character, so \\codebreak's
    \\texttt has somewhere to break it regardless of what the identifier is
    (see the module note above)."""
    out = []
    for run, is_word in _code_runs(text):
        escaped_chars = [ESCAPES.get(ch, ch) for ch in run]
        if is_word and len(run) > CODE_BREAK_THRESHOLD:
            out.append("\\allowbreak{}".join(escaped_chars))
        else:
            out.append("".join(escaped_chars))
    return "".join(out)


def _longest_code_run(text):
    """Longest breakable run in text, capped at CODE_BREAK_THRESHOLD - above
    that, esc_code() makes it breakable, so it no longer needs a wide column
    to avoid overflowing (see _column_spec)."""
    longest = 0
    for run, is_word in _code_runs(text):
        if is_word:
            longest = max(longest, min(len(run), CODE_BREAK_THRESHOLD))
    return longest


PLAIN_WORD_CAP = 12   # chars; a plain-text word this long or longer has a
                       # decent chance of a LaTeX hyphenation point, so it is
                       # not held to the same one-line minimum as a shorter
                       # word/number that cannot break at all ("short",
                       # "tail", a single letter) - the same defect as
                       # unbreakable code, just without \codebreak's markup.


def _longest_plain_word(text):
    """Longest whitespace-delimited word in ordinary (non-code) text, capped
    at PLAIN_WORD_CAP for the same reason _longest_code_run caps at
    CODE_BREAK_THRESHOLD: past that length, wrapping is the wider LaTeX
    hyphenation algorithm's job, not this column-sizing heuristic's."""
    return min(max((len(w) for w in text.split() if w), default=0), PLAIN_WORD_CAP)


# --------------------------------------------------------------------------
# Table column widths (plan section 4.1, defect 1)
#
# The previous version picked p{} fractions purely by column COUNT (a
# 3-column table always got 0.22/0.16/0.54, a 5-column table always
# 0.14/0.13/0.24/0.24/0.10) with no regard for what was actually in them.
# A code identifier set in \small\texttt is roughly CHAR_PT_SMALL points per
# character; when a column's fixed fraction was narrower than its widest
# unbreakable token, that token overflowed into the next column - the
# "IMovementStrate|section 6.5" collision this whole module exists to fix.
#
# This instead measures every cell (see Converter._pad/_render_table) for
# (a) its longest breakable code run, capped at CODE_BREAK_THRESHOLD since
# esc_code() makes anything longer wrap instead of overflow, (b) its longest
# plain (non-code) word, capped at PLAIN_WORD_CAP for the same reason, and
# (c) its plain-text length. Each column's minimum width is set from
# whichever of (a)/(b) is more demanding, so a genuine identifier OR an
# ordinary short word/number ("Commits", "short", a bare digit) that cannot
# hyphenate always fits on one line; the width left over after all minimums
# are met is handed out in proportion to (c), so a prose-heavy column still
# gets more room than a one-word column. This works for any column count and
# any content - there is nothing here keyed to a specific table, caption or
# heading, because 5A/5B/5C add tables this lane has never seen.
#
# Code and plain text need DIFFERENT pt-per-character constants: \texttt is
# monospace and comparatively narrow, but table headers are set in
# \textbf{...} (bold Latin Modern Roman), which measured wider per character
# than \texttt at the same nominal size - "\textbf{Commits}" needs 7.1pt per
# character, not the ~5.4pt \texttt{IMovementStrategy} needs. Using the
# narrower constant for both understated "Commits"'s column and it overflowed
# by 9.8pt (caught rebuilding the real report with this fix in place, not
# invented) - CHAR_PT_PLAIN below is calibrated from \settowidth measurements
# of bold text, with headroom, since headers are always bold.
# --------------------------------------------------------------------------
CHAR_PT_CODE = 5.5          # \small\texttt, pt/char (measured IMovementStrategy: 5.43)
CHAR_PT_PLAIN = 7.4         # \textbf (table headers), pt/char (measured "Commits": 7.10)
# \footnotesize is ~9pt against \small's ~10.95pt in this document's 12pt
# base size class; both fonts scale roughly linearly with size.
_SIZE_RATIO = 9 / 10.95
CHAR_PT_CODE_FOOTNOTE = CHAR_PT_CODE * _SIZE_RATIO
CHAR_PT_PLAIN_FOOTNOTE = CHAR_PT_PLAIN * _SIZE_RATIO
LINEWIDTH_PT = 505.9       # \linewidth under hcmus-report-template's margins
TABCOLSEP_PT = 6.0        # LaTeX's default \tabcolsep, unchanged by this template


def _column_spec(cols, min_chars_code, min_chars_plain, prose_chars,
                  char_pt_code, char_pt_plain):
    """p{}-width spec for `cols` columns given, per column: the longest
    unbreakable code run (min_chars_code) and plain word (min_chars_plain),
    both in characters, and the total prose length (prose_chars, in
    characters) across all cells in that column."""
    if cols <= 0:
        return ""
    if cols == 1:
        return "p{0.94\\linewidth}"

    # Every column adds 2*\tabcolsep of padding OUTSIDE its own p{} width
    # (LaTeX's array package, unconditionally) - for 6 columns that is 72pt,
    # 14% of \linewidth, that a fixed per-column fudge factor was quietly
    # underestimating (a 6-column synthetic table overflowed the table's
    # own alignment box by 11pt with an earlier "1 - 0.02*cols" guess).
    # Compute the real overhead instead of approximating it.
    overhead_frac = (2 * TABCOLSEP_PT * cols) / LINEWIDTH_PT
    table_frac = max(0.3, 1.0 - overhead_frac - 0.01)

    # min_chars_* are already capped per-source (CODE_BREAK_THRESHOLD,
    # PLAIN_WORD_CAP) by the caller. A column's true minimum is whichever of
    # its code or plain requirement is larger in points, not their sum -
    # they come from the widest token in (possibly different) cells, not a
    # simultaneous demand in one cell.
    min_frac = [
        max(min_chars_code[i] * char_pt_code, min_chars_plain[i] * char_pt_plain)
        / LINEWIDTH_PT
        for i in range(cols)
    ]
    total_min = sum(min_frac)
    if total_min > table_frac:
        # A wall of long identifiers alone would blow the table width before
        # a word of prose is added. There is no way to give every column its
        # full minimum in this case - by definition the content does not fit
        # the page at any font size this table uses - so this scales every
        # minimum down proportionally rather than letting the table run off
        # the page outright, and leaves the 5pt Overfull guard (build.sh) to
        # catch the (rare, content-dependent) residual overflow this cannot
        # avoid.
        scale = table_frac / total_min
        min_frac = [f * scale for f in min_frac]
        total_min = table_frac

    remaining = table_frac - total_min
    total_prose = sum(prose_chars) or 1
    fracs = [
        min_frac[i] + remaining * (prose_chars[i] / total_prose)
        for i in range(cols)
    ]
    return "".join(f"p{{{f:.3f}\\linewidth}}" for f in fracs)


class Converter(html.parser.HTMLParser):
    def __init__(self, figure_map):
        super().__init__(convert_charrefs=True)
        self.out = []
        self.stack = []
        self.figure_map = figure_map      # img/svg index -> LaTeX include path
        self.fig_index = 0
        self.svg_index = 0
        self.depth = 0                    # element nesting depth
        # Depth at which the current skipped subtree began, or None. Tracked by
        # DEPTH rather than by counting a couple of tag names: the first version
        # decremented on any </div>, so the cover's inner <div class="uni">
        # closed the skip while still inside the cover and the <h1> escaped.
        self.skip_from = None
        self.in_pre = False
        self.pre_buf = []
        self.in_code = False              # inside <code>, not <pre> (esc_code applies)
        self.cell = []                    # current table cell text
        self.cell_plain_len = 0           # plain-text length of the current cell
        self.cell_code_min_chars = 0       # longest unbreakable code run so far
        self.cell_plain_min_chars = 0      # longest unbreakable plain word so far
        self.row = []
        self.rows = []
        self.in_table = False
        self.header_done = False
        self.pending_caption = None
        self.current_figure = None
        self.list_stack = []
        self.strip_heading_number = False

    # ---------------------------------------------------------------- helpers
    @property
    def skipping(self):
        return self.skip_from is not None

    def emit(self, s):
        """Route a fragment to whatever buffer is currently open.

        Captions and table cells are assembled separately and flushed on their
        closing tag, so markup emitted from a start tag - \\textbf{, a <br>,
        \\texttt{ - has to land in the same buffer as the text around it.
        Writing straight to self.out put a caption's line break OUTSIDE the
        caption, which LaTeX then read as a "\\\\" with no line to end.
        """
        if self.skipping:
            return
        if self.pending_caption is not None:
            self.pending_caption.append(s)
        elif self.in_table:
            self.cell.append(s)
        else:
            self.out.append(s)

    # ------------------------------------------------------------------ tags
    def handle_starttag(self, tag, attrs):
        a = dict(attrs)
        opening_depth = self.depth
        if tag not in VOID:
            self.depth += 1

        if self.skipping:
            return

        if tag in ("style", "title"):
            self.skip_from = opening_depth
            return

        if tag == "svg":
            # The diagram itself is included as a PDF built from the same SVG.
            entry = self.figure_map["svg"].get(self.svg_index)
            self.svg_index += 1
            if isinstance(entry, dict) and entry.get("landscape_parts"):
                # Too tall for one portrait page even at 0.78 textheight
                # (build_report.py's SPLIT_MIN_HEIGHT_PT) - plan section 4.1's
                # split: one landscape page per balanced group of subtrees,
                # each near its native scale instead of one shrunken page.
                parts = entry["landscape_parts"]
                n = len(parts)
                for k, part_path in enumerate(parts):
                    label = f"\\small\\itshape Part {k + 1} of {n}" if n > 1 else ""
                    self.out.append(
                        "\\begin{landscape}\n"
                        # hcmus-report-template's fancyhdr header/footer are
                        # laid out for a PORTRAIT page and do not rotate with
                        # \begin{landscape}'s content, so a page carrying
                        # both reads as the header sideways relative to the
                        # diagram (or vice versa, depending on the viewer's
                        # /Rotate handling) - suppress them on this page
                        # rather than ship a page where the two disagree.
                        "\\thispagestyle{empty}\n"
                        "\\begin{center}\n"
                        f"\\includegraphics[width=\\linewidth,height=0.86\\textheight,"
                        f"keepaspectratio]{{{part_path}}}\n\n"
                        + label +
                        "\n\\end{center}\n\\end{landscape}\n")
            elif entry:
                path = entry
                self.out.append(
                    "\\begin{center}\n"
                    f"\\includegraphics[width=\\linewidth,height=0.78\\textheight,"
                    f"keepaspectratio]{{{path}}}\n"
                    "\\end{center}\n")
            self.skip_from = opening_depth
            return

        if tag not in TAGS:
            raise SystemExit(f"html_to_latex: unhandled tag <{tag}> — extend TAGS "
                             f"or teach the converter about it")

        cls = a.get("class", "")

        if tag == "div" and "cover" in cls:
            # The cover is title.tex's job in LaTeX.
            self.skip_from = opening_depth
            return
        if tag == "div" and "toc" in cls:
            # \tableofcontents replaces it.
            self.skip_from = opening_depth
            return
        if tag == "div" and "kpi" in cls:
            self.skip_from = opening_depth
            return

        if tag in ("h2", "h3", "h4"):
            # The heading text carries its own number ("4 &middot; Requirements
            # coverage") because the HTML edition has no sectioning counter.
            # LaTeX does, so the number is stripped here rather than duplicated
            # as "4 4 - Requirements coverage".
            level = {"h2": "section", "h3": "subsection", "h4": "subsubsection"}[tag]
            self.out.append("\n\\" + level + "{")
            self.strip_heading_number = True
            self.stack.append("}\n")
        elif tag == "p":
            self.out.append("\n")
            self.stack.append("\n")
        elif tag in ("strong", "b"):
            self.emit("\\textbf{")
            self.stack.append("}")
        elif tag == "em":
            self.emit("\\emph{")
            self.stack.append("}")
        elif tag == "code" and not self.in_pre:
            # \codebreak, not \texttt: inline code in this report is mostly
            # paths and identifiers such as
            # "docs/issues/code_audit_2026-08-18.md", which \texttt refuses to
            # break and which then run 160pt into the margin. \codebreak (see
            # main.tex) permits a break after the punctuation they are full of;
            # in_code additionally routes the text through esc_code() so a
            # token with NO such punctuation (IMovementStrategy, or whatever
            # 5A/5B/5C's new prose names next) still gets somewhere to break.
            self.in_code = True
            self.emit("\\codebreak{")
            self.stack.append("}")
        elif tag == "pre":
            self.in_pre = True
            self.pre_buf = []
        elif tag == "a":
            href = a.get("href", "")
            if href.startswith("http"):
                self.emit("\\href{" + href.replace("%", r"\%") + "}{")
                self.stack.append("}")
            else:
                self.stack.append("")
        elif tag == "ul":
            self.out.append("\n\\begin{itemize}[leftmargin=*]\n")
            self.list_stack.append("itemize")
        elif tag == "ol":
            self.out.append("\n\\begin{enumerate}[leftmargin=*]\n")
            self.list_stack.append("enumerate")
        elif tag == "li":
            self.out.append("\\item ")
            self.stack.append("\n")
        elif tag == "table":
            self.in_table = True
            self.rows, self.row, self.cell = [], [], []
            self.header_done = False
        elif tag in ("tr",):
            self.row = []
        elif tag in ("td", "th"):
            self.cell = []
            self.cell_plain_len = 0
            self.cell_code_min_chars = 0
            self.cell_plain_min_chars = 0
        elif tag == "blockquote":
            self.out.append("\n\\begin{quote}\n\\itshape\n")
            self.stack.append("\n\\end{quote}\n")
        elif tag == "figure":
            self.current_figure = []
        elif tag == "figcaption":
            self.pending_caption = []
        elif tag == "img":
            path = self.figure_map["img"].get(self.fig_index)
            self.fig_index += 1
            if path:
                self.out.append(
                    "\\begin{center}\n"
                    f"\\includegraphics[width=\\linewidth]{{{path}}}\n"
                    "\\end{center}\n")
        elif tag == "br":
            # Inside a longtable cell, "\\" ends the ROW. p{} columns take
            # \newline for an in-cell break; the member table's two-line names
            # were silently splitting every row in half.
            if self.pending_caption is not None:
                self.emit(" \\\\ ")          # inside \begin{center}, safe after text
            elif self.in_table:
                self.emit(" \\newline ")
            else:
                self.emit(" \\\\ ")
        elif tag in ("span", "div", "main", "thead", "tbody", "footer"):
            self.stack.append("")

    def handle_endtag(self, tag):
        if tag not in VOID:
            self.depth -= 1
        if self.skipping:
            if self.depth <= self.skip_from:
                self.skip_from = None
            return

        if tag == "pre":
            self.in_pre = False
            body = "".join(self.pre_buf).strip("\n")
            self.out.append(
                "\n\\begin{lstlisting}[basicstyle=\\ttfamily\\scriptsize,"
                "numbers=none,frame=single,breaklines=true]\n"
                + body + "\n\\end{lstlisting}\n")
            return

        if tag in ("ul", "ol"):
            kind = self.list_stack.pop()
            self.out.append(f"\\end{{{kind}}}\n")
            return

        if tag == "code":
            self.in_code = False
            # fall through to the generic \stack pop below, which emits the
            # closing "}" for \codebreak{

        if tag in ("td", "th"):
            self.row.append(("".join(self.cell).strip(), self.cell_plain_len,
                             self.cell_code_min_chars, self.cell_plain_min_chars))
            self.cell = []
            return
        if tag == "tr":
            if self.row:
                self.rows.append(self.row)
            self.row = []
            return
        if tag == "table":
            self.in_table = False
            self.out.append(self._render_table())
            return

        if tag == "figcaption":
            text = "".join(self.pending_caption).strip() if self.pending_caption else ""
            self.pending_caption = None
            if text:
                self.out.append("\n\\begin{center}\\small\\itshape " + text +
                                "\\end{center}\n")
            return
        if tag == "figure":
            self.current_figure = None
            return
        if tag in ("img", "br", "svg"):
            return

        if self.stack:
            closing = self.stack.pop()
            if closing:
                self.emit(closing)

    def handle_data(self, data):
        if self.in_pre:
            self.pre_buf.append(data)
            return
        if self.skipping:
            return
        if self.pending_caption is not None:
            self.pending_caption.append(esc(data))
            return
        text = re.sub(r"\s+", " ", data)
        if self.strip_heading_number:
            # "4 \u00b7 Requirements coverage" or "6.1 The entity tree" -> drop the number.
            text = re.sub(r"^\s*\d+(?:\.\d+)?\s*(?:\u00b7|&middot;|-)?\s*", "", text)
            self.strip_heading_number = False
        if not text.strip() and not self.out:
            return
        if self.in_table:
            self.cell_plain_len += len(text)
            # A one-line minimum matters for ANY unbreakable token, not only
            # code: a plain word like "short" or "tail" cannot hyphenate
            # either, and a column sized purely from prose-weight gave it a
            # handful of points and overflowed just as surely as a bare
            # identifier did (see the synthetic-table test in this lane's
            # report). Tracked separately from code because the two need
            # different pt-per-character constants (_column_spec).
            if self.in_code:
                self.cell_code_min_chars = max(self.cell_code_min_chars,
                                                _longest_code_run(text))
            else:
                self.cell_plain_min_chars = max(self.cell_plain_min_chars,
                                                 _longest_plain_word(text))
        self.emit(esc_code(text) if self.in_code else esc(text))

    # ----------------------------------------------------------------- table
    def _render_table(self):
        if not self.rows:
            return ""
        cols = max(len(r) for r in self.rows)

        # Each cell is (latex_text, plain_len, code_min_chars,
        # plain_min_chars) - see the "Table column widths" note near the top
        # of this module. Measure every column's two minimums and its prose
        # weight (total plain-text length) across ALL rows, including the
        # header - a header like "What it shows" is short enough that it is
        # never the binding constraint, but "Commits" was, so nothing here
        # assumes headers are safe to skip.
        min_chars_code = [0] * cols
        min_chars_plain = [0] * cols
        prose_chars = [0] * cols
        has_code = False
        for r in self.rows:
            for ci, cell in enumerate(_pad(r, cols)):
                latex, plain_len, code_min, plain_min = cell
                min_chars_code[ci] = max(min_chars_code[ci], code_min)
                min_chars_plain[ci] = max(min_chars_plain[ci], plain_min)
                prose_chars[ci] += plain_len
                if "\\codebreak{" in latex:
                    has_code = True

        # A code-bearing table gets \footnotesize (plan section 4.1): the
        # same content needs less width per character at that size, which is
        # also why _column_spec takes the pt/char constants as parameters
        # instead of hardcoding them.
        if has_code:
            char_pt_code, char_pt_plain = CHAR_PT_CODE_FOOTNOTE, CHAR_PT_PLAIN_FOOTNOTE
        else:
            char_pt_code, char_pt_plain = CHAR_PT_CODE, CHAR_PT_PLAIN
        size_cmd = "\\footnotesize" if has_code else "\\small"
        spec = _column_spec(cols, min_chars_code, min_chars_plain, prose_chars,
                             char_pt_code, char_pt_plain)

        lines = ["\n{" + size_cmd + "\\begin{longtable}{" + spec + "}", "\\toprule"]
        head, *body = self.rows
        lines.append(" & ".join(f"\\textbf{{{c[0]}}}" for c in _pad(head, cols)) + " \\\\")
        lines.append("\\midrule\n\\endhead")
        for r in body:
            lines.append(" & ".join(c[0] for c in _pad(r, cols)) + " \\\\")
        lines.append("\\bottomrule")
        lines.append("\\end{longtable}}\n")
        return "\n".join(lines)


def _pad(row, n):
    return list(row) + [("", 0, 0, 0)] * (n - len(row))


def convert(html_text, figure_map):
    missing = _unmapped(html_text)
    if missing:
        raise SystemExit(
            "html_to_latex: no LaTeX mapping for " +
            ", ".join(f"{ch!r} (U+{ord(ch):04X})" for ch in sorted(missing)) +
            " — add it to ESCAPES")

    c = Converter(figure_map)
    c.feed(html_text)
    body = "".join(c.out)
    # Tidy: collapse runs of blank lines, drop spaces before punctuation that
    # the whitespace normalisation can introduce.
    body = re.sub(r"\n{3,}", "\n\n", body)
    body = re.sub(r"[ \t]+\n", "\n", body)
    body = re.sub(r"\n +", "\n", body)
    return body.strip() + "\n"
