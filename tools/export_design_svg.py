#!/usr/bin/env python3
"""Exports the current Phasemap design as a layered SVG (Affinity Designer, Illustrator, Figma, Inkscape).

    python3 tools/export_design_svg.py <state.json> <out.svg>

<state.json> is written by the screenshot tool (tools/Screenshots.cpp) and contains what the meters
showed at that moment, so the moving parts are real data, as vectors.

The drawing mirrors src/ui/Background.cpp, MeterViews.cpp, LoudnessPanel.cpp and Controls.cpp.
Every visual part is its own named layer. Canvas: 1600 x 900 design pixels.
"""
import json, math, sys

FONT = "Helvetica Neue, Helvetica, Arial, sans-serif"
JUCE_TO_CSS = 0.86   # juce::Font height (ascent + descent) -> CSS font-size

TEXT, DIM, DARK, ACCENT = "#e6e6e6", "#9a9a9a", "#1e1e1e", "#f29a2e"
RED, YELLOW, GREEN = "#e0362c", "#f2c230", "#3fc24a"

defs, layers, current = [], [], None
gradient_id = [0]


def layer(name):
    global current
    current = []
    layers.append((name, current))


def emit(s):
    current.append(s)


def gradient(x1, y1, x2, y2, stops):
    gradient_id[0] += 1
    gid = f"g{gradient_id[0]}"
    s = "".join(f'<stop offset="{o}" stop-color="{c}"/>' for o, c in stops)
    defs.append(f'<linearGradient id="{gid}" gradientUnits="userSpaceOnUse" x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}">{s}</linearGradient>')
    return f"url(#{gid})"


def quad(points, fill):
    emit('<polygon points="' + " ".join(f"{x:.2f},{y:.2f}" for x, y in points) + f'" fill="{fill}"/>')


def rect(x, y, w, h, fill, opacity=1.0, rx=0, stroke=None, stroke_width=1.0):
    extra = f' rx="{rx}"' if rx else ""
    extra += f' fill-opacity="{opacity:.3f}"' if opacity < 1 else ""
    extra += f' stroke="{stroke}" stroke-width="{stroke_width}"' if stroke else ""
    emit(f'<rect x="{x:.2f}" y="{y:.2f}" width="{max(0, w):.2f}" height="{max(0, h):.2f}" fill="{fill}"{extra}/>')


def line(x1, y1, x2, y2, colour, width=1.0):
    emit(f'<line x1="{x1:.2f}" y1="{y1:.2f}" x2="{x2:.2f}" y2="{y2:.2f}" stroke="{colour}" stroke-width="{width}"/>')


def text(s, x, baseline, size, colour, anchor="start", bold=False, spacing=0.0):
    weight = ' font-weight="bold"' if bold else ""
    ls = f' letter-spacing="{spacing}"' if spacing else ""
    s = s.replace("&", "&amp;").replace("<", "&lt;")
    emit(f'<text x="{x:.2f}" y="{baseline:.2f}" font-family="{FONT}" font-size="{size:.2f}" fill="{colour}" '
         f'text-anchor="{anchor}"{weight}{ls}>{s}</text>')


def boxed_text(s, x, y, w, h, juce_height, colour, align="middle", bold=False):
    """Text centred vertically in a box, like juce::Graphics::drawText."""
    size = juce_height * JUCE_TO_CSS
    tx = {"start": x, "middle": x + w / 2, "end": x + w}[align]
    text(s, tx, y + h / 2 + size * 0.35, size, colour, align, bold)


