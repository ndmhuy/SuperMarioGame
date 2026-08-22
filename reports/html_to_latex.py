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
        self.cell = []                    # current table cell text
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
            path = self.figure_map["svg"].get(self.svg_index)
            self.svg_index += 1
            if path:
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
            # main.tex) permits a break after the punctuation they are full of.
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

        if tag in ("td", "th"):
            self.row.append("".join(self.cell).strip())
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
        self.emit(esc(text))

    # ----------------------------------------------------------------- table
    def _render_table(self):
        if not self.rows:
            return ""
        cols = max(len(r) for r in self.rows)
        # Proportional widths: first column narrower, the last (usually prose)
        # widest. p{} columns so long cells wrap instead of overflowing.
        if cols == 2:
            spec = "p{0.30\\linewidth}p{0.62\\linewidth}"
        elif cols == 3:
            spec = "p{0.22\\linewidth}p{0.16\\linewidth}p{0.54\\linewidth}"
        elif cols == 4:
            spec = ("p{0.16\\linewidth}p{0.14\\linewidth}p{0.30\\linewidth}"
                    "p{0.30\\linewidth}")
        else:
            spec = ("p{0.14\\linewidth}p{0.13\\linewidth}p{0.24\\linewidth}"
                    "p{0.24\\linewidth}p{0.10\\linewidth}")

        lines = ["\n{\\small\\begin{longtable}{" + spec + "}", "\\toprule"]
        head, *body = self.rows
        lines.append(" & ".join(f"\\textbf{{{c}}}" for c in _pad(head, cols)) + " \\\\")
        lines.append("\\midrule\n\\endhead")
        for r in body:
            lines.append(" & ".join(_pad(r, cols)) + " \\\\")
        lines.append("\\bottomrule")
        lines.append("\\end{longtable}}\n")
        return "\n".join(lines)


def _pad(row, n):
    return list(row) + [""] * (n - len(row))


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
