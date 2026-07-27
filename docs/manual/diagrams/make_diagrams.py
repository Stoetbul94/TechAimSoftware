# -*- coding: utf-8 -*-
"""Generate the Tech Aim instructional diagrams as SVG (P0.1 workstream 4).

SVG rather than Mermaid: the manual pipeline is Pandoc -> HTML -> headless
Chromium, which does not run JavaScript at print time, so a Mermaid fenced
block would render as raw code in the PDF. Hand-generated SVG embeds directly
via --embed-resources, prints in grayscale, and needs no extra toolchain.

This script is the SOURCE. Run it to regenerate every diagram:

    python docs/manual/diagrams/make_diagrams.py

Style: Tech Aim red #C40046 for emphasis, dark ink for text, light fills so
the diagrams stay legible when printed in grayscale.
"""
import io
import os

OUT = os.path.dirname(os.path.abspath(__file__))

RED   = "#C40046"
INK   = "#14171C"
SUB   = "#5C636E"
LINE  = "#B9BFC7"
FILL  = "#F4F5F7"
FILLE = "#FDE7EE"   # emphasis fill (tinted red, still light in grayscale)
WARN  = "#E8A13C"

FONT = "Segoe UI, DejaVu Sans, Arial, sans-serif"


def esc(s):
    return (s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;"))


def box(x, y, w, h, lines, fill=FILL, stroke=LINE, sw=1, rx=6, bold_first=True):
    out = ['<rect x="%d" y="%d" width="%d" height="%d" rx="%d" fill="%s" '
           'stroke="%s" stroke-width="%s"/>' % (x, y, w, h, rx, fill, stroke, sw)]
    n = len(lines)
    total = n * 14
    start = y + h / 2 - total / 2 + 11
    for i, ln in enumerate(lines):
        weight = "600" if (i == 0 and bold_first) else "400"
        size = 11 if (i == 0 and bold_first) else 10
        col = INK if i == 0 else SUB
        out.append('<text x="%d" y="%.0f" font-family="%s" font-size="%d" '
                   'font-weight="%s" fill="%s" text-anchor="middle">%s</text>'
                   % (x + w / 2, start + i * 14, FONT, size, weight, col, esc(ln)))
    return "".join(out)


def arrow(x1, y1, x2, y2, label=None, dashed=False, colour=SUB):
    dash = ' stroke-dasharray="5,4"' if dashed else ""
    s = ('<line x1="%d" y1="%d" x2="%d" y2="%d" stroke="%s" stroke-width="1.6" '
         'marker-end="url(#a)"%s/>' % (x1, y1, x2, y2, colour, dash))
    if label:
        mx, my = (x1 + x2) / 2, (y1 + y2) / 2
        dy = -6 if y1 == y2 else 0
        s += ('<text x="%.0f" y="%.0f" font-family="%s" font-size="9" fill="%s" '
              'text-anchor="middle">%s</text>' % (mx, my + dy, FONT, SUB, esc(label)))
    return s


def svg(title, w, h, body):
    return (
        '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 %d %d" width="%d" '
        'height="%d" role="img" aria-label="%s">\n'
        '<title>%s</title>\n'
        '<defs><marker id="a" viewBox="0 0 10 10" refX="9" refY="5" '
        'markerWidth="7" markerHeight="7" orient="auto-start-reverse">'
        '<path d="M0,0 L10,5 L0,10 z" fill="%s"/></marker></defs>\n'
        '<rect width="%d" height="%d" fill="white"/>\n'
        '<text x="14" y="22" font-family="%s" font-size="12" font-weight="700" '
        'fill="%s">%s</text>\n'
        '<line x1="14" y1="30" x2="%d" y2="30" stroke="%s" stroke-width="2"/>\n'
        '%s\n</svg>\n' % (w, h, w, h, esc(title), esc(title), SUB, w, h, FONT,
                          INK, esc(title), w - 14, RED, body))


def write(name, content):
    p = os.path.join(OUT, name)
    io.open(p, "w", encoding="utf-8", newline="\n").write(content)
    print("wrote", name)


# DG-01 application workflow
b = []
xs = [14, 152, 290, 428, 566]
labs = [["Launch", "TechAim.exe"], ["Athlete", "select or enter"],
        ["Discipline", "10m AR/AP, 50m"], ["Event or", "Training programme"],
        ["Session", "sighters + counted"]]
for i, (x, l) in enumerate(zip(xs, labs)):
    b.append(box(x, 55, 124, 52, l, fill=FILLE if i == 0 else FILL))
    if i:
        b.append(arrow(x - 14, 81, x - 2, 81))
b.append(box(290, 140, 124, 52, ["Results", "summary on screen"]))
b.append(arrow(628, 107, 414, 140))
b.append(box(152, 140, 124, 52, ["Report / PDF", "EXPORT PDF"]))
b.append(arrow(290, 166, 278, 166))
b.append(box(14, 140, 124, 52, ["Home", "clean close"], fill=FILLE))
b.append(arrow(152, 166, 140, 166))
write("DG-01_application_workflow.svg",
      svg("DG-01  Application workflow", 704, 210, "".join(b)))

# DG-02 live vs demo
b = [box(240, 50, 220, 44, ["Shot arrives"], fill=FILLE),
     box(40, 130, 250, 76, ["LIVE TARGET",
                            "physical target input accepted",
                            "simulated input REJECTED"]),
     box(414, 130, 250, 76, ["DEMO / SIMULATION",
                             "generated input accepted",
                             "physical input REJECTED"]),
     arrow(300, 94, 165, 130, "operating mode = Live"),
     arrow(400, 94, 539, 130, "operating mode = Demo"),
     box(40, 232, 624, 44,
         ["The mode gate exists so a demonstration can never be presented as a real result"],
         fill="white", stroke=RED, sw=1.5)]
write("DG-02_live_vs_demo.svg",
      svg("DG-02  Live versus Demo", 704, 292, "".join(b)))

# DG-03 session lifecycle
b = []
xs = [14, 186, 358, 530]
labs = [["NEW", "session identity created"], ["ACTIVE", "shots recorded"],
        ["COMPLETED", "course finished"], ["CLOSED", "durably closed, UI reset"]]
for i, (x, l) in enumerate(zip(xs, labs)):
    b.append(box(x, 55, 158, 56, l, fill=FILLE if i == 3 else FILL))
    if i:
        b.append(arrow(x - 14, 83, x - 2, 83))
b.append(arrow(609, 111, 609, 140))
b.append(box(530, 140, 158, 44, ["Home", "or NEW SESSION"], fill=FILLE))
b.append(box(14, 140, 490, 44,
             ["Force-closing instead leaves the session UNFINISHED -> offered for recovery"],
             fill="white", stroke=WARN, sw=1.5))
write("DG-03_session_lifecycle.svg",
      svg("DG-03  Session lifecycle", 704, 200, "".join(b)))

# DG-04 recovery lifecycle
b = [box(14, 55, 180, 56, ["Interruption", "crash / power / kill"], fill=FILLE),
     arrow(194, 83, 220, 83),
     box(220, 55, 180, 56, ["Restart", "TechAim.exe"]),
     arrow(400, 83, 426, 83),
     box(426, 55, 262, 56, ["Unfinished session found?",
                            "completed sessions are NEVER offered"]),
     arrow(490, 111, 490, 140, "yes"),
     box(370, 140, 150, 50, ["Resume", "Match / Training"], fill=FILLE),
     arrow(620, 111, 620, 140, "discard"),
     box(546, 140, 142, 50, ["Discard"]),
     box(14, 140, 340, 50,
         ["If the record cannot be validated it is REPORTED,",
          "never silently loaded. Preserve it and escalate."],
         fill="white", stroke=RED, sw=1.5, bold_first=False)]
write("DG-04_recovery_lifecycle.svg",
      svg("DG-04  Recovery lifecycle", 704, 206, "".join(b)))

# DG-05 training lab selection
b = [box(250, 48, 204, 44, ["Discipline selected"], fill=FILLE),
     arrow(352, 92, 352, 116),
     box(14, 116, 200, 62, ["Technical Blocks", "supported disciplines"]),
     box(232, 116, 200, 62, ["Call & Diagnose", "supported disciplines"]),
     box(450, 116, 238, 62, ["Position Transition",
                             "50 m Rifle 3 Positions ONLY"], fill=FILLE),
     arrow(300, 92, 114, 116), arrow(352, 92, 332, 116),
     arrow(404, 92, 569, 116),
     box(14, 200, 674, 44,
         ["Group Pattern Coach is an ANALYSIS LAYER inside these programmes, not a separate choice"],
         fill="white", stroke=RED, sw=1.5)]
write("DG-05_training_lab_selection.svg",
      svg("DG-05  Training Lab programme selection", 704, 260, "".join(b)))

# DG-06 technical blocks
b = []
xs = [14, 152, 290, 428, 566]
labs = [["Configure", "focus + visibility"], ["Sighters", "excluded"],
        ["START BLOCK", "right panel"], ["Shot 0 of N", "counted only"],
        ["Block Review", "measured result"]]
for i, (x, l) in enumerate(zip(xs, labs)):
    b.append(box(x, 55, 124, 52, l, fill=FILLE if i == 2 else FILL))
    if i:
        b.append(arrow(x - 14, 81, x - 2, 81))
b.append(arrow(628, 107, 628, 136))
b.append(box(490, 136, 200, 46, ["CONTINUE TO BLOCK n", "or End Training"], fill=FILLE))
b.append(box(14, 136, 460, 46,
             ["Cadence = interval BETWEEN consecutive counted shots (not a timestamp)"],
             fill="white", stroke=RED, sw=1.5))
write("DG-06_technical_blocks.svg",
      svg("DG-06  Technical Blocks workflow", 704, 198, "".join(b)))

# DG-07 call & diagnose
b = []
xs = [14, 152, 290, 428, 566]
labs = [["Fire shot", ""], ["Actual HIDDEN", "not shown"], ["Mark the call", "athlete"],
        ["CONFIRM CALL", ""], ["Reveal", "call vs actual"]]
for i, (x, l) in enumerate(zip(xs, labs)):
    b.append(box(x, 55, 124, 52, [t for t in l if t],
                 fill=FILLE if i in (1, 3) else FILL))
    if i:
        b.append(arrow(x - 14, 81, x - 2, 81))
b.append(arrow(628, 107, 628, 136))
b.append(box(490, 136, 200, 46, ["CONTINUE TO NEXT SHOT"], fill=FILLE))
b.append(box(14, 136, 460, 46,
             ["ONE unresolved shot at a time - the next shot is refused until the call is confirmed"],
             fill="white", stroke=RED, sw=1.5))
b.append(box(14, 194, 676, 40,
             ["Call bias is a PERCEPTION observation. It is not a sight-adjustment recommendation."],
             fill="white", stroke=WARN, sw=1.5))
write("DG-07_call_and_diagnose.svg",
      svg("DG-07  Call & Diagnose workflow", 704, 250, "".join(b)))

# DG-08 position transition
b = []
xs = [14, 152, 290, 428, 566]
labs = [["POSITION SETUP", "shots IGNORED"], ["POSITION READY", "timing starts"],
        ["Sighters", "optional, excluded"], ["START VERIFICATION", "Shot 0 of N"],
        ["Position Review", "measured result"]]
for i, (x, l) in enumerate(zip(xs, labs)):
    b.append(box(x, 55, 124, 56, l, fill=FILLE if i in (0, 1) else FILL))
    if i:
        b.append(arrow(x - 14, 83, x - 2, 83))
b.append(arrow(628, 111, 628, 140))
b.append(box(536, 140, 152, 44, ["More positions?"]))
# yes -> next position (loops back to POSITION SETUP)
b.append(arrow(612, 184, 612, 212, "yes"))
b.append(box(470, 212, 218, 44, ["BEGIN TRANSITION TO ...", "back to POSITION SETUP"], fill=FILLE))
b.append(arrow(470, 234, 76, 234, "next position", dashed=True))
b.append(arrow(76, 234, 76, 113))
# no -> session summary
b.append(arrow(536, 162, 460, 162, "no"))
b.append(box(240, 140, 220, 44, ["Session Summary", "POSITION TRANSITION COMPLETE"]))
b.append(box(14, 268, 676, 40,
             ["Kneeling / Prone / Standing stay SEPARATE - compare a position against itself across repeats"],
             fill="white", stroke=RED, sw=1.5))
write("DG-08_position_transition.svg",
      svg("DG-08  Position Transition workflow", 704, 324, "".join(b)))

# DG-09 report / export
b = [box(14, 55, 170, 52, ["Session complete", "summary on screen"], fill=FILLE),
     arrow(184, 81, 210, 81),
     box(210, 55, 150, 52, ["EXPORT PDF"]),
     arrow(360, 81, 386, 81),
     box(386, 55, 160, 52, ["Choose destination"]),
     arrow(546, 81, 572, 81),
     box(572, 55, 116, 52, ["Branded PDF", "written"], fill=FILLE),
     box(14, 130, 674, 58,
         ["Training reports carry \"Not an official competition result\".",
          "Competition reports must NOT carry that training disclaimer."],
         fill="white", stroke=RED, sw=1.5, bold_first=False)]
write("DG-09_report_export.svg",
      svg("DG-09  Report and export workflow", 704, 204, "".join(b)))

# DG-10 troubleshooting escalation
b = [box(14, 55, 200, 50, ["Symptom observed"], fill=FILLE),
     arrow(214, 80, 240, 80),
     box(240, 55, 210, 50, ["Check the decision tree", "A / B / C in the guide"]),
     arrow(450, 80, 476, 80),
     box(476, 55, 212, 50, ["Resolved?"]),
     arrow(582, 105, 582, 134, "no"),
     box(400, 134, 288, 60, ["ESCALATE with:",
                             "version + commit, mode, discipline,",
                             "what you did / expected / observed"], fill=FILLE),
     box(14, 134, 366, 60,
         ["NEVER disable Defender, SmartScreen or the Firewall.",
          "NEVER delete session data to clear a fault - recovery needs it."],
         fill="white", stroke=RED, sw=1.5, bold_first=False)]
write("DG-10_troubleshooting_escalation.svg",
      svg("DG-10  Troubleshooting escalation", 704, 210, "".join(b)))

# DG-11 update flow (explicitly not yet available)
b = [box(14, 60, 200, 52, ["Check installed version", "Settings > ABOUT / BUILD"]),
     arrow(214, 86, 240, 86),
     box(240, 60, 200, 52, ["Obtain update", "procedure NOT YET DEFINED"], fill=FILL),
     arrow(440, 86, 466, 86),
     box(466, 60, 222, 52, ["Install", "procedure NOT YET DEFINED"]),
     box(14, 132, 674, 62,
         ["WINDOWS RC1 DEPENDENT - this flow is a PLACEHOLDER.",
          "Installation, update, rollback and uninstall do not exist yet and must not be",
          "described as available. This diagram records the gap, not a working process."],
         fill="white", stroke=WARN, sw=2, bold_first=True)]
write("DG-11_update_flow.svg",
      svg("DG-11  Update process (NOT YET AVAILABLE)", 704, 206, "".join(b)))

print("\n11 diagrams written to", OUT)
