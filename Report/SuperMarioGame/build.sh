#!/usr/bin/env bash
# Build the PDF edition of the final report.
#
# The body is GENERATED from reports/report_content.py, which also produces the
# HTML edition — see reports/html_to_latex.py for why there is one source and
# not two manuscripts. So this regenerates before compiling: editing
# content/report_body.tex by hand would be overwritten, and would put the two
# editions out of step.
set -euo pipefail
cd "$(dirname "$0")"
REPO=$(cd ../.. && pwd)

echo "==> regenerating the body from reports/report_content.py"
python3 "$REPO/reports/build_report.py"

echo "==> pdflatex (twice, for the table of contents)"
pdflatex -interaction=nonstopmode -halt-on-error main.tex >/dev/null
pdflatex -interaction=nonstopmode -halt-on-error main.tex >/dev/null

# Guard against plan section 4.1's defect recurring: a table column or a
# \codebreak run too narrow for its content does not raise a LaTeX error, it
# just typesets text on top of the next column - the "IMovementStrate|
# section 6.5" collision the report shipped with. pdflatex DOES notice,
# though: any box it cannot fit reports an "Overfull \hbox" with the exact
# overflow in points, regardless of which page, table or section produced
# it. So this reads every such warning out of the fresh main.log (not an
# allowlist of pages known to be bad, and not text matched against a
# specific caption or table - lanes 5A-5C add new sections and new tables
# after this one runs, and the guard has to catch whatever they introduce)
# and fails the build if anything is worse than 5pt over - mutation-tested
# by lane 5D: reverting the column-width/codebreak fix makes this fail, and
# restoring it makes this pass again.
echo "==> checking main.log for Overfull \\hbox warnings over the 5pt guard (plan section 4.1)"
python3 - main.log <<'PYEOF'
import re
import sys

log = open(sys.argv[1], encoding="utf-8", errors="replace").read()
hits = [float(m) for m in re.findall(r"Overfull \\hbox \(([0-9.]+)pt too wide\)", log)]
worst = max(hits) if hits else 0.0
print(f"    Overfull \\hbox warnings: {len(hits)}"
      + (f" (worst {worst:.2f}pt)" if hits else ""))

THRESHOLD = 5.0
bad = [h for h in hits if h > THRESHOLD]
if bad:
    print(f"FAIL: {len(bad)} Overfull \\hbox warning(s) exceed the {THRESHOLD}pt "
          f"guard (worst {max(bad):.2f}pt) - a table column or \\codebreak run is "
          "too narrow for its content and text is overlapping in the PDF. See "
          "docs/issues/submission_sweep_plan_2026-09-02.md section 4.1.",
          file=sys.stderr)
    sys.exit(1)
PYEOF

cp main.pdf Group52_SuperMarioGame_Report.pdf
echo "==> Group52_SuperMarioGame_Report.pdf ($(wc -c < main.pdf) bytes, $(pdfinfo main.pdf | awk '/^Pages/{print $2}') pages)"
