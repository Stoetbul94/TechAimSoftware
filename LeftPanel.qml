import QtQuick 2.2
Item {
    property int rootItemWidth:200
    property int rootItemHeight:725

    property alias name: nameText.text
    property alias gameDisplay1: gameDisplayText1.text
    property alias gameDisplay2: gameDisplayText2.text
    property alias matchDisplay: matchText.text
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

        onBackHomeFromServer : {
            console.log("*************************************************************************************")

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

        // Discipline card (compact — no photo, per range feedback)
        Rectangle {
            width: parent.width; height: 64; radius: 10
            color: "#26272c"; border.color: "#e8003d"; border.width: 1
            Row {
                anchors.centerIn: parent
                spacing: 12
                VIcon {
                    property bool isPistol: gameDisplayText2.text === "PISTOL"
                    viewBoxW: isPistol ? 48 : 96
                    viewBoxH: isPistol ? 32 : 28
                    width: isPistol ? 48 : 66
                    height: width * (viewBoxH / viewBoxW)
                    pathData: isPistol ? pistolPath : riflePath
                    color: "white"; filled: true; strokeWidth: 0
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: gameDisplayText1.text
                    color: "white"; font.bold: true
                    font.family: theme.fontFamily; font.pixelSize: 20
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }

        // Match badge
        Rectangle {
            width: parent.width; height: 46; radius: 8; color: "#e8003d"
            Text {
                anchors.centerIn: parent
                text: matchText.text
                color: "white"; font.bold: true
                font.family: theme.fontFamily; font.pixelSize: 20
            }
        }

        // Shooter name
        Rectangle {
            id: nameCard
            width: parent.width; height: 42; radius: 8
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

        readonly property int navSpacing: 12
        readonly property int navCount: 6
        // Each button grows to share the available height evenly.
        readonly property real btnHeight: (height - navSpacing*(navCount-1)) / navCount

        Column {
            anchors.fill: parent
            spacing: navColumn.navSpacing

            Repeater {
                id: navRepeater
                model: [
                    { key: "stats",    label: qsTr("Stats"),
                      path: "M18 20V10 M12 20V4 M6 20v-6" },
                    { key: "report",   label: qsTr("Report"),
                      path: "M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z M14 2v6h6 M16 13H8 M16 17H8 M10 9H8" },
                    { key: "coach",    label: qsTr("Coach report"),
                      path: "M12 15a7 7 0 1 0 0-14 7 7 0 0 0 0 14z M8.21 13.89L7 23l5-3 5 3-1.21-9.12" },
                    { key: "mpi",      label: qsTr("Group / MPI"),
                      path: "M12 22a10 10 0 1 0 0-20 10 10 0 0 0 0 20z M12 18a6 6 0 1 0 0-12 6 6 0 0 0 0 12z M12 14a2 2 0 1 0 0-4 2 2 0 0 0 0 4z" },
                    { key: "settings", label: qsTr("Settings"),
                      path: "M12 15a3 3 0 1 0 0-6 3 3 0 0 0 0 6z M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 1 1-2.83 2.83l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 1 1-4 0v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 1 1-2.83-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 1 1 0-4h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 1 1 2.83-2.83l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 1 1 4 0v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 1 1 2.83 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 1 1 0 4h-.09a1.65 1.65 0 0 0-1.51 1z" },
                    { key: "home",     label: qsTr("Home"),
                      path: "M3 9l9-7 9 7v11a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z M9 22V12h6v10" }
                ]
                delegate: Rectangle {
                    id: navBtn
                    width: navColumn.width
                    height: navColumn.btnHeight
                    radius: 10
                    color: navMouse.containsMouse ? "#2f3037" : "#26272c"
                    border.color: modelData.key === "mpi" && isShowMPI ? "#e8003d" : "#3a3b40"
                    border.width: modelData.key === "mpi" && isShowMPI ? 2 : 1
                    Row {
                        anchors.left: parent.left; anchors.leftMargin: 18
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 14
                        VIcon {
                            width: 26; height: 26
                            pathData: modelData.path
                            color: modelData.key === "mpi" && isShowMPI ? "#e8003d" : "white"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: modelData.label
                            color: "white"
                            font.family: theme.fontFamily
                            font.pixelSize: 17
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

        function navAction(key) {
            if (key === "stats") {
                if (sligterMode) {
                    cannotGenerate.text = sighterSummaryText
                    cannotGenerate.visible = true
                } else if (!isSaveGame) {
                    showSummary()
                }
            } else if (key === "report") {
                if (sligterMode) {
                    cannotGenerate.text = sighterMatchText
                    cannotGenerate.visible = true
                } else {
                    showMatchReport()
                }
            } else if (key === "coach") {
                if (sligterMode) {
                    cannotGenerate.text = sighterMatchText
                    cannotGenerate.visible = true
                } else {
                    // Open the Coach floating window via the manager (it re-analyses
                    // the current match on open and defaults to the Dashboard tab).
                    windowManager.openCoach()
                }
            } else if (key === "mpi") {
                isShowMPI = !isShowMPI
            } else if (key === "settings") {
                settingsClicked()
            } else if (key === "home") {
                homeButtonClicked()
            }
        }
    }

    // Anchor for the SettingsPage popup (roughly the Settings button, index 4).
    Item {
        id: settingsNavBtn
        property real navY: navColumn.y + 4*(navColumn.btnHeight + navColumn.navSpacing)
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
        matchReportPage.visible = true
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
