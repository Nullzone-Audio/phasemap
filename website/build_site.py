#!/usr/bin/env python3
"""Wraps site.html into index.html, a complete page for hosting (GitHub Pages, Netlify, any web server).

    python3 website/build_site.py
"""
from pathlib import Path
here = Path(__file__).parent
body = (here / "site.html").read_text(encoding="utf-8")
page = ('<!doctype html>\n<html lang="en">\n<head>\n<meta charset="utf-8">\n'
        '<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">\n'
        '<link rel="icon" href="images/favicon.svg" type="image/svg+xml">\n'
        + body + "\n</html>\n")
(here / "index.html").write_text(page, encoding="utf-8")
print("website/index.html written")
