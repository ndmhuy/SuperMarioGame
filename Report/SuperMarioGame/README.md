# Super Mario Game — Final Report (PDF edition)

> CS202 Final Project · Group 52 · HCMUS report template.

**The prose in this report is generated, not authored here.**

The single source is [`reports/report_content.py`](../../reports/report_content.py) at the repository
root, which also produces the HTML edition. `reports/html_to_latex.py` converts it, and
`reports/build_report.py` writes `content/report_body.tex` along with the figures — screenshots copied
into `img/`, and the UML diagrams re-rendered as vector PDFs straight from the headers by
[`gen_class_diagram.py`](../../SuperMarioGame/tools/gen_class_diagram.py).

Do not edit `content/report_body.tex`. It is overwritten on every build, and hand-editing it would put
the PDF and the HTML out of step — which is the failure §10.4 of the report itself is about.

This replaces an earlier skeleton that listed eleven content files (`00_summary.tex` …
`10_appendix.tex`); none of them were ever written.

## Layout

| Path | What it is |
| :--- | :--- |
| `main.tex` | Document setup: HCMUS template, course metadata, `\codebreak`. **Hand-authored.** |
| `content/title.tex` | Cover page. **Hand-authored.** |
| `content/report_body.tex` | The report. **Generated.** |
| `img/hcmus-logo.png` | Committed. Every other file in `img/` is **generated and gitignored**. |
| `hcmus-report-template.sty` | The faculty template, unmodified. |
| `build.sh` | Regenerate, compile twice, name the output. |

## Build

```bash
./build.sh
```

Needs a TeX distribution carrying the template's packages (TeX Live works), `rsvg-convert` for the UML
diagrams, and Python 3.

To change the report, edit `reports/report_content.py` and run `build.sh` — both editions update
together.
