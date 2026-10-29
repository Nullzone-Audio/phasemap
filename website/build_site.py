#!/usr/bin/env python3
"""Wraps site.html into index.html, a complete page for hosting (GitHub Pages, Netlify, any web server).

    python3 website/build_site.py
"""
from pathlib import Path
here = Path(__file__).parent
body = (here / "site.html").read_text(encoding="utf-8")

# The hosted site loads its fonts from its own server (no connection to Google, GDPR).
# The Claude preview keeps the Google Fonts link, because it cannot serve font files.
local_fonts = """<style>
@font-face { font-family: "Archivo"; src: url("fonts/archivo-variable.woff") format("woff"); font-weight: 100 900; font-stretch: 62% 125%; font-display: swap; }
@font-face { font-family: "IBM Plex Sans"; src: url("fonts/ibm-plex-sans-variable.woff") format("woff"); font-weight: 100 700; font-display: swap; }
@font-face { font-family: "IBM Plex Mono"; src: url("fonts/ibm-plex-mono-400.woff") format("woff"); font-weight: 400; font-display: swap; }
@font-face { font-family: "IBM Plex Mono"; src: url("fonts/ibm-plex-mono-500.woff") format("woff"); font-weight: 500; font-display: swap; }
</style>"""
lines = [l for l in body.splitlines() if "fonts.googleapis.com" not in l and "fonts.gstatic.com" not in l]
body = "\n".join(lines).replace("<style>", local_fonts + "\n<style>", 1)
page = ('<!doctype html>\n<html lang="en">\n<head>\n<meta charset="utf-8">\n'
        '<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">\n'
        '<link rel="icon" href="images/favicon.svg" type="image/svg+xml">\n'
        + body + "\n</html>\n")
(here / "index.html").write_text(page, encoding="utf-8")
assert "googleapis" not in page and "gstatic" not in page
print("website/index.html written (fonts self-hosted)")
