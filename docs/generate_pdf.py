import os
import sys
import markdown
import subprocess

def compile_markdown_to_pdf(md_path, pdf_path):
    if not os.path.exists(md_path):
        print(f"Error: {md_path} does not exist.")
        return False
        
    with open(md_path, "r", encoding="utf-8") as f:
        md_content = f.read()

    # Convert markdown to HTML with common extensions
    html_body = markdown.markdown(md_content, extensions=['tables', 'fenced_code', 'toc'])

    # CSS for premium report presentation
    css = """
    @page {
        size: A4;
        margin: 2.5cm;
        @top-left {
            content: "CS202 Final Project Progress Report";
            font-family: 'Helvetica Neue', Helvetica, Arial, sans-serif;
            font-size: 8.5pt;
            color: #718096;
            border-bottom: 1px solid #E2E8F0;
            padding-bottom: 3px;
        }
        @top-right {
            content: "Group 52";
            font-family: 'Helvetica Neue', Helvetica, Arial, sans-serif;
            font-size: 8.5pt;
            color: #718096;
            border-bottom: 1px solid #E2E8F0;
            padding-bottom: 3px;
        }
        @bottom-center {
            content: "Page " counter(page);
            font-family: 'Helvetica Neue', Helvetica, Arial, sans-serif;
            font-size: 9pt;
            color: #718096;
        }
    }

    body {
        font-family: 'Helvetica Neue', Helvetica, Arial, sans-serif;
        color: #2D3748;
        line-height: 1.6;
        font-size: 10.5pt;
    }

    h1 {
        font-size: 22pt;
        color: #1A365D;
        margin-top: 0;
        margin-bottom: 20px;
        padding-bottom: 10px;
        border-bottom: 2px solid #3182CE;
    }

    h2 {
        font-size: 15pt;
        color: #2B6CB0;
        margin-top: 30px;
        margin-bottom: 15px;
        padding-bottom: 5px;
        border-bottom: 1px solid #E2E8F0;
        page-break-after: avoid;
    }

    h3 {
        font-size: 12pt;
        color: #2D3748;
        margin-top: 25px;
        margin-bottom: 10px;
        page-break-after: avoid;
    }

    h4 {
        font-size: 11pt;
        color: #4A5568;
        margin-top: 15px;
        margin-bottom: 5px;
        font-style: italic;
        page-break-after: avoid;
    }

    p {
        margin-bottom: 12px;
    }

    ul, ol {
        margin-top: 5px;
        margin-bottom: 15px;
        padding-left: 20px;
    }

    li {
        margin-bottom: 6px;
    }

    code {
        font-family: Menlo, Monaco, Consolas, 'Courier New', monospace;
        font-size: 9pt;
        background-color: #F7FAFC;
        color: #C53030;
        padding: 2px 4px;
        border-radius: 3px;
        border: 1px solid #EDF2F7;
    }

    pre {
        background-color: #F7FAFC;
        border: 1px solid #E2E8F0;
        border-radius: 6px;
        padding: 12px;
        overflow-x: auto;
        margin-bottom: 15px;
        page-break-inside: avoid;
    }

    pre code {
        background-color: transparent;
        border: none;
        color: #2D3748;
        padding: 0;
        font-size: 8.5pt;
    }

    table {
        width: 100%;
        border-collapse: collapse;
        margin-top: 15px;
        margin-bottom: 20px;
        page-break-inside: avoid;
    }

    th {
        background-color: #EDF2F7;
        color: #2D3748;
        font-weight: bold;
        text-align: left;
        border: 1px solid #E2E8F0;
        padding: 8px 12px;
        font-size: 9.5pt;
    }

    td {
        border: 1px solid #E2E8F0;
        padding: 8px 12px;
        font-size: 9.5pt;
    }

    tr:nth-child(even) td {
        background-color: #F7FAFC;
    }

    hr {
        border: 0;
        height: 1px;
        background: #E2E8F0;
        margin: 25px 0;
    }

    a {
        color: #3182CE;
        text-decoration: none;
    }

    a:hover {
        text-decoration: underline;
    }
    """

    html_content = f"""<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>Weekly Report</title>
    <style>
        {css}
    </style>
</head>
<body>
    {html_body}
</body>
</html>
"""

    # Write temporary HTML file
    temp_html_path = md_path.replace(".md", "_temp.html")
    with open(temp_html_path, "w", encoding="utf-8") as f:
        f.write(html_content)

    print(f"Generated intermediate HTML at {temp_html_path}")

    # Compile using WeasyPrint
    try:
        subprocess.run(["weasyprint", temp_html_path, pdf_path], check=True)
        print(f"Successfully generated beautiful PDF at {pdf_path}")
        success = True
    except Exception as e:
        print(f"Failed to run weasyprint: {e}")
        success = False
    finally:
        if os.path.exists(temp_html_path):
            os.remove(temp_html_path)
            
    return success

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python3 generate_pdf.py <input_md_file> <output_pdf_file>")
        sys.exit(1)
        
    compile_markdown_to_pdf(sys.argv[1], sys.argv[2])