# ------------------------------------------------------------------ building blocks (Background.cpp)
def raised_panel(x, y, w, h, b=11):
    face = "#828282"
    x0, y0, x1, y1 = x, y, x + w, y + h
    rect(x, y, w, h, face)
    quad([(x0, y0), (x1, y0), (x1 - b, y0 + b), (x0 + b, y0 + b)], gradient(0, y0, 0, y0 + b, [(0, "#868686"), (0.45, "#b5b5b5"), (1, face)]))
    quad([(x1, y0), (x1, y1), (x1 - b, y1 - b), (x1 - b, y0 + b)], gradient(x1 - b, 0, x1, 0, [(0, face), (0.55, "#b6b6b6"), (0.85, "#8c8c8c"), (1, "#3a3a3a")]))
    quad([(x0, y1), (x1, y1), (x1 - b, y1 - b), (x0 + b, y1 - b)], gradient(0, y1 - b, 0, y1, [(0, face), (0.85, "#363636"), (1, "#2e2e2e")]))
    quad([(x0, y0), (x0, y1), (x0 + b, y1 - b), (x0 + b, y0 + b)], gradient(x0, 0, x0 + b, 0, [(0, "#2e2e2e"), (0.2, "#303030"), (1, face)]))
    emit(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" fill="none" stroke="#2b2b2b" stroke-width="1.2"/>')


def inset_field(x, y, w, h, b=9, fill="#535353"):
    x0, y0, x1, y1 = x, y, x + w, y + h
    b = min(b, w / 2.2, h / 2.2)
    rect(x, y, w, h, fill)
    quad([(x0, y0), (x1, y0), (x1 - b, y0 + b), (x0 + b, y0 + b)], gradient(0, y0, 0, y0 + b, [(0, "#161616"), (0.3, "#2a2a2a"), (1, fill)]))
    quad([(x0, y0), (x0, y1), (x0 + b, y1 - b), (x0 + b, y0 + b)], gradient(x0, 0, x0 + b, 0, [(0, "#141414"), (0.3, "#2c2c2c"), (1, fill)]))
    line(x0, y1, x1, y1, "#8a8a8a", 1.2)
    line(x1, y0, x1, y1, "#7a7a7a", 1.2)


def hardware_button(x, y, w, h, label, on, icon=None):
    r = (x + 1, y + 1, w - 2, h - 2)
    rect(*r, "#a8a8a8" if on else "#868686", rx=10, stroke="#2a2a2a", stroke_width=1.6)
    emit(f'<rect x="{r[0] + 1.8}" y="{r[1] + 1.8}" width="{r[2] - 3.6}" height="{r[3] - 3.6}" rx="8.5" fill="none" stroke="#a6a6a6" stroke-opacity="0.8" stroke-width="0.9"/>')
    ax, ay, aw, ah = r[0] + 8, r[1] + 6, r[2] - 16, r[3] - 12
    if icon:
        icon_h = ah * 0.55
        icon(ax, ay, aw, icon_h)
        boxed_text(label, ax, ay + icon_h, aw, 15, 15, DARK, bold=on)
    else:
        boxed_text(label, ax, ay, aw, ah, 15, DARK, bold=on)


def icon_energy(x, y, w, h):
    cx = x + w / 2
    for i, half in enumerate((10, 18, 14, 22)):
        rect(cx - half, y + 2 + i * 6, half * (1.6 if i % 2 else 2.0), 4, "#2a2a2a")


def icon_phase(x, y, w, h):
    cx, cy = x + w / 2, y + h / 2
    for sign, opacity in ((1, 1.0), (-1, 0.5)):
        pts = [(cx - 26 + i * 52 / 30, cy + sign * 8 * math.sin(i / 30 * 2 * math.pi)) for i in range(31)]
        emit('<polyline points="' + " ".join(f"{px:.2f},{py:.2f}" for px, py in pts) +
             f'" fill="none" stroke="#2a2a2a" stroke-opacity="{opacity}" stroke-width="2"/>')


def correlation_colour(c):
    return RED if c < 0 else (YELLOW if c < 0.3 else GREEN)


def dim_colour(hex_colour, brightness, saturation=1.0):
    r, g, b = (int(hex_colour[i:i + 2], 16) / 255 for i in (1, 3, 5))
    import colorsys
    h, s, v = colorsys.rgb_to_hsv(r, g, b)
    r, g, b = colorsys.hsv_to_rgb(h, s * saturation, v * brightness)
    return "#%02x%02x%02x" % (round(r * 255), round(g * 255), round(b * 255))


def lufs_text(v):
    return f"{v:.1f}" if v > -69.95 else "--.-"


# ------------------------------------------------------------------ the design
def build(state):
    layer("Canvas")
    rect(0, 0, 1600, 900, "#555555")

    layer("Panels")
    raised_panel(46, 52, 873, 638)        # spectrum
    raised_panel(954, 52, 284, 638)       # loudness
    raised_panel(46, 720, 1192, 155)      # correlation
    raised_panel(1273, 52, 300, 823)      # right column

    layer("Displays and fields")
    inset_field(68, 70, 829, 602)
    inset_field(976, 108, 240, 26, 4)
    inset_field(976, 148, 240, 298)
    inset_field(976, 484, 240, 50, 5)
    inset_field(1100, 548, 116, 26, 4)
    inset_field(1100, 584, 116, 26, 4)
    inset_field(70, 790, 126, 26, 4)
    inset_field(112, 830, 84, 24, 4)
    inset_field(232, 772, 976, 60)
    inset_field(1295, 74, 256, 256)
    inset_field(1295, 632, 256, 32, 5)

    # spectrum geometry (Theme.h: spectrumPlot)
    px, py, pw, ph = 150, 86, 727, 550
    cx, half = px + pw / 2, pw / 2

    def y_for_hz(hz):
        return py + ph - (math.log10(hz) - math.log10(20)) / 3 * ph

    layer("Scales and grid")
    for hz, lab in [(20, "20"), (50, "50"), (100, "100"), (200, "200"), (500, "500"), (1000, "1k"),
                    (2000, "2k"), (5000, "5k"), (10000, "10k"), (20000, "20k")]:
        line(px, y_for_hz(hz), px + pw, y_for_hz(hz), "#5e5e5e")
        text(lab, px - 12, y_for_hz(hz) + 4, 11, DIM, "end")
    line(cx, py, cx, py + ph, "#7a7a7a", 1.2)

    def grid_line(x, lab):
        line(x, py, x, py + ph, "#5e5e5e")
        boxed_text(lab, x - 40, py + ph + 10, 80, 16, 11, DIM)

    if state["phaseMode"]:
        grid_line(cx, "+1")
        for side in (-1, 1):
            grid_line(cx + side * half / 2, "0")
            grid_line(cx + side * half, "-1")
        boxed_text("out of phase", px + 4, py + 2, 120, 16, 11, DIM, "start")
        boxed_text("out of phase", px + pw - 124, py + 2, 120, 16, 11, DIM, "end")
    else:
        for db in (0, -12, -24, -36, -48):
            off = (db + 60) / 60 * half
            lab = "0 dB" if db == 0 else str(db)
            grid_line(cx - off, lab)
            grid_line(cx + off, lab)
        boxed_text("L", px, py - 2, 20, 16, 13, DIM, "start", True)
        boxed_text("R", px + pw - 20, py - 2, 20, 16, 13, DIM, "end", True)

    # loudness scale between the M and S bars
    scale_x = (1062 + 1130) / 2
    for db in range(0, -49, -6):
        y = 180 + (-db) / 48 * (406 - 180)
        line(1065, y, 1073, y, "#7a7a7a")
        line(1119, y, 1127, y, "#7a7a7a")
        text(str(db), scale_x, y + 4, 11, DIM, "middle")
    text("LUFS", scale_x, 432, 10, DIM, "middle")

    # correlation scale
    bx, by, bw, bh = 232, 772, 976, 60
    for value, lab in [(-1, "-1"), (-0.5, "-0.5"), (0, "0"), (0.5, "+0.5"), (1, "+1")]:
        x = bx + 12 + (value + 1) / 2 * (bw - 24)
        line(x, by - 9, x, by - 3, "#3a3a3a", 1.6 if value == 0 else 1.0)
        text(lab, x, by - 13, 11, "#333333", "middle")

    # goniometer guides
    gx, gy, gs = 1295, 74, 256
    gcx, gcy, gr = gx + gs / 2, gy + gs / 2, gs / 2 - 14
    emit(f'<circle cx="{gcx}" cy="{gcy}" r="{gr}" fill="none" stroke="#5e5e5e" stroke-width="1"/>')
    for deg in (0, 45, 90, 135):
        a = math.radians(deg)
        line(gcx - gr * math.cos(a), gcy - gr * math.sin(a), gcx + gr * math.cos(a), gcy + gr * math.sin(a), "#5e5e5e")

    # balance scale
    for db, lab in [(-12, "L 12"), (-6, "6"), (0, "0"), (6, "6"), (12, "12 R")]:
        x = 1295 + 8 + (db + 12) / 24 * (256 - 16)
        line(x, 664 + 3, x, 664 + 9, "#3a3a3a", 1.6 if db == 0 else 1.0)
        text(lab, x, 664 + 22, 11, "#333333", "middle")

    layer("Labels")
    for s, x, y, size in [("Loudness", 976, 96, 15), ("Integrated", 976, 474, 15), ("Range", 976, 566, 13),
                          ("True Peak", 976, 602, 13), ("Correlation", 70, 774, 15), ("Hold", 70, 847, 13),
                          ("Mode", 1295, 374, 15), ("Response", 1295, 502, 15), ("Balance", 1295, 622, 15),
                          ("Check", 1295, 742, 15)]:
        text(s, x, y, size, "#2c2c2c")
    text("M", (996 + 1062) / 2, 168, 12, DIM, "middle", True)
    text("S", (1130 + 1196) / 2, 168, 12, DIM, "middle", True)
    text("M", gcx, gy + 13, 11, DIM, "middle")
    text("L", gx + 14, gy + 22, 11, DIM, "middle")
    text("R", gx + gs - 14, gy + 22, 11, DIM, "middle")
    text("+S", gx + 10, gcy - 5, 10, DIM)
    text("-S", gx + gs - 10, gcy - 5, 10, DIM, "end")
    text("out of phase", bx, by + bh + 22, 11, "#4a4a4a")
    text("mono compatible", bx + bw, by + bh + 22, 11, "#4a4a4a", "end")

    layer("Brand")
    text("PHASEMAP", 1573, 36, 22, "#3a3a3a", "end", True, 3)

    # ---------------------------------------------------------- live data
    layer("Spectrum bars (live data)")
    alarm = False
    for b in state["bands"]:
        top, bottom = y_for_hz(min(b["highHz"], 20000)), y_for_hz(b["lowHz"])
        height = max(1.5, (bottom - top) - (1.5 if bottom - top > 5 else 0.5))
        y = top + ((bottom - top) - height) / 2
        l_db, r_db, c = b["levelLeftDb"], b["levelRightDb"], b["correlation"]
        if not state["phaseMode"]:
            length = lambda db: min(1, max(0, (db + 60) / 60)) * half
            rect(cx - length(l_db), y, length(l_db), height, ACCENT, 1.0 if l_db > r_db + 3 else 0.7)
            rect(cx, y, length(r_db), height, ACCENT, 1.0 if r_db > l_db + 3 else 0.7)
        elif l_db > -72 or r_db > -72:
            w = min(1, max(0, (1 - c) / 2)) * half
            rect(cx - w, y, 2 * w, height, correlation_colour(c), 0.9)
            if b["centreHz"] < 160 and c < 0 and (l_db > -60 or r_db > -60):
                alarm = True

    if state["phaseMode"]:
        layer("Low End lamp")
        bxl, byl = px + pw - 112, py + ph - 28
        rect(bxl, byl, 106, 22, "#2a2a2a", 0.85, rx=4)
        if alarm:
            emit(f'<circle cx="{bxl + 13}" cy="{byl + 11}" r="9" fill="{RED}" fill-opacity="0.35"/>')
        emit(f'<circle cx="{bxl + 13}" cy="{byl + 11}" r="5" fill="{RED if alarm else dim_colour(RED, 0.3)}"/>')
        boxed_text("Low End", bxl + 26, byl, 80, 22, 12, TEXT if alarm else DIM, "start", True)

    layer("Loudness meter (live data)")
    ox, oy = 976, 108
    goal = state["targetLufs"]
    target_name = state["targetName"]
    whole = abs(goal - round(goal)) < 0.05
    boxed_text(f"{target_name}  ·  {goal:.0f} LUFS" if whole else f"{target_name}  ·  {goal:.1f} LUFS",
               ox, oy, 240, 26, 12.5, TEXT)
    top, bottom = oy + 72, oy + 298
    seg_h = (bottom - top) / 24
    for bar_x0, bar_x1, value in ((20, 86, state["momentary"]), (154, 220, state["shortTerm"])):
        for i in range(24):
            lo, hi = -48 + 2 * i, -46 + 2 * i
            col = GREEN if hi <= goal else (YELLOW if lo < goal + 3 else RED)
            col = col if value > lo else dim_colour(col, 0.3, 0.6)
            rect(ox + bar_x0, bottom - (i + 1) * seg_h + 0.75, bar_x1 - bar_x0, seg_h - 1.5, col)
    ty = bottom - (min(0, max(-48, goal)) + 48) / 48 * (bottom - top)
    rect(ox + 16, ty - 1, 74, 2, ACCENT)
    rect(ox + 150, ty - 1, 74, 2, ACCENT)
    quad([(ox + 92, ty - 5), (ox + 92, ty + 5), (ox + 99, ty)], ACCENT)
    quad([(ox + 148, ty - 5), (ox + 148, ty + 5), (ox + 141, ty)], ACCENT)
    boxed_text(lufs_text(state["momentary"]), ox + 14, oy + 304, 78, 18, 12, TEXT)
    boxed_text(lufs_text(state["shortTerm"]), ox + 148, oy + 304, 78, 18, 12, TEXT)

    integrated = state["integrated"]
    measured = integrated > -69.95
    distance = integrated - goal
    colour = TEXT
    if measured:
        colour = GREEN if abs(distance) <= state["targetTolerance"] else (RED if distance > 0 else YELLOW)
    boxed_text(lufs_text(integrated), ox, oy + 376, 148, 50, 30, colour, "end")
    boxed_text("LUFS", ox + 156, oy + 382, 80, 18, 12.5, DIM, "start")
    if measured:
        boxed_text(f"{distance:+.1f} LU", ox + 156, oy + 401, 80, 18, 12.5, colour, "start")
    boxed_text(f"{state['loudnessRange']:.1f} LU" if measured else "--", ox + 124, oy + 440, 116, 26, 12.5, TEXT)
    tp = state["truePeak"]
    boxed_text(f"{tp:.1f} dBTP" if tp > -100 else "--", ox + 124, oy + 476, 116, 26, 12.5,
               RED if tp > state["targetTruePeak"] else TEXT)

    layer("Correlation LEDs (live data)")
    ax, ay, aw, ah = bx + 12, by + 10, bw - 24, bh - 20
    shown, active = state["correlationShown"], state["hasSignal"]
    led_w = aw / 40
    for i in range(40):
        lo = -1 + 2 * i / 40
        hi, mid = lo + 2 / 40, lo + 1 / 40
        lit = active and ((mid >= 0 and lo < shown) if shown >= 0 else (mid < 0 and hi > shown))
        col = correlation_colour(mid)
        rect(ax + i * led_w + 1.5, ay, led_w - 3, ah, col if lit else dim_colour(col, 0.35, 0.7), rx=1.5)
    lowest = state["correlationLowest"]
    if active and lowest < 0.99:
        rect(ax + (lowest + 1) / 2 * aw - 1.5, ay - 4, 3, ah + 8, TEXT)
    boxed_text((("+" if shown >= 0.005 else "") + f"{shown:.2f}") if active else "--", 70, 790, 126, 26, 15,
               RED if active and shown < 0 else TEXT)
    boxed_text(f"{round(state['holdMs'])} ms", 114, 830, 82, 24, 12.5, TEXT)

    layer("Goniometer and balance (live data)")
    left, right = state["scopeLeft"], state["scopeRight"]
    peak = max([1e-6] + [max(abs(l + r), abs(l - r)) for l, r in zip(left, right)])
    zoom = min(200, max(0.5, 0.9 / (peak * 0.7071)))
    n = len(left)
    for k, (l, r) in enumerate(zip(left, right)):
        m, s = (l + r) * 0.7071, (r - l) * 0.7071
        x, y = gcx + s * zoom * gr, gcy - m * zoom * gr
        if gx + 3 < x < gx + gs - 3 and gy + 3 < y < gy + gs - 3:
            rect(x - 0.9, y - 0.9, 1.8, 1.8, ACCENT, 0.15 + 0.6 * k / n)
    if state["hasSignal"]:
        area_x, area_w = 1295 + 8, 256 - 16
        bcx = area_x + area_w / 2
        bal = min(12, max(-12, state["balanceDb"]))
        x = bcx + bal / 12 * area_w / 2
        rect(min(bcx, x), 632 + 7, abs(x - bcx), 32 - 14, ACCENT, 0.85)
        rect(x - 1.5, 632 + 5, 3, 32 - 10, TEXT)

    layer("Buttons")
    phase = state["phaseMode"]
    hardware_button(1295, 384, 124, 64, "Energy", not phase, icon_energy)
    hardware_button(1427, 384, 124, 64, "Phase", phase, icon_phase)
    hardware_button(1295, 512, 124, 54, "Fast", not state["slow"])
    hardware_button(1427, 512, 124, 54, "Slow", state["slow"])
    hardware_button(1295, 752, 124, 64, "Mono", state["mono"])
    hardware_button(1427, 752, 124, 64, "Freeze", state["frozen"])
    hardware_button(976, 624, 240, 42, "Reset", False)


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    build(json.load(open(sys.argv[1])))
    body = []
    for i, (name, items) in enumerate(layers):
        body.append(f'<g id="{name.replace(" ", "_")}" inkscape:groupmode="layer" inkscape:label="{name}">')
        body.extend(items)
        body.append("</g>")
    svg = ('<?xml version="1.0" encoding="UTF-8"?>\n'
           '<svg xmlns="http://www.w3.org/2000/svg" xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape" '
           'width="1600" height="900" viewBox="0 0 1600 900">\n'
           f'<defs>{"".join(defs)}</defs>\n' + "\n".join(body) + "\n</svg>\n")
    open(sys.argv[2], "w", encoding="utf-8").write(svg)
    print(f"{sys.argv[2]}: {len(layers)} layers, {len(svg) // 1024} KB")


if __name__ == "__main__":
    main()
