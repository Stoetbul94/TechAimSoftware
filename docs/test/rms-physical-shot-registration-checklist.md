# Physical shot registration — range checklist

**For SETA / Arnold, with real hardware. Not performed by development.**

Everything RMS knows about which way is up comes from reading code. Four
renderers in the shared foundation agree that telemetry `y` is positive
**upwards**, and RMS follows them. **No shot has ever been fired to confirm
it.** This is the procedure that closes that gap.

It takes about fifteen minutes and one target.

---

## Why it matters

If the acquisition hardware's sign convention is the opposite of what the
software assumes, every shot appears mirrored top-to-bottom. Scores stay
correct — the node scores from radius, which is unaffected — so nothing looks
broken, and a coach reading the group would draw exactly the wrong conclusion
about elevation.

One test settles it. The fix, if needed, is one line in RMS
(`TargetSpec::yAxisUp`) and nothing in the node.

---

## Before you start

- [ ] One target station, connected and broadcasting on UDP 7755
- [ ] RMS running in **LIVE** mode (`Launch-TechAimRMS-Live.cmd`), showing the
      lane as `TARGET_CONNECTED`
- [ ] The lane open in **DISPLAYS → single target**
- [ ] Note the target standard the station reports: ____________________
- [ ] A way to record: phone camera is enough

Record the station's own reported x/y and score for each shot — they are shown
in Lane detail, and in RMS's log when the development geometry overlay is on
(`TECHAIM_RMS_GEOMETRY_OVERLAY=1`).

---

## 1. Centre

- [ ] Fire (or produce) one shot as close to centre as you can manage.

| | value |
|---|---|
| station x | __________ mm |
| station y | __________ mm |
| station score | __________ |

- [ ] RMS draws the hole at the centre of the face
- [ ] The hole is visibly the right size for the calibre — on a 10 m air rifle
      face the pellet should look **large**, roughly a seventh of the face
      radius. If it looks like a pinprick, something is wrong.

## 2. Right

- [ ] Produce a shot clearly to the **RIGHT** of centre, roughly half way out.

| | value |
|---|---|
| station x | __________ mm (expect **positive**) |
| station y | __________ mm |

- [ ] RMS draws it on the **RIGHT** of the face

**If x is negative for a right-hand shot, stop and report it.** That is a node
or hardware convention question, not an RMS one.

## 3. High — THE ONE THAT MATTERS

- [ ] Produce a shot clearly **HIGH**, roughly half way out.

| | value |
|---|---|
| station x | __________ mm |
| station y | __________ mm (expect **positive**) |

- [ ] RMS draws it **ABOVE** centre

| result | meaning | action |
|---|---|---|
| high shot drawn high | the assumed convention is right | tick and move on |
| high shot drawn LOW | y is inverted | report it — one-line fix in RMS |

## 4. Low

- [ ] Produce a shot clearly **LOW**.
- [ ] Station y is **negative**
- [ ] RMS draws it **BELOW** centre

## 5. A diagonal, for scale

- [ ] Produce a shot up and to the right.
- [ ] RMS draws it up and to the right
- [ ] Its distance from centre looks the same as an equally distant shot on
      either axis — a group should not look squashed in one direction

## 6. Ring agreement — the reason for the footprint

Pick a shot whose score is a whole number, or close to one.

- [ ] The **edge** of the drawn hole sits on the ring line its score names —
      **not** the centre of the hole

ISSF scores by outward gauge: a shot is a 10 when the pellet's edge reaches the
ten ring, not when its centre does. On a 10 m air rifle face a 10.0 has its
centre about 2.5 mm out, ten times the ten-ring radius. **That is correct.**

| shot | station score | does the hole EDGE meet that ring? |
|---|---|---|
| | | ☐ yes ☐ no |
| | | ☐ yes ☐ no |
| | | ☐ yes ☐ no |

## 7. Record

- [ ] Photograph the RMS screen next to the physical target
- [ ] Save the RMS window capture and the station log for the session
- [ ] Write the target standard and calibre used at the top of the notes

---

## Outcome

Tester: ________________  Date: ____________  Station: ________________

```
PHYSICAL X ORIENTATION:   ☐ CONFIRMED right-positive   ☐ INVERTED   ☐ not tested
PHYSICAL Y ORIENTATION:   ☐ CONFIRMED up-positive      ☐ INVERTED   ☐ not tested
PROJECTILE FOOTPRINT:     ☐ looks physically right     ☐ wrong size
RING AGREEMENT (edge):    ☐ agrees                     ☐ does not agree
```

Anything unexpected:

_______________________________________________________________________

_______________________________________________________________________

Until this sheet is filled in and returned, the project's status stays:

```
PHYSICAL X/Y ORIENTATION: NOT PHYSICALLY VERIFIED
```
