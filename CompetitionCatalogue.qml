import QtQuick 2.15

// ─────────────────────────────────────────────────────────────────────────────
// COMPETITION CATALOGUE — the seam.
//
// Programme definitions used to be 48 hardcoded ListElements inside main.qml.
// That made every programme a UI literal: nothing outside the QML file could
// name a competition, and SETA and RMS would each have had to re-describe the
// same programmes. This file is now the single description; main.qml builds
// its ListModels FROM it, in the same order, with the same values.
//
// THIS IS A SEAM, NOT A FEATURE. The entries below were generated from the
// literals they replace, so the offered programmes are byte-identical. Only
// ISSF programmes that already exist are present - no DSB, no 3x10/3x15/3x20,
// no new 50 m programmes.
//
// A profile SELECTS existing authoritative behaviour. It carries NO ring
// geometry, NO projectile diameter and NO scoring formula: those stay in
// CenterPane.qml and AppSettings, which remain the only scoring authority.
//
// TARGET STANDARD vs COMPETITION PROGRAMME - these are NOT the same thing.
//   targetStandardId  the ISSF target geometry, calibre and scoring a preset
//                     uses. Every entry here uses an ISSF standard.
//   rulesetId /       whether the PROGRAMME is an official competition course.
//   programmeType     Only the 60-shot courses are official ISSF events
//                     (10 m Air Rifle, 10 m Air Pistol, 50 m Rifle, 50 m
//                     Pistol qualification). FREE/10/15/20/30/40 are Tech Aim
//                     configurable PRESETS: they shoot on an ISSF target and
//                     score by ISSF rules, but no rule authority makes them
//                     ISSF events, so they must not claim to be. That is why
//                     their ids are techaim.* and their federation is empty.
//
// programmeId is the stable machine identity - lowercase, dotted, never
// translated, safe to put in a session file or an RMS message. The *Key
// fields hold the ENGLISH SOURCE TEXT used with qsTr() for display only.
// Per UI-DEC-015 and QML-LANG-001, no logic may ever branch on them.
// ─────────────────────────────────────────────────────────────────────────────

