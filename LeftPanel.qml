import QtQuick 2.2
Item {
    property int rootItemWidth:200
    property int rootItemHeight:725

    property alias name: nameText.text
    property alias gameDisplay1: gameDisplayText1.text
    property alias gameDisplay2: gameDisplayText2.text
    property alias matchDisplay: matchText.text

    // UI-TRAIN-001. The badge below used to render matchDisplay directly.
    // matchDisplay holds INHERITED EVENT STATE - whatever the last selected
    // event card wrote ("MATCH 60", "FINAL 35") - and entering a Training Lab
    // programme never clears it, so Call & Diagnose presented a red MATCH 60
    // badge beside an athlete who was not shooting a match.
    //
    // Stage 5.2 gated the competition statusStrip on isTrainingModeAny but did
    // not reach here, because the left panel renders the SAME inherited state
    // through a different alias. The owner passes the ACTIVE programme in.
    // Empty means "no programme override" - i.e. a genuine competition event,
    // where matchDisplay is the correct thing to show.
    property string programmeLabel: ""
    readonly property string effectiveProgramme:
        programmeLabel !== "" ? programmeLabel : matchText.text

    // Which discipline plate to draw. Supplied by the owner where it is known
    // precisely (Prone vs 3 Positions cannot be told apart from the two
    // display strings, which are identical for both). Falls back to deriving
    // what it can, so the card is never blank.
    property string disciplineKeyOverride: ""
    readonly property string disciplineKey: {
        if (disciplineKeyOverride !== "") return disciplineKeyOverride
        // The stable discipline enum, never the displayed text: that text is
        // translated, so this returned the rifle plate for an Air Pistol
        // session in German. 0 = pistol, 1 = rifle.
        if (loginPage.gameMode === 0) return "AP10"
        return "AR10"
    }
    // The FULL discipline name. gameDisplay1 alone reads "10M AIR", which
    // names no discipline at all - the discriminating half is in gameDisplay2.
    property string disciplineNameOverride: ""
    readonly property string disciplineName: {
        if (disciplineNameOverride !== "") return disciplineNameOverride
        const a = gameDisplayText1.text, b = gameDisplayText2.text
        if (b === "" || a.indexOf(b) >= 0) return a
        return a + " " + b
    }
    property alias settingsX:navColumn.x
    property alias settingsY:settingsNavBtn.navY
    property alias settingsWidth:navColumn.width


    property bool isShowMPI: APPSETTINGS.getShowGroupAndMPI()
    property alias playVisible: play.visible
    property alias abhi: rectangle_1.scale

    property int offsetDisplacement: 100

    // Filled match-equipment silhouettes traced from the reference photos,
    // muzzle pointing right. Match AIR PISTOL: orthopedic grip with palm
    // shelf + finger grooves, forked barrel/air-cylinder, sights.
    // Authoring grid 48x32.
    readonly property string pistolPath:
        "M8 11 L8 9 L12 9 L12 11 L20 11 L20 10 L42 10 L42 8 L44 8 L44 10 " +
        "L45 10 L45 12.5 L44 12.5 L29 12.5 L29 14 L43 14 L43 17 L29 17 " +
        "L27 17 L27 20 L24 20 L24 17 L21 17 L20 20 L21 21 L20 23 L21 24 " +
        "L20 26 L19 29 L19 31 L6 31 L5 29 L7 17 L8 11 Z"
    // 50m TARGET RIFLE: long barrel, raised aperture sight, thumbhole stock,
    // hand-stop under the forend, adjustable buttplate hook. Grid 96x28.
    readonly property string riflePath:
        "M4 8 L11 8 L14 5 L23 5 L25 8 L31 8 L33 3 L37 3 L37 8 L46 9 L54 11 " +
        "L88 11 L88 8 L94 8 L94 15 L88 15 L88 13 L54 13 L50 14 L48 14 L48 22 " +
        "L45 22 L45 14 L41 14 L39 17 L37 17 L37 14 L34 14 L34 21 L28 21 L28 14 " +
        "L11 14 L8 15 L9 16 L8 17 L8 20 L4 20 Z"

    // TIGHT PANE. True when this panel is far shorter than the layout it was
    // designed against, which is the normal case on an Android tablet in
    // landscape: the whole application viewport there is 345 units tall (a
    // 2220x1080 panel at 440 dpi, devicePixelRatio 2.75) versus the 725-unit
    // reference at the top of this file. After the app header, status strip
    // and action bar, this pane gets roughly 249 units.
    //
    // The header cards then consumed about 209 of those 249, leaving the seven
    // navigation buttons a viewport of about TEN units - a 27-pixel strip. The
    // buttons were laid out below it and simply drawn off the bottom of the
    // screen, and Home is the LAST of the seven, so during a match there was
    // no way back to the home screen at all.
    //
    // The threshold is the height below which all seven buttons cannot be
    // shown at a tappable size (7*40 + 6*10 = 340) plus the header's own
    // needs. Desktop windows sit far above it, so `tightPane` is false there
    // and nothing about the Windows pane changes.
    readonly property bool tightPane: height > 0 && height < 420

    signal homeButtonClicked()
    signal settingsClicked()

//    MouseArea
//    {
//        anchors.fill: parent
//        onClicked:
//        {
//            console.log("I am here as well .............")
//        }
//    }

    Connections {
        target: loginPage

        function onBackHomeFromServer() {

            homeButtonClicked()
        }
    }

    Image {
        id: rectangle_1
        source: "qrc:/images/leftPanel/rectangle_1.png"
        x: ((parent.width/rootItemWidth)*0)
        y: ((parent.height/rootItemHeight)*0)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: rounded_rectangle_4_copy
        visible: true
        source: "qrc:/images/leftPanel/rounded_rectangle_4_copy.png"
        x: ((parent.width/rootItemWidth)*8)
        y: ((parent.height/rootItemHeight)*10)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: white_tile
        visible: true
        source: "qrc:/images/leftPanel/white_tile.png"
        x: ((parent.width/rootItemWidth)*34)
//        y: ((parent.height/rootItemHeight)*268)
        y: ((parent.height/rootItemHeight)*(268-offsetDisplacement))
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }

    // ── Redesign: full-height panel. Compact header (discipline · match ·
    // shooter) then a large nav column that fills the rest. The legacy PNG
    // tiles and their aliased Text elements stay as invisible data sources.
    Rectangle {   // panel background
        anchors.fill: parent
        color: "#15161a"
    }

    Column {
        id: headerCol
        anchors.top: parent.top; anchors.topMargin: parent.width * 0.06
        anchors.left: parent.left; anchors.leftMargin: parent.width * 0.06
        anchors.right: parent.right; anchors.rightMargin: parent.width * 0.06
        spacing: 10

        // Discipline card. Carries the discipline ARTWORK and the FULL
        // discipline name.
        //
        // It used to render gameDisplayText1 alone, which holds only the first
        // half of the name ("10M AIR"); the discriminating half ("RIFLE" /
        // "PISTOL") lives in gameDisplayText2 and was used only to pick an
        // icon. So the pane announced "10M AIR" and left the athlete to infer
        // the rest from a silhouette.
        Rectangle {
            // Height DERIVED from the art plus the label, not a fixed 96. The
            // fixed height cropped the plate's position-glyph row: cosmetic at
            // Prone, but those glyphs are the ONLY thing that distinguishes
            // 3 Positions from Prone, so 3P lost its differentiator entirely.
            width: parent.width
            height: discArt.height + discLabel.implicitHeight + 18
            radius: 10
            color: "#26272c"; border.color: "#e8003d"; border.width: 1

            DisciplineArt {
                id: discArt
                anchors.top: parent.top; anchors.topMargin: 6
                anchors.horizontalCenter: parent.horizontalCenter
                width: Math.min(parent.width - 16, 132)
                // On a tight pane the silhouette is the first thing to go. It
                // is the single largest item in the header, and it is
                // DECORATIVE here: discLabel below carries the full discipline
                // name in words, so nothing identifying is lost. Trading it
                // away is what buys the navigation a usable viewport.
                // The card's height derives from this value, so zero here
                // collapses the card without touching its formula.
                visible: !tightPane
                height: tightPane ? 0 : width * (64 / 120)
                discipline: disciplineKey
            }
            Text {
                id: discLabel
                anchors.top: discArt.bottom; anchors.topMargin: 2
                anchors.left: parent.left; anchors.right: parent.right
                anchors.leftMargin: 6; anchors.rightMargin: 6
                text: disciplineName
                color: "white"; font.bold: true
                font.family: theme.fontFamily; font.pixelSize: 15
                fontSizeMode: Text.HorizontalFit; minimumPixelSize: 9
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideNone
            }
        }

        // Programme badge. Competition red ONLY for a genuine competition
        // event; a Training Lab programme is not a match and must not borrow
        // the match colour any more than it may borrow the match wording.
        Rectangle {
            // Tight pane trims the badge and name cards too - see `tightPane`.
            width: parent.width; height: tightPane ? 34 : 46; radius: 8
            color: programmeLabel !== "" ? "#1b2733" : "#e8003d"
            border.color: programmeLabel !== "" ? "#3d5a75" : "transparent"
            border.width: programmeLabel !== "" ? 1 : 0
            Text {
                anchors.fill: parent
                anchors.margins: 6
                text: effectiveProgramme
                color: "white"; font.bold: true
                font.family: theme.fontFamily
                // "CALL & DIAGNOSE" and "POSITION TRANSITION" are far longer
                // than "MATCH 60". Shrink to fit rather than clip or elide -
                // a truncated programme name is the same defect in a new form.
                font.pixelSize: 20
                fontSizeMode: Text.HorizontalFit
                minimumPixelSize: 10
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideNone
            }
        }

        // Shooter name
        Rectangle {
            id: nameCard
            width: parent.width; height: tightPane ? 32 : 42; radius: 8
            color: "#26272c"; border.color: "#3a3b40"; border.width: 1
            Text {
                anchors.centerIn: parent
                text: nameText.text
                color: "white"; font.bold: true
                font.family: theme.fontFamily; font.pixelSize: 15
                elide: Text.ElideRight
                width: parent.width - 16
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }

    Image {
        id: play
        // `visible` doubles as the waiting-to-start STATE (playVisible alias is
        // read by enterPositionTransition and the auto-print guard), so the
        // redesign hides the icon with opacity instead of visible. The bottom
        // action bar in ShootingPage is the start control now.
        visible: true
        opacity: 0
        source: "qrc:/images/leftPanel/play.png"
        anchors.left: white_tile.left
        anchors.leftMargin: 3
        anchors.top: white_tile.bottom
        anchors.topMargin: 20
        width: ((rightPanel.width/rightPanel.rootItemWidth)*sourceSize.width)
        height: ((rightPanel.height/rightPanel.rootItemHeight)*sourceSize.height)

        MouseArea
        {
            anchors.fill: parent
            enabled: false
        }
    }

    Image {
        id: device_not_connected
        visible: false
        source: "qrc:/images/leftPanel/device_not_connected.png"
        x: ((parent.width/rootItemWidth)*14)
        y: ((parent.height/rootItemHeight)*653)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: device_connected
        visible: false
        source: "qrc:/images/leftPanel/device_connected.png"
        x: ((parent.width/rootItemWidth)*14)
        y: ((parent.height/rootItemHeight)*653)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    // ── Redesign: large labeled nav that fills the panel below the header.
    // Same handlers and guard logic as the legacy MouseAreas; PNG icons reused.
    Item {
        id: navColumn
        anchors.top: headerCol.bottom; anchors.topMargin: 16
        anchors.bottom: parent.bottom; anchors.bottomMargin: parent.width * 0.06
        anchors.left: parent.left; anchors.leftMargin: parent.width * 0.06
        anchors.right: parent.right; anchors.rightMargin: parent.width * 0.06

        readonly property int navSpacing: 10
        // Full navigation model — the ownership of the left-nav tabs. navCount
        // derives from it so adding a tab (e.g. Range incident) can never again
        // push a later tab (Home) off-screen: every item shares the available
        // height evenly (Option 3). At short window heights the shared height
        // shrinks uniformly with a sensible floor; icons/text scale down before
        // the buttons become un-tappable.
        // One icon family, authored on the same 24x24 grid with the same
        // optical box (roughly 4..20) and the same stroke, so no glyph
        // dominates another in the column. Detail that cannot survive the
        // 22 px compact size is deliberately absent - the old gear carried
        // sixteen 1.65-unit arcs that resolved to grey mush, and the target
        // rings had a centre too small to draw. See UI-DEC-014.
        readonly property var navModel: [
            { key: "stats",    label: qsTr("Stats"),
              path: "M4 20h16 M7.5 20V12.5 M12 20V6 M16.5 20V9.5" },
            { key: "report",   label: qsTr("Report"),
              path: "M6 3.5h8l4 4V20.5H6z M14 3.5v4h4 M9 13h6 M9 16.5h6" },
            { key: "coach",    label: qsTr("Coach report"),
              path: "M12 3.5a5.5 5.5 0 1 0 0 11 5.5 5.5 0 0 0 0-11z M8.6 13.6L7.2 20.8l4.8-2.5 4.8 2.5-1.4-7.2" },
            { key: "mpi",      label: qsTr("Group / MPI"),
              path: "M12 3.5a8.5 8.5 0 1 0 0 17 8.5 8.5 0 0 0 0-17z M12 7.4a4.6 4.6 0 1 0 0 9.2 4.6 4.6 0 0 0 0-9.2z M12 10.8a1.2 1.2 0 1 0 0 2.4 1.2 1.2 0 0 0 0-2.4z" },
            { key: "incident", label: qsTr("Range incident"),
              path: "M12 4L21 19.5H3z M12 10v4 M12 17h.01" },
            { key: "settings", label: qsTr("Settings"),
              // The gear is KEPT as drawn: simplifying it to spokes turned it
              // into a sunburst that no longer read as Settings at all. With
              // the VIcon fixes it resolves cleanly at both 22 and 26 px.
              path: "M12 15a3 3 0 1 0 0-6 3 3 0 0 0 0 6z M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 1 1-2.83 2.83l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 1 1-4 0v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 1 1-2.83-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 1 1 0-4h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 1 1 2.83-2.83l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 1 1 4 0v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 1 1 2.83 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 1 1 0 4h-.09a1.65 1.65 0 0 0-1.51 1z" },
            { key: "home",     label: qsTr("Home"),
              path: "M4 10.5L12 4l8 6.5V20.5H4z M9.5 20.5V14h5v6.5" }
        ]
        readonly property int navCount: navModel.length
        // Each button grows to share the available height evenly (floored so it
        // never collapses below a tappable size — below that a scroll fallback
        // would engage, but at every supported height all items fit).
        readonly property real btnHeight:
            Math.max(40, (height - navSpacing*(navCount-1)) / navCount)
        // Slightly smaller icon/text at short heights so labels never clip.
        readonly property bool compact: btnHeight < 54

        // The scroll fallback the sizing comment above promised, which did not
        // actually exist.
        //
        // btnHeight floors at 40 so buttons never become un-tappable, which
        // means the column needs 7*40 + 6*10 = 340 units MINIMUM. On this
        // Android tablet build the whole application viewport is 807 x 345 dp
        // (a 2220x1080 panel at 440dpi, devicePixelRatio 2.75) against the
        // 1366 x 724 desktop reference this pane was laid out for. After the
        // app header and the discipline/athlete header card, navColumn is left
        // with roughly 150 units - far under the 340 it needs.
        //
        // The Column simply overflowed, and Home is the LAST item, so during a
        // match the way back to the home screen was pushed off the bottom of
        // the panel with no way to reach it. Nothing was scrollable, so
        // dragging did nothing.
        //
        // Flickable + interactive-only-when-overflowing is deliberately
        // conservative: when the content fits (every desktop window size) the
        // Flickable is inert and non-interactive, so drag, hover and click
        // behaviour on Windows are exactly as before.
        Flickable {
            id: navFlick
            anchors.fill: parent
            contentWidth: width
            contentHeight: navContent.height
            clip: true
            interactive: contentHeight > height
            boundsBehavior: Flickable.StopAtBounds
            flickDeceleration: 4000

            Column {
                id: navContent
                width: navFlick.width
                spacing: navColumn.navSpacing

            Repeater {
                id: navRepeater
                model: navColumn.navModel
                delegate: Rectangle {
                    id: navBtn
                    width: navColumn.width
                    height: navColumn.btnHeight
                    radius: 10
                    color: navMouse.containsMouse ? "#2f3037" : "#26272c"
                    border.color: modelData.key === "mpi" && isShowMPI ? "#e8003d" : "#3a3b40"
                    border.width: modelData.key === "mpi" && isShowMPI ? 2 : 1
                    Row {
                        anchors.left: parent.left
                        anchors.leftMargin: navColumn.compact ? 14 : 18
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right: parent.right; anchors.rightMargin: 8
                        spacing: navColumn.compact ? 10 : 14
                        VIcon {
                            width: navColumn.compact ? 22 : 26
                            height: width
                            pathData: modelData.path
                            color: modelData.key === "mpi" && isShowMPI ? "#e8003d" : "white"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: modelData.label
                            color: "white"
                            font.family: theme.fontFamily
                            font.pixelSize: navColumn.compact ? 14 : 17
                            elide: Text.ElideRight
                            width: parent.width - (navColumn.compact ? 32 : 40)
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    MouseArea {
                        id: navMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: navColumn.navAction(modelData.key)
                    }
                }
            }
            }
        }

        // Scroll affordance, drawn with a plain Rectangle rather than a
        // Controls ScrollBar: this file imports QtQuick only, and adding
        // QtQuick.Controls to a pane this size to gain one indicator risks
        // type shadowing for no benefit.
        //
        // It exists because a Flickable that overflows gives NO hint that it
        // scrolls - which is precisely how Home became unreachable rather than
        // merely inconvenient. Present only while there is somewhere to scroll.
        Rectangle {
            visible: navFlick.interactive
            width: 3
            radius: 1.5
            color: "#5a5c63"
            anchors.right: parent.right
            y: navFlick.contentHeight > 0
               ? (navFlick.contentY / navFlick.contentHeight) * navFlick.height
               : 0
            height: navFlick.contentHeight > 0
                    ? Math.max(24, (navFlick.height / navFlick.contentHeight) * navFlick.height)
                    : 0
        }

        function navAction(key) {
            if (key === "stats") {
                if (sligterMode) {
                    dialogManager.showWarning(qsTr("Match Summary Unavailable"),
                        qsTr("You are currently in Sighter mode.\n\nA Match Summary can only be generated after official Match shots have been recorded. Switch to Match mode and fire at least one shot before generating the report."))
                } else if (!isSaveGame) {
                    showSummary()
                }
            } else if (key === "report") {
                if (sligterMode) {
                    dialogManager.showWarning(qsTr("Match Report Unavailable"),
                        qsTr("You are currently in Sighter mode.\n\nA Match Report can only be generated after official Match shots have been recorded. Switch to Match mode and fire at least one shot before generating the report."))
                } else {
                    showMatchReport()
                }
            } else if (key === "coach") {
                if (sligterMode) {
                    dialogManager.showWarning(qsTr("Coach Report Unavailable"),
                        qsTr("You are currently in Sighter mode.\n\nThe Coach Report analyses official Match shots. Switch to Match mode and fire at least one shot before opening it."))
                } else {
                    // Re-analyse the current match from the authoritative match
                    // record (real coords + positions), then open the Coach window.
                    shootingPage.feedCoachReport()
                    windowManager.openCoach()
                }
            } else if (key === "mpi") {
                isShowMPI = !isShowMPI
            } else if (key === "incident") {
                // Phase E: operator range control — EST incident / Jury
                // workflow (windowed; never on the athlete scoring face).
                windowManager.openIncidents()
            } else if (key === "settings") {
                settingsClicked()
            } else if (key === "home") {
                homeButtonClicked()
            }
        }
    }

    // Anchor for the SettingsPage popup — the Settings button is index 5 in the
    // nav model (stats0, report1, coach2, mpi3, incident4, settings5, home6).
    Item {
        id: settingsNavBtn
        property real navY: navColumn.y + 5*(navColumn.btnHeight + navColumn.navSpacing)
    }
    Image {
        id: sighter_selected
        visible: true
        source: "qrc:/images/leftPanel/sighter_selected.png"
        x: ((parent.width/rootItemWidth)*14)
        y: ((parent.height/rootItemHeight)*120)
        opacity: 0
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: match_selected
        visible: !sighter_selected.visible
        source: "qrc:/images/leftPanel/match_selected.png"
        x: ((parent.width/rootItemWidth)*14)
        y: ((parent.height/rootItemHeight)*120)-offsetDisplacement/2
        opacity: 0
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: user_picture_circle_with_black_border
        visible: true
        source: "qrc:/images/leftPanel/user_picture_circle_with_black_border.png"
        x: ((parent.width/rootItemWidth)*54)
//        y: ((parent.height/rootItemHeight)*240)
//        x: ((parent.width/rootItemWidth)*14)
        y: ((parent.height/rootItemHeight)*120)
        opacity: 0
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: name
        visible: true
        source: "qrc:/images/leftPanel/name.png"
        x: ((parent.width/rootItemWidth)*42)
        //y: ((parent.height/rootItemHeight)*308)
        y: ((parent.height/rootItemHeight)*(308-offsetDisplacement))
        opacity: 0
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Text {
        id: nameText
        anchors.left: name.left
        anchors.top: name.top
        width: ((parent.width/rootItemWidth)*name.sourceSize.width)
        height: ((parent.height/rootItemHeight)*name.sourceSize.height)
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: (height)


        text: "Dummy"
        color: "white"
        font.bold: true
        opacity: 0   // data source only; the visible name is drawn in nameCard
    }

    Image {
        // Legacy match badge — now drawn in headerCol; kept invisible for layout refs.
        id: match_60_40_box
        visible: false
        source: "qrc:/images/leftPanel/match_60_40_box.png"
        x: ((parent.width/rootItemWidth)*24) - height/2
        y: ((parent.height/rootItemHeight)*181)-offsetDisplacement/2
        opacity: 0
        width: parent.width - (2*x)
        height: ((parent.height/rootItemHeight)*sourceSize.height)*2
    }
    Text {
        // Data source for the headerCol match badge; not shown directly.
        id: matchText
        text: "Dummy"
        color: "white"
        opacity: 0
    }

    Image {
        id: pistol_box
        visible: true
        source: "qrc:/images/leftPanel/pistol_box.png"
        x: ((parent.width/rootItemWidth)*24)
        y: ((parent.height/rootItemHeight)*65)
        opacity: 0
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: pistol_box_copy
        visible: true
        source: "qrc:/images/leftPanel/pistol_box_copy.png"
        x: ((parent.width/rootItemWidth)*24)
        y: ((parent.height/rootItemHeight)*25)
        opacity: 0
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }

//    Rectangle {
//        anchors.fill: pistol_over
//        color: "#2298D4"
//    }

    Text {
        // Data source for the headerCol discipline label; not shown directly.
        id: gameDisplayText1
        text: "Dummy"
        color: "white"
        opacity: 0
    }
    Text {
        id: gameDisplayText2
        anchors.top: pistol_box.top
        anchors.left: pistol_box.left
        width: ((parent.height/rootItemHeight)*pistol_box.sourceSize.width)
        height: ((parent.height/rootItemHeight)*pistol_box.sourceSize.height)
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: (height)
        text: "Dummy"
        color: "white"
        opacity: 0
    }

    function showSummary()
    {
        // Open the Summary in the floating Report window via the manager.
        windowManager.openReport()
    }

    function showMatchReport()
    {
        // Open the Match report in the floating Report window (Match tab).
        windowManager.openMatchReport()
    }


    function showReport()
    {

    }

    function showSettings()
    {

    }

    function enableSighterMode(enableFlag)
    {
        sighter_selected.visible = enableFlag
    }

    // png text
    Text {
        id: dConnectionText
        x: device_connected.x + (device_connected.width/2) - (width/2) - 10
        y: device_connected.y + (device_connected.height/2) - (height/2) - 2
        text : device_connected.opacity == 1 ? qsTr("Device connected") : qsTr("Device not connected")
        width: implicitWidth
        height: implicitHeight
        color: "white"
        font.pointSize: 8
        visible: device_connected.visible
    }
    Text {
        id: sighterText
        width: ((parent.width/rootItemWidth)*sighter_selected.sourceSize.width)
        height: ((parent.height/rootItemHeight)*sighter_selected.sourceSize.height) / 2
        x: sighter_selected.x - 5
        y: sighter_selected.y + (height/8) - 5
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: (matchText.height) + 5
        text : qsTr("SIGHTER")
        color: "white"
        opacity: 0
    }

    function startFromServer()
    {
        // Nav column disabling for server-driven sessions can be reinstated
        // here if needed; the legacy icon ids it toggled no longer exist.
    }
}
