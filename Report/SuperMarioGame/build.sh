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

cp main.pdf Group52_SuperMarioGame_Report.pdf
echo "==> Group52_SuperMarioGame_Report.pdf ($(wc -c < main.pdf) bytes, $(pdfinfo main.pdf | awk '/^Pages/{print $2}') pages)"