QtObject {
    id: catalogue

    readonly property var models: ({
        // game10RangeEventModel  (10 m)
        "game10RangeEventModel": [
            { "programmeId": "techaim.10m.air-rifle.free",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.10m.air-rifle",
              "disciplineId": "AR10", "distanceM": 10,
              "targetFamily": "AIR_RIFLE", "isPistol": false,
              "shotCount": -1, "unlimited": true,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": false,
              "nameKey": "10M AIR RIFLE FREE", "gameDisplay1Key": "10M AIR",
              "gameDisplay2Key": "RIFLE", "matchDisplayKey": "UN-LIMITED" },
            { "programmeId": "techaim.10m.air-rifle.match10",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.10m.air-rifle",
              "disciplineId": "AR10", "distanceM": 10,
              "targetFamily": "AIR_RIFLE", "isPistol": false,
              "shotCount": 10, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": false,
              "nameKey": "10M AIR RIFLE 10", "gameDisplay1Key": "10M AIR",
              "gameDisplay2Key": "RIFLE", "matchDisplayKey": "MATCH-10" },
            { "programmeId": "techaim.10m.air-rifle.match20",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.10m.air-rifle",
              "disciplineId": "AR10", "distanceM": 10,
              "targetFamily": "AIR_RIFLE", "isPistol": false,
              "shotCount": 20, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": false,
              "nameKey": "10M AIR RIFLE 20", "gameDisplay1Key": "10M AIR",
              "gameDisplay2Key": "RIFLE", "matchDisplayKey": "MATCH-20" },
            { "programmeId": "techaim.10m.air-rifle.match30",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.10m.air-rifle",
              "disciplineId": "AR10", "distanceM": 10,
              "targetFamily": "AIR_RIFLE", "isPistol": false,
              "shotCount": 30, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": false,
              "nameKey": "10M AIR RIFLE 30", "gameDisplay1Key": "10M AIR",
              "gameDisplay2Key": "RIFLE", "matchDisplayKey": "MATCH-30" },
            { "programmeId": "techaim.10m.air-rifle.match40",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.10m.air-rifle",
              "disciplineId": "AR10", "distanceM": 10,
              "targetFamily": "AIR_RIFLE", "isPistol": false,
              "shotCount": 40, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": false,
              "nameKey": "10M AIR RIFLE 40", "gameDisplay1Key": "10M AIR",
              "gameDisplay2Key": "RIFLE", "matchDisplayKey": "MATCH-40" },
            { "programmeId": "issf.10m.air-rifle.qualification60",
              "rulesetId": "issf", "federation": "ISSF", "programmeType": "OFFICIAL",
              "targetStandardId": "issf.10m.air-rifle",
              "disciplineId": "AR10", "distanceM": 10,
              "targetFamily": "AIR_RIFLE", "isPistol": false,
              "shotCount": 60, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": false,
              "nameKey": "10M AIR RIFLE 60", "gameDisplay1Key": "10M AIR",
              "gameDisplay2Key": "RIFLE", "matchDisplayKey": "MATCH-60" },
            { "programmeId": "techaim.10m.air-pistol.free",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.10m.air-pistol",
              "disciplineId": "AP10", "distanceM": 10,
              "targetFamily": "AIR_PISTOL", "isPistol": true,
              "shotCount": -1, "unlimited": true,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": false,
              "nameKey": "10M AIR PISTOL FREE", "gameDisplay1Key": "10M AIR",
              "gameDisplay2Key": "PISTOL", "matchDisplayKey": "UN-LIMITED" },
            { "programmeId": "techaim.10m.air-pistol.match10",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.10m.air-pistol",
              "disciplineId": "AP10", "distanceM": 10,
              "targetFamily": "AIR_PISTOL", "isPistol": true,
              "shotCount": 10, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": false,
              "nameKey": "10M AIR PISTOL 10", "gameDisplay1Key": "10M AIR",
              "gameDisplay2Key": "PISTOL", "matchDisplayKey": "MATCH-10" },
            { "programmeId": "techaim.10m.air-pistol.match20",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.10m.air-pistol",
              "disciplineId": "AP10", "distanceM": 10,
              "targetFamily": "AIR_PISTOL", "isPistol": true,
              "shotCount": 20, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": false,
              "nameKey": "10M AIR PISTOL 20", "gameDisplay1Key": "10M AIR",
              "gameDisplay2Key": "PISTOL", "matchDisplayKey": "MATCH-20" },
            { "programmeId": "techaim.10m.air-pistol.match30",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.10m.air-pistol",
              "disciplineId": "AP10", "distanceM": 10,
              "targetFamily": "AIR_PISTOL", "isPistol": true,
              "shotCount": 30, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": false,
              "nameKey": "10M AIR PISTOL 30", "gameDisplay1Key": "10M AIR",
              "gameDisplay2Key": "PISTOL", "matchDisplayKey": "MATCH-30" },
            { "programmeId": "techaim.10m.air-pistol.match40",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.10m.air-pistol",
              "disciplineId": "AP10", "distanceM": 10,
              "targetFamily": "AIR_PISTOL", "isPistol": true,
              "shotCount": 40, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": false,
              "nameKey": "10M AIR PISTOL 40", "gameDisplay1Key": "10M AIR",
              "gameDisplay2Key": "PISTOL", "matchDisplayKey": "MATCH-40" },
            { "programmeId": "issf.10m.air-pistol.qualification60",
              "rulesetId": "issf", "federation": "ISSF", "programmeType": "OFFICIAL",
              "targetStandardId": "issf.10m.air-pistol",
              "disciplineId": "AP10", "distanceM": 10,
              "targetFamily": "AIR_PISTOL", "isPistol": true,
              "shotCount": 60, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": false,
              "nameKey": "10M AIR PISTOL 60", "gameDisplay1Key": "10M AIR",
              "gameDisplay2Key": "PISTOL", "matchDisplayKey": "MATCH-60" }
        ],
        // game10RangeEventModel_15  (10 m, 15-shot variant)
        "game10RangeEventModel_15": [
            { "programmeId": "techaim.10m.air-rifle.free.p15",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.10m.air-rifle",
              "disciplineId": "AR10", "distanceM": 10,
              "targetFamily": "AIR_RIFLE", "isPistol": false,
              "shotCount": -1, "unlimited": true,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": true,
              "nameKey": "10M AIR RIFLE FREE", "gameDisplay1Key": "10M AIR",
              "gameDisplay2Key": "RIFLE", "matchDisplayKey": "UN-LIMITED" },
            { "programmeId": "techaim.10m.air-rifle.match10.p15",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.10m.air-rifle",
              "disciplineId": "AR10", "distanceM": 10,
              "targetFamily": "AIR_RIFLE", "isPistol": false,
              "shotCount": 10, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": true,
              "nameKey": "10M AIR RIFLE 10", "gameDisplay1Key": "10M AIR",
              "gameDisplay2Key": "RIFLE", "matchDisplayKey": "MATCH-10" },
            { "programmeId": "techaim.10m.air-rifle.match15.p15",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.10m.air-rifle",
              "disciplineId": "AR10", "distanceM": 10,
              "targetFamily": "AIR_RIFLE", "isPistol": false,
              "shotCount": 15, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": true,
              "nameKey": "10M AIR RIFLE 15", "gameDisplay1Key": "10M AIR",
              "gameDisplay2Key": "RIFLE", "matchDisplayKey": "MATCH-15" },
            { "programmeId": "techaim.10m.air-rifle.match20.p15",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.10m.air-rifle",
              "disciplineId": "AR10", "distanceM": 10,
              "targetFamily": "AIR_RIFLE", "isPistol": false,
              "shotCount": 20, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": true,
              "nameKey": "10M AIR RIFLE 20", "gameDisplay1Key": "10M AIR",
              "gameDisplay2Key": "RIFLE", "matchDisplayKey": "MATCH-20" },
            { "programmeId": "techaim.10m.air-rifle.match30.p15",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.10m.air-rifle",
              "disciplineId": "AR10", "distanceM": 10,
              "targetFamily": "AIR_RIFLE", "isPistol": false,
              "shotCount": 30, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": true,
              "nameKey": "10M AIR RIFLE 30", "gameDisplay1Key": "10M AIR",
              "gameDisplay2Key": "RIFLE", "matchDisplayKey": "MATCH-30" },
            { "programmeId": "techaim.10m.air-rifle.match40.p15",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.10m.air-rifle",
              "disciplineId": "AR10", "distanceM": 10,
              "targetFamily": "AIR_RIFLE", "isPistol": false,
              "shotCount": 40, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": true,
              "nameKey": "10M AIR RIFLE 40", "gameDisplay1Key": "10M AIR",
              "gameDisplay2Key": "RIFLE", "matchDisplayKey": "MATCH-40" },
            { "programmeId": "techaim.10m.air-pistol.free.p15",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.10m.air-pistol",
              "disciplineId": "AP10", "distanceM": 10,
              "targetFamily": "AIR_PISTOL", "isPistol": true,
              "shotCount": -1, "unlimited": true,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": true,
              "nameKey": "10M AIR PISTOL FREE", "gameDisplay1Key": "10M AIR",
              "gameDisplay2Key": "PISTOL", "matchDisplayKey": "UN-LIMITED" },
            { "programmeId": "techaim.10m.air-pistol.match10.p15",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.10m.air-pistol",
              "disciplineId": "AP10", "distanceM": 10,
              "targetFamily": "AIR_PISTOL", "isPistol": true,
              "shotCount": 10, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": true,
              "nameKey": "10M AIR PISTOL 10", "gameDisplay1Key": "10M AIR",
              "gameDisplay2Key": "PISTOL", "matchDisplayKey": "MATCH-10" },
            { "programmeId": "techaim.10m.air-pistol.match15.p15",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.10m.air-pistol",
              "disciplineId": "AP10", "distanceM": 10,
              "targetFamily": "AIR_PISTOL", "isPistol": true,
              "shotCount": 15, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": true,
              "nameKey": "10M AIR PISTOL 15", "gameDisplay1Key": "10M AIR",
              "gameDisplay2Key": "PISTOL", "matchDisplayKey": "MATCH-15" },
            { "programmeId": "techaim.10m.air-pistol.match20.p15",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.10m.air-pistol",
              "disciplineId": "AP10", "distanceM": 10,
              "targetFamily": "AIR_PISTOL", "isPistol": true,
              "shotCount": 20, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": true,
              "nameKey": "10M AIR PISTOL 20", "gameDisplay1Key": "10M AIR",
              "gameDisplay2Key": "PISTOL", "matchDisplayKey": "MATCH-20" },
            { "programmeId": "techaim.10m.air-pistol.match30.p15",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.10m.air-pistol",
              "disciplineId": "AP10", "distanceM": 10,
              "targetFamily": "AIR_PISTOL", "isPistol": true,
              "shotCount": 30, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": true,
              "nameKey": "10M AIR PISTOL 30", "gameDisplay1Key": "10M AIR",
              "gameDisplay2Key": "PISTOL", "matchDisplayKey": "MATCH-30" },
            { "programmeId": "techaim.10m.air-pistol.match40.p15",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.10m.air-pistol",
              "disciplineId": "AP10", "distanceM": 10,
              "targetFamily": "AIR_PISTOL", "isPistol": true,
              "shotCount": 40, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": true,
              "nameKey": "10M AIR PISTOL 40", "gameDisplay1Key": "10M AIR",
              "gameDisplay2Key": "PISTOL", "matchDisplayKey": "MATCH-40" }
        ],
        // game50RangeEventModel  (50 m)
        "game50RangeEventModel": [
            { "programmeId": "techaim.50m.rifle.free",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.50m.rifle",
              "disciplineId": "RIFLE50", "distanceM": 50,
              "targetFamily": "SMALLBORE_RIFLE", "isPistol": false,
              "shotCount": -1, "unlimited": true,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": false,
              "nameKey": "50 Meter RIFLE FREE", "gameDisplay1Key": "50 Meter",
              "gameDisplay2Key": "RIFLE", "matchDisplayKey": "UN-LIMITED" },
            { "programmeId": "techaim.50m.rifle.match10",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.50m.rifle",
              "disciplineId": "RIFLE50", "distanceM": 50,
              "targetFamily": "SMALLBORE_RIFLE", "isPistol": false,
              "shotCount": 10, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": false,
              "nameKey": "50 Meter RIFLE 10", "gameDisplay1Key": "50 Meter",
              "gameDisplay2Key": "RIFLE", "matchDisplayKey": "MATCH-10" },
            { "programmeId": "techaim.50m.rifle.match20",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.50m.rifle",
              "disciplineId": "RIFLE50", "distanceM": 50,
              "targetFamily": "SMALLBORE_RIFLE", "isPistol": false,
              "shotCount": 20, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": false,
              "nameKey": "50 Meter RIFLE 20", "gameDisplay1Key": "50 Meter",
              "gameDisplay2Key": "RIFLE", "matchDisplayKey": "MATCH-20" },
            { "programmeId": "techaim.50m.rifle.match30",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.50m.rifle",
              "disciplineId": "RIFLE50", "distanceM": 50,
              "targetFamily": "SMALLBORE_RIFLE", "isPistol": false,
              "shotCount": 30, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": false,
              "nameKey": "50 Meter RIFLE 30", "gameDisplay1Key": "50 Meter",
              "gameDisplay2Key": "RIFLE", "matchDisplayKey": "MATCH-30" },
            { "programmeId": "techaim.50m.rifle.match40",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.50m.rifle",
              "disciplineId": "RIFLE50", "distanceM": 50,
              "targetFamily": "SMALLBORE_RIFLE", "isPistol": false,
              "shotCount": 40, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": false,
              "nameKey": "50 Meter RIFLE 40", "gameDisplay1Key": "50 Meter",
              "gameDisplay2Key": "RIFLE", "matchDisplayKey": "MATCH-40" },
            { "programmeId": "issf.50m.rifle.qualification60",
              "rulesetId": "issf", "federation": "ISSF", "programmeType": "OFFICIAL",
              "targetStandardId": "issf.50m.rifle",
              "disciplineId": "RIFLE50", "distanceM": 50,
              "targetFamily": "SMALLBORE_RIFLE", "isPistol": false,
              "shotCount": 60, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": false,
              "nameKey": "50 Meter RIFLE 60", "gameDisplay1Key": "50 Meter",
              "gameDisplay2Key": "RIFLE", "matchDisplayKey": "MATCH-60" },
            { "programmeId": "techaim.50m.pistol.free",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.50m.pistol",
              "disciplineId": "FREEPISTOL50", "distanceM": 50,
              "targetFamily": "FREE_PISTOL", "isPistol": true,
              "shotCount": -1, "unlimited": true,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": false,
              "nameKey": "50 Meter Free PISTOL FREE", "gameDisplay1Key": "50 Meter Free",
              "gameDisplay2Key": "PISTOL", "matchDisplayKey": "UN-LIMITED" },
            { "programmeId": "techaim.50m.pistol.match10",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.50m.pistol",
              "disciplineId": "FREEPISTOL50", "distanceM": 50,
              "targetFamily": "FREE_PISTOL", "isPistol": true,
              "shotCount": 10, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": false,
              "nameKey": "50 Meter Free PISTOL 10", "gameDisplay1Key": "50 Meter Free",
              "gameDisplay2Key": "PISTOL", "matchDisplayKey": "MATCH-10" },
            { "programmeId": "techaim.50m.pistol.match20",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.50m.pistol",
              "disciplineId": "FREEPISTOL50", "distanceM": 50,
              "targetFamily": "FREE_PISTOL", "isPistol": true,
              "shotCount": 20, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": false,
              "nameKey": "50 Meter Free PISTOL 20", "gameDisplay1Key": "50 Meter Free",
              "gameDisplay2Key": "PISTOL", "matchDisplayKey": "MATCH-20" },
            { "programmeId": "techaim.50m.pistol.match30",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.50m.pistol",
              "disciplineId": "FREEPISTOL50", "distanceM": 50,
              "targetFamily": "FREE_PISTOL", "isPistol": true,
              "shotCount": 30, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": false,
              "nameKey": "50 Meter Free PISTOL 30", "gameDisplay1Key": "50 Meter Free",
              "gameDisplay2Key": "PISTOL", "matchDisplayKey": "MATCH-30" },
            { "programmeId": "techaim.50m.pistol.match40",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.50m.pistol",
              "disciplineId": "FREEPISTOL50", "distanceM": 50,
              "targetFamily": "FREE_PISTOL", "isPistol": true,
              "shotCount": 40, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": false,
              "nameKey": "50 Meter Free PISTOL 40", "gameDisplay1Key": "50 Meter Free",
              "gameDisplay2Key": "PISTOL", "matchDisplayKey": "MATCH-40" },
            { "programmeId": "issf.50m.pistol.qualification60",
              "rulesetId": "issf", "federation": "ISSF", "programmeType": "OFFICIAL",
              "targetStandardId": "issf.50m.pistol",
              "disciplineId": "FREEPISTOL50", "distanceM": 50,
              "targetFamily": "FREE_PISTOL", "isPistol": true,
              "shotCount": 60, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": false,
              "nameKey": "50 Meter Free PISTOL 60", "gameDisplay1Key": "50 Meter Free",
              "gameDisplay2Key": "PISTOL", "matchDisplayKey": "MATCH-60" }
        ],
        // game50RangeEventModel_15  (50 m, 15-shot variant)
        "game50RangeEventModel_15": [
            { "programmeId": "techaim.50m.rifle.free.p15",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.50m.rifle",
              "disciplineId": "RIFLE50", "distanceM": 50,
              "targetFamily": "SMALLBORE_RIFLE", "isPistol": false,
              "shotCount": -1, "unlimited": true,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": true,
              "nameKey": "50 Meter RIFLE FREE", "gameDisplay1Key": "50 Meter",
              "gameDisplay2Key": "RIFLE", "matchDisplayKey": "UN-LIMITED" },
            { "programmeId": "techaim.50m.rifle.match10.p15",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.50m.rifle",
              "disciplineId": "RIFLE50", "distanceM": 50,
              "targetFamily": "SMALLBORE_RIFLE", "isPistol": false,
              "shotCount": 10, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": true,
              "nameKey": "50 Meter RIFLE 10", "gameDisplay1Key": "50 Meter",
              "gameDisplay2Key": "RIFLE", "matchDisplayKey": "MATCH-10" },
            { "programmeId": "techaim.50m.rifle.match15.p15",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.50m.rifle",
              "disciplineId": "RIFLE50", "distanceM": 50,
              "targetFamily": "SMALLBORE_RIFLE", "isPistol": false,
              "shotCount": 15, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": true,
              "nameKey": "50 Meter RIFLE 15", "gameDisplay1Key": "50 Meter",
              "gameDisplay2Key": "RIFLE", "matchDisplayKey": "MATCH-15" },
            { "programmeId": "techaim.50m.rifle.match20.p15",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.50m.rifle",
              "disciplineId": "RIFLE50", "distanceM": 50,
              "targetFamily": "SMALLBORE_RIFLE", "isPistol": false,
              "shotCount": 20, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": true,
              "nameKey": "50 Meter RIFLE 20", "gameDisplay1Key": "50 Meter",
              "gameDisplay2Key": "RIFLE", "matchDisplayKey": "MATCH-20" },
            { "programmeId": "techaim.50m.rifle.match30.p15",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.50m.rifle",
              "disciplineId": "RIFLE50", "distanceM": 50,
              "targetFamily": "SMALLBORE_RIFLE", "isPistol": false,
              "shotCount": 30, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": true,
              "nameKey": "50 Meter RIFLE 30", "gameDisplay1Key": "50 Meter",
              "gameDisplay2Key": "RIFLE", "matchDisplayKey": "MATCH-30" },
            { "programmeId": "techaim.50m.rifle.match40.p15",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.50m.rifle",
              "disciplineId": "RIFLE50", "distanceM": 50,
              "targetFamily": "SMALLBORE_RIFLE", "isPistol": false,
              "shotCount": 40, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": true,
              "nameKey": "50 Meter RIFLE 40", "gameDisplay1Key": "50 Meter",
              "gameDisplay2Key": "RIFLE", "matchDisplayKey": "MATCH-40" },
            { "programmeId": "techaim.50m.pistol.free.p15",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.50m.pistol",
              "disciplineId": "FREEPISTOL50", "distanceM": 50,
              "targetFamily": "FREE_PISTOL", "isPistol": true,
              "shotCount": -1, "unlimited": true,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": true,
              "nameKey": "50 Meter Free PISTOL FREE", "gameDisplay1Key": "50 Meter Free",
              "gameDisplay2Key": "PISTOL", "matchDisplayKey": "UN-LIMITED" },
            { "programmeId": "techaim.50m.pistol.match10.p15",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.50m.pistol",
              "disciplineId": "FREEPISTOL50", "distanceM": 50,
              "targetFamily": "FREE_PISTOL", "isPistol": true,
              "shotCount": 10, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": true,
              "nameKey": "50 Meter Free PISTOL 10", "gameDisplay1Key": "50 Meter Free",
              "gameDisplay2Key": "PISTOL", "matchDisplayKey": "MATCH-10" },
            { "programmeId": "techaim.50m.pistol.match15.p15",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.50m.pistol",
              "disciplineId": "FREEPISTOL50", "distanceM": 50,
              "targetFamily": "FREE_PISTOL", "isPistol": true,
              "shotCount": 15, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": true,
              "nameKey": "50 Meter Free PISTOL 15", "gameDisplay1Key": "50 Meter Free",
              "gameDisplay2Key": "PISTOL", "matchDisplayKey": "MATCH-15" },
            { "programmeId": "techaim.50m.pistol.match20.p15",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.50m.pistol",
              "disciplineId": "FREEPISTOL50", "distanceM": 50,
              "targetFamily": "FREE_PISTOL", "isPistol": true,
              "shotCount": 20, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": true,
              "nameKey": "50 Meter Free PISTOL 20", "gameDisplay1Key": "50 Meter Free",
              "gameDisplay2Key": "PISTOL", "matchDisplayKey": "MATCH-20" },
            { "programmeId": "techaim.50m.pistol.match30.p15",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.50m.pistol",
              "disciplineId": "FREEPISTOL50", "distanceM": 50,
              "targetFamily": "FREE_PISTOL", "isPistol": true,
              "shotCount": 30, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": true,
              "nameKey": "50 Meter Free PISTOL 30", "gameDisplay1Key": "50 Meter Free",
              "gameDisplay2Key": "PISTOL", "matchDisplayKey": "MATCH-30" },
            { "programmeId": "techaim.50m.pistol.match40.p15",
              "rulesetId": "techaim", "federation": "", "programmeType": "PRESET",
              "targetStandardId": "issf.50m.pistol",
              "disciplineId": "FREEPISTOL50", "distanceM": 50,
              "targetFamily": "FREE_PISTOL", "isPistol": true,
              "shotCount": 40, "unlimited": false,
              "scoringMode": "DECIMAL_OR_INTEGER_BY_SETTING",
              "positions": [], "fifteenShotVariant": true,
              "nameKey": "50 Meter Free PISTOL 40", "gameDisplay1Key": "50 Meter Free",
              "gameDisplay2Key": "PISTOL", "matchDisplayKey": "MATCH-40" }
        ]
    })

    // The ordered entries backing one ListModel. Order is load-bearing:
    // ShootingPage resolves the user's choice by INDEX.
    function entriesFor(modelKey) {
        return models[modelKey] !== undefined ? models[modelKey] : []
    }

    // Stable identity for a selection, without consulting any display text.
    function programmeIdAt(modelKey, index) {
        var e = entriesFor(modelKey)
        return (index >= 0 && index < e.length) ? e[index].programmeId : ""
    }

    function profile(programmeId) {
        for (var k in models)
            for (var i = 0; i < models[k].length; ++i)
                if (models[k][i].programmeId === programmeId)
                    return models[k][i]
        return null
    }

    // BACKWARD COMPATIBILITY. Legacy sessions predate programmeId and store
    // only stable numeric state, so the identity is recomputed from that -
    // never from a saved display string, which may have been translated.
    //   range      10 | 50
    //   isPistol   bool
    //   shotCount  -1 for unlimited
    //   fifteen    true when the 15-shot model was in use
    function legacyProgrammeId(range, isPistol, shotCount, fifteen) {
        var key = "game" + range + "RangeEventModel" + (fifteen ? "_15" : "")
        var e = entriesFor(key)
        for (var i = 0; i < e.length; ++i)
            if (e[i].isPistol === isPistol && e[i].shotCount === shotCount)
                return e[i].programmeId
        return ""
    }

    // Populate a ListModel from the catalogue, preserving order and adding
    // programmeId as a role. qsTr() is applied HERE, at display-build time,
    // exactly as the replaced ListElement literals did - the *Key fields are
    // the English source text, and programmeId is what logic uses.
    function fill(model, modelKey) {
        var e = entriesFor(modelKey)
        model.clear()
        for (var i = 0; i < e.length; ++i)
            model.append({ "programmeId":  e[i].programmeId,
                           "name":         qsTr(e[i].nameKey),
                           "count":        e[i].shotCount,
                           "gameDisplay1": qsTr(e[i].gameDisplay1Key),
                           "gameDisplay2": qsTr(e[i].gameDisplay2Key),
                           "matchDisplay": qsTr(e[i].matchDisplayKey) })
    }

    // ── hierarchy queries (SETA selector) ────────────────────────────────
    // Rule set -> discipline -> programme. Everything below is derived from
    // the SAME entries the ListModels use, so the hierarchy can never offer a
    // programme the engine does not know, and no programme can go missing.
    //
    // rulesetId is COMPETITION AUTHORITY, not target standard: a DSB rule set
    // still shoots issf.10m.air-rifle geometry. The two must not be collapsed.

    function allEntries() {
        var out = []
        for (var k in models)
            for (var i = 0; i < models[k].length; ++i)
                out.push(models[k][i])
        return out
    }

    // Distinct rule sets present in the catalogue, in a stable order.
    // "techaim" is not a federation - it is the practice-preset set - so it is
    // labelled as such rather than pretending to carry rule authority.
    function ruleSets() {
        var seen = {}, out = []
        var all = allEntries()
        for (var i = 0; i < all.length; ++i) {
            var r = all[i].rulesetId
            if (seen[r]) continue
            seen[r] = true
            out.push({ "rulesetId": r,
                       "federation": all[i].federation,
                       "official": all[i].programmeType === "OFFICIAL",
                       "labelKey": r === "issf" ? "ISSF"
                                 : r === "dsb"  ? "DSB / German"
                                                : "Practice presets" })
        }
        out.sort(function(a, b) { return a.rulesetId === "issf" ? -1
                                       : b.rulesetId === "issf" ?  1 : 0 })
        return out
    }

    // Distinct disciplines within a rule set. Keyed by disciplineId, which is
    // stable; the label is derived from the target standard, not translated
    // text, so a German UI cannot change which discipline this is.
    function disciplines(rulesetId) {
        var seen = {}, out = []
        var all = allEntries()
        for (var i = 0; i < all.length; ++i) {
            var e = all[i]
            if (e.rulesetId !== rulesetId) continue
            if (seen[e.disciplineId]) continue
            seen[e.disciplineId] = true
            out.push({ "disciplineId": e.disciplineId,
                       "distanceM": e.distanceM,
                       "targetStandardId": e.targetStandardId,
                       "labelKey": e.gameDisplay1Key + " " + e.gameDisplay2Key })
        }
        out.sort(function(a, b) { return a.distanceM - b.distanceM })
        return out
    }

    // Programmes within a rule set + discipline. Step 3 of the selector is
    // shown only when this returns more than one.
    function programmes(rulesetId, disciplineId) {
        var out = []
        var all = allEntries()
        for (var i = 0; i < all.length; ++i) {
            var e = all[i]
            if (e.rulesetId === rulesetId && e.disciplineId === disciplineId)
                out.push(e)
        }
        return out
    }

    function count() {
        var n = 0
        for (var k in models) n += models[k].length
        return n
    }
}
