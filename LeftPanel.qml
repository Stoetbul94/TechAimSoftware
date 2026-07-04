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

    // ── Rebrand overlays (phase 3): themed cards drawn on the legacy PNG
    // tiles' exact geometry. The PNGs stay for layout; these cover them.
    Rectangle {   // panel background
        x: rectangle_1.x; y: rectangle_1.y
        width: rectangle_1.width; height: rectangle_1.height
        color: "#15161a"
    }
    Rectangle {   // discipline card (was blue)
        x: rounded_rectangle_4_copy.x; y: rounded_rectangle_4_copy.y
        width: rounded_rectangle_4_copy.width; height: rounded_rectangle_4_copy.height
        radius: 8
        color: "#26272c"
        border.color: "#e8003d"; border.width: 1
    }
    Rectangle {   // shooter name tile (was white) — compact, wraps the name
        id: nameCard
        x: white_tile.x; y: white_tile.y
        width: white_tile.width; height: 46
        radius: 6
        color: "#26272c"
        border.color: "#3a3b40"; border.width: 1
        Text {
            anchors.centerIn: parent
            text: nameText.text
            color: "white"; font.bold: true
            font.family: theme.fontFamily; font.pixelSize: 14
            elide: Text.ElideRight
            width: parent.width - 16
            horizontalAlignment: Text.AlignHCenter
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
    // ── Redesign: labeled nav column replaces the bare icon stack. Same
    // handlers and guard logic as the legacy MouseAreas; PNG icons reused.
    Column {
        id: navColumn
        anchors.top: nameCard.bottom
        anchors.topMargin: 14
        anchors.left: parent.left; anchors.leftMargin: parent.width * 0.08
        anchors.right: parent.right; anchors.rightMargin: parent.width * 0.08
        spacing: 8

        Repeater {
            id: navRepeater
            model: [
                { key: "stats",    label: qsTr("Stats"),        icon: "qrc:/images/leftPanel/summary.png" },
                { key: "report",   label: qsTr("Report"),       icon: "qrc:/images/leftPanel/match.png" },
                { key: "coach",    label: qsTr("Coach report"), icon: "qrc:/images/leftPanel/match.png" },
                { key: "mpi",      label: qsTr("Group / MPI"),  icon: "qrc:/images/leftPanel/target.png" },
                { key: "settings", label: qsTr("Settings"),     icon: "qrc:/images/leftPanel/settings.png" },
                { key: "home",     label: qsTr("Home"),         icon: "qrc:/images/leftPanel/home.png" }
            ]
            delegate: Rectangle {
                id: navBtn
                width: navColumn.width
                height: 38
                radius: 8
                color: navMouse.containsMouse ? "#26272c" : "transparent"
                border.color: modelData.key === "mpi" && isShowMPI ? "#e8003d" : "#3a3b40"
                border.width: 1
                Row {
                    anchors.left: parent.left; anchors.leftMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 9
                    Image {
                        source: modelData.icon
                        width: 16; height: 16
                        fillMode: Image.PreserveAspectFit
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: modelData.label
                        color: "#b9b9c0"
                        font.family: theme.fontFamily
                        font.pixelSize: 12
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
                cannotGenerate.text = qsTr("Coach report is coming soon")
                cannotGenerate.visible = true
            } else if (key === "mpi") {
                isShowMPI = !isShowMPI
            } else if (key === "settings") {
                settingsClicked()
            } else if (key === "home") {
                homeButtonClicked()
            }
        }
    }

    // Anchor for the SettingsPage popup (was the settings icon's position).
    Item {
        id: settingsNavBtn
        property real navY: navColumn.y + 4*46
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
        id: match_60_40_box
        visible: true
        source: "qrc:/images/leftPanel/match_60_40_box.png"
        x: ((parent.width/rootItemWidth)*24) - height/2
        y: ((parent.height/rootItemHeight)*181)-offsetDisplacement/2
        opacity: 1
        width: parent.width - (2*x) //((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)*2

        Rectangle {
            anchors.fill: parent
            color: "#e8003d"
            radius: 6
        }
    }
    Text {
        id: matchText
        width: ((parent.width/rootItemWidth)*match_60_40_box.sourceSize.width)
        height: ((parent.height/rootItemHeight)*match_60_40_box.sourceSize.height)
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: (height*1.2)

        text: "Dummy"
        color: "white"
        anchors.centerIn: match_60_40_box
        opacity: 1
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
        id: gameDisplayText1
        anchors.top: pistol_box_copy.top
        anchors.left: pistol_box_copy.left
        width: ((parent.height/rootItemHeight)*pistol_box_copy.sourceSize.width)
        height: ((parent.height/rootItemHeight)*pistol_box_copy.sourceSize.height)
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: (height)

        text: "Dummy"
        color: "white"
    }
    Image {
        id: gameDisplayImage
        source: gameDisplayText2.text == "PISTOL" ? "qrc:/images/loginPage/iconPistol.png" : "qrc:/images/loginPage/iconRifle.png"
        anchors.top: pistol_box.top
        anchors.left: pistol_box.left
        width: ((parent.height/rootItemHeight)*pistol_box.sourceSize.width)
        height: ((parent.height/rootItemHeight)*pistol_box.sourceSize.height)
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
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
        showSummaryPage.visible = true
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
