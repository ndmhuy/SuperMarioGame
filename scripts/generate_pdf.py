#!/usr/bin/env python3
"""generate_pdf.py -- Convert Markdown files to styled, publication-ready PDFs.

Uses Chromium (Microsoft Edge / Google Chrome headless) or WeasyPrint to
produce clean, non-overflowing A4 PDFs.
"""
import os
import sys
import markdown
import subprocess
from pathlib import Path

def find_pdf_renderer():
    """Find a supported browser or tool to print HTML to PDF."""
    candidates = [
        r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe",
        r"C:\Program Files\Microsoft\Edge\Application\msedge.exe",
        r"C:\Program Files\Google\Chrome\Application\chrome.exe",
        r"C:\Program Files (x86)\Google\Chrome\Application\chrome.exe",
        "msedge",
        "chrome",
        "chromium",
        "weasyprint"
    ]
    for c in candidates:
        if os.path.isabs(c) and os.path.exists(c):
            return c
        if not os.path.isabs(c):
            which = subprocess.run(["where.exe" if os.name == "nt" else "which", c],
                                   capture_output=True, text=True)
            if which.returncode == 0 and which.stdout.strip():
                return which.stdout.strip().splitlines()[0]
    return None

def compile_markdown_to_pdf(md_path, pdf_path):
    md_path = Path(md_path).resolve()
    pdf_path = Path(pdf_path).resolve()

    if not md_path.exists():
        print(f"Error: {md_path} does not exist.")
        return False

    with open(md_path, "r", encoding="utf-8") as f:
        md_content = f.read()

    # Convert markdown to HTML with common extensions
    html_body = markdown.markdown(md_content, extensions=['tables', 'fenced_code', 'toc', 'nl2br'])

    # CSS designed for publication quality with zero table overflow on A4
    css = """
    @page {
        size: A4 portrait;
        margin: 1.2cm 1.2cm 1.5cm 1.2cm;
    }

    * {
        box-sizing: border-box;
    }

    body {
        font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
        color: #2D3748;
        line-height: 1.5;
        font-size: 9.5pt;
        margin: 0;
        padding: 0;
        width: 100%;
        max-width: 100%;
        word-wrap: break-word;
        overflow-wrap: break-word;
    }

    header-bar {
        display: flex;
        justify-content: space-between;
        font-size: 8pt;
        color: #718096;
        border-bottom: 1px solid #E2E8F0;
        padding-bottom: 4px;
        margin-bottom: 16px;
    }

    h1 {
        font-size: 18pt;
        color: #1A365D;
        margin-top: 0;
        margin-bottom: 12px;
        padding-bottom: 6px;
        border-bottom: 2px solid #3182CE;
    }

    h2 {
        font-size: 13pt;
        color: #2B6CB0;
        margin-top: 20px;
        margin-bottom: 10px;
        padding-bottom: 4px;
        border-bottom: 1px solid #E2E8F0;
        page-break-after: avoid;
    }

    h3 {
        font-size: 11pt;
        color: #2D3748;
        margin-top: 16px;
        margin-bottom: 8px;
        page-break-after: avoid;
    }

    h4 {
        font-size: 10pt;
        color: #4A5568;
        margin-top: 12px;
        margin-bottom: 4px;
        font-style: italic;
        page-break-after: avoid;
    }

    p {
        margin-top: 0;
        margin-bottom: 8px;
    }

    ul, ol {
        margin-top: 4px;
        margin-bottom: 10px;
        padding-left: 18px;
    }

    li {
        margin-bottom: 4px;
    }

    code {
        background-color: #EDF2F7;
        padding: 1px 4px;
        border-radius: 3px;
        font-family: "Cascadia Code", Consolas, "Courier New", monospace;
        font-size: 8.5pt;
        color: #805AD5;
        word-break: break-all;
    }

    pre {
        background-color: #1A202C;
        color: #E2E8F0;
        padding: 10px;
        border-radius: 4px;
        overflow-x: auto;
        font-family: "Cascadia Code", Consolas, "Courier New", monospace;
        font-size: 8pt;
        margin-bottom: 12px;
        white-space: pre-wrap;
        word-break: break-all;
    }

    blockquote {
        border-left: 3px solid #3182CE;
        background-color: #EBF8FF;
        margin: 10px 0;
        padding: 8px 12px;
        color: #2C5282;
        font-size: 8.5pt;
    }

    table {
        width: 100% !important;
        max-width: 100% !important;
        border-collapse: collapse;
        margin-top: 8px;
        margin-bottom: 16px;
        font-size: 8pt;
        table-layout: fixed;
        word-wrap: break-word;
        overflow-wrap: break-word;
    }

    tr {
        page-break-inside: avoid;
    }

    th {
        background-color: #2B6CB0;
        color: white;
        text-align: left;
        padding: 5px 6px;
        font-weight: 600;
        font-size: 8pt;
        border: 1px solid #2B6CB0;
        overflow-wrap: break-word;
        word-break: break-word;
    }

    td {
        padding: 4px 6px;
        border: 1px solid #E2E8F0;
        vertical-align: top;
        font-size: 8pt;
        line-height: 1.35;
        overflow-wrap: break-word;
        word-break: break-word;
    }

    tr:nth-child(even) td {
        background-color: #F7FAFC;
    }

    /* Column-specific formatting for 11-column Summary Table */
    table:has(th:nth-child(11)) th,
    table:has(th:nth-child(11)) td {
        font-size: 7.5pt;
        padding: 4px 3px;
        text-align: center;
    }
    table:has(th:nth-child(11)) th:nth-child(3),
    table:has(th:nth-child(11)) td:nth-child(3) {
        text-align: left;
    }
    table:has(th:nth-child(11)) col:nth-child(1) { width: 4%; }
    table:has(th:nth-child(11)) col:nth-child(2) { width: 11%; }
    table:has(th:nth-child(11)) col:nth-child(3) { width: 17%; }
    table:has(th:nth-child(11)) col:nth-child(4) { width: 7%; }
    table:has(th:nth-child(11)) col:nth-child(5) { width: 8%; }
    table:has(th:nth-child(11)) col:nth-child(6) { width: 9%; }
    table:has(th:nth-child(11)) col:nth-child(7) { width: 8%; }
    table:has(th:nth-child(11)) col:nth-child(8) { width: 9%; }
    table:has(th:nth-child(11)) col:nth-child(9) { width: 8%; }
    table:has(th:nth-child(11)) col:nth-child(10) { width: 9%; }
    table:has(th:nth-child(11)) col:nth-child(11) { width: 10%; }

    /* Column-specific formatting for 6-column Task Breakdown Table */
    table:has(th:nth-child(6)):not(:has(th:nth-child(7))) th:nth-child(1),
    table:has(th:nth-child(6)):not(:has(th:nth-child(7))) td:nth-child(1) {
        width: 4%;
        text-align: center;
    }
    table:has(th:nth-child(6)):not(:has(th:nth-child(7))) th:nth-child(2),
    table:has(th:nth-child(6)):not(:has(th:nth-child(7))) td:nth-child(2) {
        width: 11%;
        text-align: center;
    }
    table:has(th:nth-child(6)):not(:has(th:nth-child(7))) th:nth-child(3),
    table:has(th:nth-child(6)):not(:has(th:nth-child(7))) td:nth-child(3) {
        width: 17%;
    }
    table:has(th:nth-child(6)):not(:has(th:nth-child(7))) th:nth-child(4),
    table:has(th:nth-child(6)):not(:has(th:nth-child(7))) td:nth-child(4) {
        width: 45%;
    }
    table:has(th:nth-child(6)):not(:has(th:nth-child(5):empty)):not(:has(th:nth-child(7))) th:nth-child(5),
    table:has(th:nth-child(6)):not(:has(th:nth-child(5):empty)):not(:has(th:nth-child(7))) td:nth-child(5) {
        width: 6%;
        text-align: center;
    }
    table:has(th:nth-child(6)):not(:has(th:nth-child(7))) th:nth-child(6),
    table:has(th:nth-child(6)):not(:has(th:nth-child(7))) td:nth-child(6) {
        width: 17%;
        font-size: 7.5pt;
        word-break: break-all;
    }

    a {
        color: #3182CE;
        text-decoration: none;
    }

    hr {
        border: none;
        border-top: 1px solid #E2E8F0;
        margin: 16px 0;
    }
    """

    html_content = f"""<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>{md_path.stem}</title>
    <style>
        {css}
    </style>
</head>
<body>
    <div class="header-bar" style="display: flex; justify-content: space-between; font-size: 8pt; color: #718096; border-bottom: 1px solid #E2E8F0; padding-bottom: 4px; margin-bottom: 16px;">
        <span>CS202 Final Project — Super Mario Game</span>
        <span>Group 52</span>
    </div>
    {html_body}
</body>
</html>
"""

    temp_html_path = md_path.parent / f"{md_path.stem}_render_temp.html"
    with open(temp_html_path, "w", encoding="utf-8") as f:
        f.write(html_content)

    renderer = find_pdf_renderer()
    if not renderer:
        print("Error: No PDF renderer (Edge, Chrome, or WeasyPrint) found.")
        return False

    success = False
    try:
        if "weasyprint" in renderer.lower() and not renderer.endswith(".exe"):
            subprocess.run([renderer, str(temp_html_path), str(pdf_path)], check=True)
            success = True
        else:
            # Chromium / Edge headless print-to-pdf
            cmd = [
                renderer,
                "--headless",
                "--disable-gpu",
                "--run-all-compositor-stages-before-draw",
                f"--print-to-pdf={str(pdf_path)}",
                "--no-pdf-header-footer",
                temp_html_path.as_uri()
            ]
            subprocess.run(cmd, check=True, capture_output=True)
            success = True
        print(f"Successfully generated PDF at {pdf_path}")
    except Exception as e:
        print(f"Failed to render PDF using {renderer}: {e}")
        success = False
    finally:
        if temp_html_path.exists():
            temp_html_path.unlink()

    return success

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python generate_pdf.py <input_md_file> <output_pdf_file>")
        sys.exit(1)
        
    res = compile_markdown_to_pdf(sys.argv[1], sys.argv[2])
    sys.exit(0 if res else 1)
