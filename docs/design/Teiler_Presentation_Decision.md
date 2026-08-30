# Teiler — presentation decision

**Status: Tech Aim does not present Teiler. The measurement is retained.**
Recorded 2026-08-30. Accepted as `UI-DEC-018`.

## What Teiler is

A German shooting figure for how far a shot landed from the centre of the
target, expressed in hundredths of a millimetre — lower is better. In this
codebase it is computed by `TachusWidget::getTeiler()` as the **mean radial
distance** of the shots in a series (or of the whole match, for `series = -1`):

```
teiler = mean over shots of sqrt(x² + y²) × 100
```

with `getTeilerForShoot()` giving the same figure for one shot.

It is a property of coordinates the application already measures. It is not a
score, it does not enter a score, and nothing about scoring, MPI, grouping or
acquisition depends on whether it is printed.

## The decision

| Edition | Teiler |
|---|---|
| **Tech Aim** | **NOT PRESENTED.** Absent from View Report, PDF, printed result, Coach Report, Qualification report, Final report, Open Practice and session summaries. |
| **SETA / a German-market edition** | **UNDECIDED — a future decision for Phase B.** Not enabled, not designed, not promised. |

Tech Aim reports are ISSF-shaped. Teiler is not an ISSF concept and appears in
no ISSF result; it was inherited from the origin of this codebase rather than
chosen. The four places that displayed it now do not:

| Where | Was | Now |
|---|---|---|
| `SummaryReportView.qml` | executive metric card | removed; the grid reflows |
| `MatchReportView.qml` | executive metric card | removed; the grid reflows |
| `MatchReportInfo.qml` | series header label + value | removed with its spacer |
| `MatchReportInfo.qml` | per-shot table column | removed; the remaining five columns re-proportioned to 12 / 24 / 21 / 21 / 22 so the table still fills its width |

**Nothing was renamed to hide it, and no blank heading, empty column or orphan
spacing was left behind.** `tests/qml` asserts that the header and data rows
still sum to 100% and still line up with each other.

## What was deliberately NOT done

- **The measurement was not deleted.** `getTeiler()`, `getTeilerForShoot()` and
  `getTeilerForShootOfMatch()` are untouched in `ModReader/forms/tachuswidget.cpp`.
  Removing a label must never move a number.
- **No scoring, coordinate, MPI, grouping or acquisition code was touched.**
- **The flag was not wired into a hidden column.** Tech Aim's views do not
  contain the field at all, so there is nothing to reveal and nothing to
  mis-lay-out.

## Where the decision lives

`ta::app::BrandPackage::showTeilerMetric` — `false` for `TECH_AIM`, and
deliberately also `false` for the reserved `SETA_OEM` package, because no SETA
requirement for it has been stated. `tests/reliability/tst_brandpackage.cpp`
asserts both, asserts the measurement is still present in `TachusWidget`, and
asserts that no brand flag has reached the measurement.

This follows `UI-DEC-010`: an edition difference is a value in a brand package,
not a fork of the source.

## One occurrence remains in the source, and it cannot render

`TachusWidget::getPDFString()` builds a text table whose header string contains
`Teiler`. That function is called from exactly one place —
`CustomPrint::createSummryPdf()` — **after an unconditional `painter.end();
return;`**. The Match Summary PDF has been the grabbed A4 page since the report
system redesign, and the legacy text dump below that return is dead code.

It is left untouched on purpose: `tachuswidget.cpp` is inside the acquisition
freeze for this release, and editing dead code there buys nothing and risks
something. It is recorded here so that anyone who ever revives that code path
knows it would print a figure Tech Aim has decided not to show.

## What a future SETA edition would have to decide

1. Whether SETA actually requires the figure at all, and in which reports.
2. Which column it occupies and what the other columns give up — the shot table
   is already full at five columns.
3. Whether it is per-shot, per-series, per-match, or all three.
4. Whether the label stays German in a non-German locale.

None of these has an answer today. Until they do, `showTeilerMetric` stays
false everywhere, and **no report may reintroduce the figure without a new
decision entry superseding `UI-DEC-018`.**
