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


    // ─────────────────────────────────────────────────────────────────────────
    // DSB 2026 — Sportordnung des Deutschen Schützenbundes, Stand 01.01.2026.
    //
    // A SEPARATE list, deliberately. The four `models` arrays above are
    // INDEX-LOCKED (ShootingPage resolves a selection by row index), so adding
    // DSB rows there would silently move every ISSF programme. These entries
    // are surfaced through allEntries()/the hierarchy and carry their own
    // runtime configuration instead of a legacy row index.
    //
    // AUTHORITY: docs/rules/dsb-2026-source-register.md and the programme
    // matrix beside it. Every number below traces to a rule number and a page.
    //
    // FOUR SEPARATE LAYERS, never collapsed into one enum:
    //   rulesetId          DSB_2026
    //   ruleNumber         1.10 / 1.20 / 1.40 / 1.60 / 1.80 / 2.10 / 2.20
    //   programmeVariant   20 | 40 | 60 | 3x10 | 3x20 | 3x40 | 30
    //   competitionContext DM_2026 (…LM, regional and customer contexts later)
    //
    // scoringMode belongs to the CONTEXT, not to the rule: DSB DM 2026 scores
    // 1.10 and 1.80 in tenths and 1.20/1.40/1.60/2.10/2.20 in whole rings, so
    // "DSB means integer" is false and must never be written anywhere.
    //
    // TIMING IS EXPLICIT, never inferred. DSB uses BOTH models for
    // three-position rifle: 1.20 runs three independent position clocks, while
    // 1.40 and 1.60 run one master clock across the same three positions.
    readonly property string timingSingleMatchClock: "SINGLE_MATCH_CLOCK"
    readonly property string timingIndependentPositionClocks: "INDEPENDENT_POSITION_CLOCKS"

    // Preparation policy: is the preparation/sighting period inside or outside
    // the shooting time? Every DSB programme here is OUTSIDE (Teil 1 S. 20 /
    // Teil 2 S. 25); ISSF combines them, which is exactly why this is a field.
    readonly property string prepOutsideMatchTime: "OUTSIDE_MATCH_TIME"

    // Sighter policy per phase.
    readonly property string sightersUnlimitedInPreparation: "UNLIMITED_IN_PREPARATION"
    // 1.20 only: sighting before prone and before standing is at the shooter's
    // discretion and consumes THAT POSITION'S clock.
    readonly property string sightersInsidePositionClock: "INSIDE_POSITION_CLOCK"

    // Position transition. DSB 1.20 does not chain its clocks: a position ends,
    // the session waits in POSITION_CHANGE, and the next clock is started by an
    // authorised match-control action (S-C.6).
    readonly property string transitionGated: "GATED_BY_MATCH_CONTROL"
    // 1.40 / 1.60: the master clock simply keeps running across the change.
    readonly property string transitionWithinMasterClock: "WITHIN_MASTER_CLOCK"

    // Where a duration's authority comes from. A recommended time may be
    // overridden by the Ausschreibung; a rule time may not.
    readonly property string timeAuthorityRule: "RULE"
    readonly property string timeAuthorityRecommended: "RECOMMENDED"

    readonly property var dsbProgrammes: ([
        // ── 1.10 Luftgewehr 10 m ─────────────────────────────────────────
        { "programmeId": "dsb.10m.air-rifle.lg20",
          "rulesetId": "dsb", "rulesetVersion": "2026-01-01", "federation": "DSB",
          "ruleNumber": "1.10", "programmeVariant": "20",
          "competitionContext": "DM_2026", "programmeType": "OFFICIAL",
          "targetStandardId": "issf.10m.air-rifle", "dsbTargetNumber": 1,
          "disciplineId": "AR10", "distanceM": 10, "targetFamily": "AIR_RIFLE",
          "disciplineLabelKey": "Luftgewehr",
          "isPistol": false, "shotCount": 20, "unlimited": false,
          "scoringMode": "DECIMAL", "fifteenShotVariant": false,
          "positions": ["STANDING"], "shotsPerPosition": [20],
          "timingModel": "SINGLE_MATCH_CLOCK",
          "matchMinutes": 30, "positionMinutes": [],
          "matchTimeAuthority": "RULE",
          "preparationMinutes": 15, "preparationPolicy": "OUTSIDE_MATCH_TIME",
          "sighterPolicy": "UNLIMITED_IN_PREPARATION",
          "positionTransitionPolicy": "",
          "nameKey": "DSB 1.10 LUFTGEWEHR 20", "gameDisplay1Key": "10M AIR",
          "gameDisplay2Key": "RIFLE", "matchDisplayKey": "LG-20" },
        { "programmeId": "dsb.10m.air-rifle.lg40",
          "rulesetId": "dsb", "rulesetVersion": "2026-01-01", "federation": "DSB",
          "ruleNumber": "1.10", "programmeVariant": "40",
          "competitionContext": "DM_2026", "programmeType": "OFFICIAL",
          "targetStandardId": "issf.10m.air-rifle", "dsbTargetNumber": 1,
          "disciplineId": "AR10", "distanceM": 10, "targetFamily": "AIR_RIFLE",
          "disciplineLabelKey": "Luftgewehr",
          "isPistol": false, "shotCount": 40, "unlimited": false,
          "scoringMode": "DECIMAL", "fifteenShotVariant": false,
          "positions": ["STANDING"], "shotsPerPosition": [40],
          "timingModel": "SINGLE_MATCH_CLOCK",
          "matchMinutes": 50, "positionMinutes": [],
          "matchTimeAuthority": "RULE",
          "preparationMinutes": 15, "preparationPolicy": "OUTSIDE_MATCH_TIME",
          "sighterPolicy": "UNLIMITED_IN_PREPARATION",
          "positionTransitionPolicy": "",
          "nameKey": "DSB 1.10 LUFTGEWEHR 40", "gameDisplay1Key": "10M AIR",
          "gameDisplay2Key": "RIFLE", "matchDisplayKey": "LG-40" },
        { "programmeId": "dsb.10m.air-rifle.lg60",
          "rulesetId": "dsb", "rulesetVersion": "2026-01-01", "federation": "DSB",
          "ruleNumber": "1.10", "programmeVariant": "60",
          "competitionContext": "DM_2026", "programmeType": "OFFICIAL",
          "targetStandardId": "issf.10m.air-rifle", "dsbTargetNumber": 1,
          "disciplineId": "AR10", "distanceM": 10, "targetFamily": "AIR_RIFLE",
          "disciplineLabelKey": "Luftgewehr",
          "isPistol": false, "shotCount": 60, "unlimited": false,
          "scoringMode": "DECIMAL", "fifteenShotVariant": false,
          "positions": ["STANDING"], "shotsPerPosition": [60],
          "timingModel": "SINGLE_MATCH_CLOCK",
          "matchMinutes": 75, "positionMinutes": [],
          "matchTimeAuthority": "RULE",
          "preparationMinutes": 15, "preparationPolicy": "OUTSIDE_MATCH_TIME",
          "sighterPolicy": "UNLIMITED_IN_PREPARATION",
          "positionTransitionPolicy": "",
          "nameKey": "DSB 1.10 LUFTGEWEHR 60", "gameDisplay1Key": "10M AIR",
          "gameDisplay2Key": "RIFLE", "matchDisplayKey": "LG-60" },

        // ── 1.20 Luftgewehr 3-Stellung ───────────────────────────────────
        // THE ONE PROGRAMME WITH INDEPENDENT POSITION CLOCKS. The 15-minute
        // preparation runs once, before KNEELING, outside every position clock.
        // Each position clock then CONTAINS that position's own sighting, and
        // the next clock is started by match control, never by expiry.
        { "programmeId": "dsb.10m.air-rifle.3x10",
          "rulesetId": "dsb", "rulesetVersion": "2026-01-01", "federation": "DSB",
          "ruleNumber": "1.20", "programmeVariant": "3x10",
          "competitionContext": "DM_2026", "programmeType": "OFFICIAL",
          "targetStandardId": "issf.10m.air-rifle", "dsbTargetNumber": 1,
          "disciplineId": "AR10_3P", "distanceM": 10, "targetFamily": "AIR_RIFLE",
          "disciplineLabelKey": "Luftgewehr 3-Stellung",
          "isPistol": false, "shotCount": 30, "unlimited": false,
          "scoringMode": "INTEGER", "fifteenShotVariant": false,
          "positions": ["KNEELING", "PRONE", "STANDING"],
          "shotsPerPosition": [10, 10, 10],
          "timingModel": "INDEPENDENT_POSITION_CLOCKS",
          "matchMinutes": 0, "positionMinutes": [25, 20, 30],
          "matchTimeAuthority": "RULE",
          "preparationMinutes": 15, "preparationPolicy": "OUTSIDE_MATCH_TIME",
          "sighterPolicy": "INSIDE_POSITION_CLOCK",
          "positionTransitionPolicy": "GATED_BY_MATCH_CONTROL",
          "nameKey": "DSB 1.20 LUFTGEWEHR 3-STELLUNG 3x10",
          "gameDisplay1Key": "10M AIR", "gameDisplay2Key": "RIFLE",
          "matchDisplayKey": "3x10" },
        { "programmeId": "dsb.10m.air-rifle.3x20",
          "rulesetId": "dsb", "rulesetVersion": "2026-01-01", "federation": "DSB",
          "ruleNumber": "1.20", "programmeVariant": "3x20",
          "competitionContext": "DM_2026", "programmeType": "OFFICIAL",
          "targetStandardId": "issf.10m.air-rifle", "dsbTargetNumber": 1,
          "disciplineId": "AR10_3P", "distanceM": 10, "targetFamily": "AIR_RIFLE",
          "disciplineLabelKey": "Luftgewehr 3-Stellung",
          "isPistol": false, "shotCount": 60, "unlimited": false,
          "scoringMode": "INTEGER", "fifteenShotVariant": false,
          "positions": ["KNEELING", "PRONE", "STANDING"],
          "shotsPerPosition": [20, 20, 20],
          "timingModel": "INDEPENDENT_POSITION_CLOCKS",
          "matchMinutes": 0, "positionMinutes": [35, 30, 40],
          "matchTimeAuthority": "RULE",
          "preparationMinutes": 15, "preparationPolicy": "OUTSIDE_MATCH_TIME",
          "sighterPolicy": "INSIDE_POSITION_CLOCK",
          "positionTransitionPolicy": "GATED_BY_MATCH_CONTROL",
          "nameKey": "DSB 1.20 LUFTGEWEHR 3-STELLUNG 3x20",
          "gameDisplay1Key": "10M AIR", "gameDisplay2Key": "RIFLE",
          "matchDisplayKey": "3x20" },

        // ── 1.40 KK-Sportgewehr 50 m 3x20 ────────────────────────────────
        // Three positions, ONE master clock. Same position sequence as 1.20 and
        // a completely different timing model - which is the whole reason
        // timing is a profile field and not a consequence of "3 positions".
        { "programmeId": "dsb.50m.rifle.3x20",
          "rulesetId": "dsb", "rulesetVersion": "2026-01-01", "federation": "DSB",
          "ruleNumber": "1.40", "programmeVariant": "3x20",
          "competitionContext": "DM_2026", "programmeType": "OFFICIAL",
          "targetStandardId": "issf.50m.rifle", "dsbTargetNumber": 3,
          "disciplineId": "RIFLE50_3P", "distanceM": 50,
          "disciplineLabelKey": "KK-Sportgewehr 3x20",
          "targetFamily": "SMALLBORE_RIFLE",
          "isPistol": false, "shotCount": 60, "unlimited": false,
          "scoringMode": "INTEGER", "fifteenShotVariant": false,
          "positions": ["KNEELING", "PRONE", "STANDING"],
          "shotsPerPosition": [20, 20, 20],
          "timingModel": "SINGLE_MATCH_CLOCK",
          "matchMinutes": 105, "positionMinutes": [],
          "matchTimeAuthority": "RULE",
          "preparationMinutes": 15, "preparationPolicy": "OUTSIDE_MATCH_TIME",
          "sighterPolicy": "UNLIMITED_IN_PREPARATION",
          "positionTransitionPolicy": "WITHIN_MASTER_CLOCK",
          "nameKey": "DSB 1.40 KK-SPORTGEWEHR 3x20", "gameDisplay1Key": "50 Meter",
          "gameDisplay2Key": "RIFLE", "matchDisplayKey": "KK 3x20" },

        // ── 1.60 KK-Freigewehr 50 m 3x40 ─────────────────────────────────
        { "programmeId": "dsb.50m.rifle.3x40",
          "rulesetId": "dsb", "rulesetVersion": "2026-01-01", "federation": "DSB",
          "ruleNumber": "1.60", "programmeVariant": "3x40",
          "competitionContext": "DM_2026", "programmeType": "OFFICIAL",
          "targetStandardId": "issf.50m.rifle", "dsbTargetNumber": 3,
          "disciplineId": "RIFLE50_3P", "distanceM": 50,
          "disciplineLabelKey": "KK-Freigewehr 3x40",
          "targetFamily": "SMALLBORE_RIFLE",
          "isPistol": false, "shotCount": 120, "unlimited": false,
          "scoringMode": "INTEGER", "fifteenShotVariant": false,
          "positions": ["KNEELING", "PRONE", "STANDING"],
          "shotsPerPosition": [40, 40, 40],
          "timingModel": "SINGLE_MATCH_CLOCK",
          "matchMinutes": 165, "positionMinutes": [],
          "matchTimeAuthority": "RULE",
          "preparationMinutes": 15, "preparationPolicy": "OUTSIDE_MATCH_TIME",
          "sighterPolicy": "UNLIMITED_IN_PREPARATION",
          "positionTransitionPolicy": "WITHIN_MASTER_CLOCK",
          "nameKey": "DSB 1.60 KK-FREIGEWEHR 3x40", "gameDisplay1Key": "50 Meter",
          "gameDisplay2Key": "RIFLE", "matchDisplayKey": "KK 3x40" },

        // ── 1.80 KK-Liegendkampf 50 m ────────────────────────────────────
        { "programmeId": "dsb.50m.rifle.prone60",
          "rulesetId": "dsb", "rulesetVersion": "2026-01-01", "federation": "DSB",
          "ruleNumber": "1.80", "programmeVariant": "60",
          "competitionContext": "DM_2026", "programmeType": "OFFICIAL",
          "targetStandardId": "issf.50m.rifle", "dsbTargetNumber": 3,
          "disciplineId": "PRONE50", "distanceM": 50,
          "disciplineLabelKey": "KK-Liegendkampf",
          "targetFamily": "SMALLBORE_RIFLE",
          "isPistol": false, "shotCount": 60, "unlimited": false,
          "scoringMode": "DECIMAL", "fifteenShotVariant": false,
          "positions": ["PRONE"], "shotsPerPosition": [60],
          "timingModel": "SINGLE_MATCH_CLOCK",
          "matchMinutes": 50, "positionMinutes": [],
          "matchTimeAuthority": "RULE",
          "preparationMinutes": 15, "preparationPolicy": "OUTSIDE_MATCH_TIME",
          "sighterPolicy": "UNLIMITED_IN_PREPARATION",
          "positionTransitionPolicy": "",
          "nameKey": "DSB 1.80 KK-LIEGENDKAMPF", "gameDisplay1Key": "50 Meter",
          "gameDisplay2Key": "RIFLE", "matchDisplayKey": "KK LIEGEND" },

        // ── 2.10 10 m Luftpistole ────────────────────────────────────────
        { "programmeId": "dsb.10m.air-pistol.lp20",
          "rulesetId": "dsb", "rulesetVersion": "2026-01-01", "federation": "DSB",
          "ruleNumber": "2.10", "programmeVariant": "20",
          "competitionContext": "DM_2026", "programmeType": "OFFICIAL",
          "targetStandardId": "issf.10m.air-pistol", "dsbTargetNumber": 7,
          "disciplineId": "AP10", "distanceM": 10, "targetFamily": "AIR_PISTOL",
          "disciplineLabelKey": "Luftpistole",
          "isPistol": true, "shotCount": 20, "unlimited": false,
          "scoringMode": "INTEGER", "fifteenShotVariant": false,
          "positions": ["STANDING"], "shotsPerPosition": [20],
          "timingModel": "SINGLE_MATCH_CLOCK",
          "matchMinutes": 30, "positionMinutes": [],
          "matchTimeAuthority": "RULE",
          "preparationMinutes": 15, "preparationPolicy": "OUTSIDE_MATCH_TIME",
          "sighterPolicy": "UNLIMITED_IN_PREPARATION",
          "positionTransitionPolicy": "",
          "nameKey": "DSB 2.10 LUFTPISTOLE 20", "gameDisplay1Key": "10M AIR",
          "gameDisplay2Key": "PISTOL", "matchDisplayKey": "LP-20" },
        { "programmeId": "dsb.10m.air-pistol.lp40",
          "rulesetId": "dsb", "rulesetVersion": "2026-01-01", "federation": "DSB",
          "ruleNumber": "2.10", "programmeVariant": "40",
          "competitionContext": "DM_2026", "programmeType": "OFFICIAL",
          "targetStandardId": "issf.10m.air-pistol", "dsbTargetNumber": 7,
          "disciplineId": "AP10", "distanceM": 10, "targetFamily": "AIR_PISTOL",
          "disciplineLabelKey": "Luftpistole",
          "isPistol": true, "shotCount": 40, "unlimited": false,
          "scoringMode": "INTEGER", "fifteenShotVariant": false,
          "positions": ["STANDING"], "shotsPerPosition": [40],
          "timingModel": "SINGLE_MATCH_CLOCK",
          "matchMinutes": 50, "positionMinutes": [],
          "matchTimeAuthority": "RULE",
          "preparationMinutes": 15, "preparationPolicy": "OUTSIDE_MATCH_TIME",
          "sighterPolicy": "UNLIMITED_IN_PREPARATION",
          "positionTransitionPolicy": "",
          "nameKey": "DSB 2.10 LUFTPISTOLE 40", "gameDisplay1Key": "10M AIR",
          "gameDisplay2Key": "PISTOL", "matchDisplayKey": "LP-40" },
        { "programmeId": "dsb.10m.air-pistol.lp60",
          "rulesetId": "dsb", "rulesetVersion": "2026-01-01", "federation": "DSB",
          "ruleNumber": "2.10", "programmeVariant": "60",
          "competitionContext": "DM_2026", "programmeType": "OFFICIAL",
          "targetStandardId": "issf.10m.air-pistol", "dsbTargetNumber": 7,
          "disciplineId": "AP10", "distanceM": 10, "targetFamily": "AIR_PISTOL",
          "disciplineLabelKey": "Luftpistole",
          "isPistol": true, "shotCount": 60, "unlimited": false,
          "scoringMode": "INTEGER", "fifteenShotVariant": false,
          "positions": ["STANDING"], "shotsPerPosition": [60],
          "timingModel": "SINGLE_MATCH_CLOCK",
          "matchMinutes": 75, "positionMinutes": [],
          "matchTimeAuthority": "RULE",
          "preparationMinutes": 15, "preparationPolicy": "OUTSIDE_MATCH_TIME",
          "sighterPolicy": "UNLIMITED_IN_PREPARATION",
          "positionTransitionPolicy": "",
          "nameKey": "DSB 2.10 LUFTPISTOLE 60", "gameDisplay1Key": "10M AIR",
          "gameDisplay2Key": "PISTOL", "matchDisplayKey": "LP-60" },

        // ── 2.20 50 m Pistole ────────────────────────────────────────────
        // The 30-shot time is printed as a RECOMMENDATION, so its authority is
        // RECOMMENDED and an Ausschreibung may override it. The 60-shot time is
        // a rule time and may not be.
        { "programmeId": "dsb.50m.pistol.p60",
          "rulesetId": "dsb", "rulesetVersion": "2026-01-01", "federation": "DSB",
          "ruleNumber": "2.20", "programmeVariant": "60",
          "competitionContext": "DM_2026", "programmeType": "OFFICIAL",
          "targetStandardId": "issf.50m.pistol", "dsbTargetNumber": 4,
          "disciplineId": "FREEPISTOL50", "distanceM": 50,
          "disciplineLabelKey": "50 m Pistole",
          "targetFamily": "FREE_PISTOL",
          "isPistol": true, "shotCount": 60, "unlimited": false,
          "scoringMode": "INTEGER", "fifteenShotVariant": false,
          "positions": ["STANDING"], "shotsPerPosition": [60],
          "timingModel": "SINGLE_MATCH_CLOCK",
          "matchMinutes": 90, "positionMinutes": [],
          "matchTimeAuthority": "RULE",
          "preparationMinutes": 15, "preparationPolicy": "OUTSIDE_MATCH_TIME",
          "sighterPolicy": "UNLIMITED_IN_PREPARATION",
          "positionTransitionPolicy": "",
          "nameKey": "DSB 2.20 PISTOLE 60", "gameDisplay1Key": "50 Meter Free",
          "gameDisplay2Key": "PISTOL", "matchDisplayKey": "P-60" },
        { "programmeId": "dsb.50m.pistol.p30",
          "rulesetId": "dsb", "rulesetVersion": "2026-01-01", "federation": "DSB",
          "ruleNumber": "2.20", "programmeVariant": "30",
          "competitionContext": "DM_2026", "programmeType": "OFFICIAL",
          "targetStandardId": "issf.50m.pistol", "dsbTargetNumber": 4,
          "disciplineId": "FREEPISTOL50", "distanceM": 50,
          "disciplineLabelKey": "50 m Pistole",
          "targetFamily": "FREE_PISTOL",
          "isPistol": true, "shotCount": 30, "unlimited": false,
          "scoringMode": "INTEGER", "fifteenShotVariant": false,
          "positions": ["STANDING"], "shotsPerPosition": [30],
          "timingModel": "SINGLE_MATCH_CLOCK",
          "matchMinutes": 55, "positionMinutes": [],
          "matchTimeAuthority": "RECOMMENDED",
          "preparationMinutes": 15, "preparationPolicy": "OUTSIDE_MATCH_TIME",
          "sighterPolicy": "UNLIMITED_IN_PREPARATION",
          "positionTransitionPolicy": "",
          "nameKey": "DSB 2.20 PISTOLE 30", "gameDisplay1Key": "50 Meter Free",
          "gameDisplay2Key": "PISTOL", "matchDisplayKey": "P-30" }
    ])

    // Every DSB entry, in declaration order.
    function dsbEntries() { return dsbProgrammes }

    // The full competition definition for a programme, with the ISSF/practice
    // entries given explicit defaults so callers never branch on ruleset.
    //
    // ISSF and the practice presets keep the behaviour they have always had:
    // one match clock, a combined preparation+sighting period INSIDE the phase
    // sequence the engine already runs, and the scoring mode the discipline
    // selects. Nothing about them changes here.
    function competitionDefinition(programmeId) {
        var e = profile(programmeId)
        if (e === null) return null
        if (e.rulesetId === "dsb") return e
        return {
            "programmeId": e.programmeId,
            "rulesetId": e.rulesetId, "rulesetVersion": "",
            "federation": e.federation,
            "ruleNumber": "", "programmeVariant": String(e.shotCount),
            "competitionContext": "", "programmeType": e.programmeType,
            "targetStandardId": e.targetStandardId, "dsbTargetNumber": 0,
            "disciplineId": e.disciplineId, "distanceM": e.distanceM,
            "targetFamily": e.targetFamily, "isPistol": e.isPistol,
            "shotCount": e.shotCount, "unlimited": e.unlimited,
            "scoringMode": e.scoringMode,
            "fifteenShotVariant": e.fifteenShotVariant,
            "positions": e.positions, "shotsPerPosition": [],
            // ISSF/practice timing is NOT expressed here: it stays where it has
            // always been (AppSettings/the discipline selectors). Declaring a
            // number would create a second, competing source of truth.
            "timingModel": timingSingleMatchClock,
            "matchMinutes": 0, "positionMinutes": [],
            "matchTimeAuthority": "",
            "preparationMinutes": 0, "preparationPolicy": "",
            "sighterPolicy": "", "positionTransitionPolicy": "",
            "nameKey": e.nameKey, "gameDisplay1Key": e.gameDisplay1Key,
            "gameDisplay2Key": e.gameDisplay2Key,
            "matchDisplayKey": e.matchDisplayKey
        }
    }

    // Session metadata for the journal and the report. Enough to identify
    // exactly which rules governed a stored match, so a later rule change
    // cannot silently reinterpret history (requirement 20).
    function rulesetMetadata(programmeId) {
        var d = competitionDefinition(programmeId)
        if (d === null) return null
        return { "ruleset": d.rulesetId, "rulesetVersion": d.rulesetVersion,
                 "ruleNumber": d.ruleNumber,
                 "programmeVariant": d.programmeVariant,
                 "competitionContext": d.competitionContext,
                 "scoringMode": d.scoringMode,
                 "timingModel": d.timingModel,
                 "targetStandardId": d.targetStandardId,
                 "programmeId": d.programmeId }
    }

    // THE ADOPTED AUTHORITY a session records at creation. Everything here is a
    // machine value: the session must be able to prove what governed it years
    // later, from a journal written on a range running any language.
    //
    // prepMs/matchMs are the durations the session ACTUALLY adopted - passed in
    // rather than re-read here, because what the clocks were anchored to is the
    // fact worth persisting, not what the catalogue would say today.
    //
    // A programme with no rule authority (ISSF, the practice presets) returns
    // null: that session is LEGACY, which is an honest answer, not a gap to
    // fill with an invented identity.
    function ruleAuthorityFor(programmeId, prepMs, matchMs) {
        var d = competitionDefinition(programmeId)
        if (d === null || d.matchTimeAuthority === "") return null
        var seq = (d.positions && d.positions.length > 1)
                  ? d.positions.join(",") : ""
        var durs = ""
        if (d.positionMinutes && d.positionMinutes.length > 0) {
            var ms = []
            for (var i = 0; i < d.positionMinutes.length; ++i)
                ms.push(d.positionMinutes[i] * 60000)
            durs = ms.join(",")
        }
        return { "programmeId":        d.programmeId,
                 "rulesetId":          d.rulesetId,
                 "rulesetVersion":     d.rulesetVersion,
                 "ruleNumber":         d.ruleNumber,
                 "programmeVariant":   d.programmeVariant,
                 "competitionContext": d.competitionContext,
                 "scoringMode":        d.scoringMode,
                 "timingModel":        d.timingModel,
                 "targetStandardId":   d.targetStandardId,
                 "disciplineId":       d.disciplineId,
                 "distanceM":          d.distanceM,
                 "preparationMs":      prepMs,
                 "matchMs":            matchMs,
                 "positionSequence":   seq,
                 "positionDurationsMs": durs }
    }

    // Timing for the engine. Returns milliseconds, because that is what the
    // session controller takes. positionMs is empty for a single-clock
    // programme and matchMs is 0 for an independent-position-clock programme -
    // a caller that reads the wrong one gets 0, not a plausible wrong number.
    function timingFor(programmeId) {
        var d = competitionDefinition(programmeId)
        if (d === null) return null
        var pos = []
        for (var i = 0; i < d.positionMinutes.length; ++i)
            pos.push(d.positionMinutes[i] * 60000)
        return { "timingModel": d.timingModel,
                 "matchMs": d.matchMinutes * 60000,
                 "positionMs": pos,
                 "preparationMs": d.preparationMinutes * 60000,
                 "preparationPolicy": d.preparationPolicy,
                 "sighterPolicy": d.sighterPolicy,
                 "positionTransitionPolicy": d.positionTransitionPolicy,
                 "matchTimeAuthority": d.matchTimeAuthority }
    }

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
        for (var j = 0; j < dsbProgrammes.length; ++j)
            if (dsbProgrammes[j].programmeId === programmeId)
                return dsbProgrammes[j]
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
        // DSB entries are NOT in `models` - those arrays are index-locked - so
        // they are appended here. entriesFor()/fill() deliberately never see
        // them, which is what keeps every ISSF row index stable.
        for (var j = 0; j < dsbProgrammes.length; ++j)
            out.push(dsbProgrammes[j])
        return out
    }

    // PAPER MODE IS PART OF THE QUESTION. The catalogue holds both the standard
    // and the 15-shot-paper variant of every preset, because the running
    // application shows exactly one of them - ShootingPage picks the model from
    // APPSETTINGS.getIs15Shoot(). A hierarchy that ignored this would offer
    // "MATCH-20" twice with no way to tell the two apart, and half of what it
    // offered could not be run. Passing `fifteen` filters to the variant the
    // installation is actually configured for; omitting it means "every entry",
    // which is what the catalogue-integrity checks want.
    //
    // A real consequence, not a bug: in 15-shot paper mode there is NO 60-shot
    // entry, so no ISSF course exists and the ISSF rule set correctly
    // disappears. That installation genuinely cannot run an official course.
    function entriesIn(fifteen) {
        var all = allEntries()
        if (fifteen === undefined || fifteen === null) return all
        var out = []
        for (var i = 0; i < all.length; ++i)
            if (all[i].fifteenShotVariant === fifteen) out.push(all[i])
        return out
    }

    // Distinct rule sets present in the catalogue, in a stable order.
    // "techaim" is not a federation - it is the practice-preset set - so it is
    // labelled as such rather than pretending to carry rule authority.
    function ruleSets(fifteen) {
        var seen = {}, out = []
        var all = entriesIn(fifteen)
        for (var i = 0; i < all.length; ++i) {
            var r = all[i].rulesetId
            if (seen[r]) continue
            seen[r] = true
            out.push({ "rulesetId": r,
                       "federation": all[i].federation,
                       "official": all[i].programmeType === "OFFICIAL",
                       "labelKey": r === "issf" ? "ISSF"
                                 : r === "dsb"  ? "DSB 2026"
                                                : "Practice presets" })
        }
        // ISSF first, then DSB, then the practice presets. A stable, declared
        // order - not whichever ruleset happened to appear first in the data.
        var rank = { "issf": 0, "dsb": 1, "techaim": 2 }
        out.sort(function(a, b) {
            var ra = rank[a.rulesetId] === undefined ? 99 : rank[a.rulesetId]
            var rb = rank[b.rulesetId] === undefined ? 99 : rank[b.rulesetId]
            return ra - rb
        })
        return out
    }

    // Distinct disciplines within a rule set. Keyed by disciplineId, which is
    // stable; the label is derived from the target standard, not translated
    // text, so a German UI cannot change which discipline this is.
    function disciplines(rulesetId, fifteen) {
        var seen = {}, out = []
        var all = entriesIn(fifteen)
        for (var i = 0; i < all.length; ++i) {
            var e = all[i]
            if (e.rulesetId !== rulesetId) continue
            if (seen[e.disciplineId]) continue
            seen[e.disciplineId] = true
            // A programme may name its discipline explicitly. DSB needs this:
            // 1.10 and 1.20 are both "10M AIR RIFLE" by display key, and a
            // selector that showed the same label twice would be unusable.
            out.push({ "disciplineId": e.disciplineId,
                       "distanceM": e.distanceM,
                       "targetStandardId": e.targetStandardId,
                       "labelKey": e.disciplineLabelKey !== undefined
                                   && e.disciplineLabelKey !== ""
                                   ? e.disciplineLabelKey
                                   : e.gameDisplay1Key + " " + e.gameDisplay2Key })
        }
        out.sort(function(a, b) { return a.distanceM - b.distanceM })
        return out
    }

    // Programmes within a rule set + discipline. Step 3 of the selector is
    // shown only when this returns more than one.
    function programmes(rulesetId, disciplineId, fifteen) {
        var out = []
        var all = entriesIn(fifteen)
        for (var i = 0; i < all.length; ++i) {
            var e = all[i]
            if (e.rulesetId === rulesetId && e.disciplineId === disciplineId)
                out.push(e)
        }
        return out
    }

    // ── programmeId -> the EXISTING runtime configuration ────────────────
    // This is the whole integration contract. The hierarchy changes NAVIGATION
    // only; this function is the single place a choice becomes machine state,
    // and it introduces no new match rules. The event index it returns is the
    // same index the legacy weapon/distance/event controls already set, which
    // is what makes old and new paths provably identical - and it is why there
    // is no second set of match-configuration rules to drift out of step.
    //
    // gameEvent indices are the LoginPage ones: 0..4 the shot-count events in
    // catalogue order, 5 unlimited. 6 (3P Final) and 7 (10 m Final) are NOT
    // produced here - the finals are a separate domain with no catalogue entry,
    // and inventing one would put a programme in the hierarchy that the
    // qualification engine cannot run.
    function eventIndexFor(p) {
        if (p.unlimited) return 5
        var s = p.shotCount
        if (p.fifteenShotVariant)
            return s === 10 ? 0 : s === 15 ? 1 : s === 20 ? 2 : s === 30 ? 3 : 4
        return s === 10 ? 0 : s === 20 ? 1 : s === 30 ? 2 : s === 40 ? 3 : 4
    }

    function runtimeConfig(programmeId) {
        var p = profile(programmeId)
        if (p === null) return null
        // A DSB programme supplies its OWN durations and scoring mode; it never
        // borrows the legacy shot-count-keyed timing. gameEvent is -1 because
        // there is no legacy event card behind it.
        if (p.rulesetId === "dsb") {
            var t = timingFor(programmeId)
            return { "programmeId":      p.programmeId,
                     "gameRange":        p.distanceM,
                     "gameMode":         p.isPistol ? 0 : 1,
                     "gameEvent":        -1,
                     "fifteen":          false,
                     "shotCount":        p.shotCount,
                     "targetStandardId": p.targetStandardId,
                     "programmeType":    p.programmeType,
                     "matchDisplayKey":  p.matchDisplayKey,
                     "scoringMode":      p.scoringMode,
                     "timingModel":      t.timingModel,
                     "matchMs":          t.matchMs,
                     "positionMs":       t.positionMs,
                     "preparationMs":    t.preparationMs,
                     "positions":        p.positions,
                     "shotsPerPosition": p.shotsPerPosition }
        }
        return { "programmeId":      p.programmeId,
                 "gameRange":        p.distanceM,
                 "gameMode":         p.isPistol ? 0 : 1,
                 "gameEvent":        eventIndexFor(p),
                 "fifteen":          p.fifteenShotVariant,
                 "shotCount":        p.shotCount,
                 "targetStandardId": p.targetStandardId,
                 "programmeType":    p.programmeType,
                 "matchDisplayKey":  p.matchDisplayKey }
    }

    function count() {
        var n = 0
        for (var k in models) n += models[k].length
        return n
    }
}
