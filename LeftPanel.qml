import QtQuick 2.2
Item {
    property int rootItemWidth:200
    property int rootItemHeight:725

    property alias name: nameText.text
    property alias gameDisplay1: gameDisplayText1.text
    property alias gameDisplay2: gameDisplayText2.text
    property alias matchDisplay: matchText.text
    property alias settingsX:settings_unclicked.x
    property alias settingsY:settings_unclicked.y
    property alias settingsWidth:settings_unclicked.width


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


    Image {
        id: play
        visible: true
        source: "qrc:/images/leftPanel/play.png"
        //anchors.horizontalCenter: parent.horizontalCenter
        anchors.left: white_tile.left
        anchors.leftMargin: 3
        anchors.top: white_tile.bottom
        anchors.topMargin: 20
        opacity: 1
        width: ((rightPanel.width/rightPanel.rootItemWidth)*sourceSize.width)
        height: ((rightPanel.height/rightPanel.rootItemHeight)*sourceSize.height)

        MouseArea
        {
            anchors.fill: parent
            onClicked:
            {
                rightPanel.startClicked()
            }
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
    Image {
        id: home_hover
        visible: false
        source: "qrc:/images/leftPanel/home.png"
        x: ((parent.width/rootItemWidth)*38)
//        y: ((parent.height/rootItemHeight)*549)
        y: ((parent.height/rootItemHeight)*(549-offsetDisplacement))
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: home_clicked
        visible: false
        source: "qrc:/images/leftPanel/home.png"
        x: ((parent.width/rootItemWidth)*38)
//        y: ((parent.height/rootItemHeight)*549)
        y: ((parent.height/rootItemHeight)*(549-offsetDisplacement))
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }

//    Rectangle {
//        color: "black"//isShowMPI ? "lightblue" : "red"
//        width: home_clicked.width
//        x: home_clicked.x
//        y: home_clicked.y + 2.5*home_clicked.height
//        height: home_clicked.height - 10
////        visible: false

//        Text {
//            text: isShowMPI ? "Hide MPI" : "Show MPI"
//            width: implicitWidth
//            height: implicitHeight
//            anchors.centerIn: parent
//        }
//        Image {
//            id: mpiTarget
//            visible: true
//    //        visible: false
//            source: "qrc:/images/leftPanel/target.png"
//            width: ((parent.width/rootItemWidth)*sourceSize.width)
//            height: ((parent.height/rootItemHeight)*sourceSize.height)
//            opacity: 1
//        }

//        MouseArea {
//            anchors.fill: parent
//            onClicked: {
//                isShowMPI = !isShowMPI
//            }
//        }
//    }

    Image {
        id: mpi_unclicked
        visible: true
//        visible: false
        source: "qrc:/images/leftPanel/target.png"
        x: ((parent.width/rootItemWidth)*38)
//        y: ((parent.height/rootItemHeight)*549)
        y: ((parent.height/rootItemHeight)*(617-offsetDisplacement))
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
        MouseArea {
            anchors.fill: parent
            onClicked: {
                isShowMPI = !isShowMPI
            }
        }
    }

    Image {
        id: home_unclicked
        visible: true
//        visible: false
        source: "qrc:/images/leftPanel/home.png"
        x: ((parent.width/rootItemWidth)*38)
//        y: ((parent.height/rootItemHeight)*549)
        y: ((parent.height/rootItemHeight)*(549-offsetDisplacement))
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
        MouseArea {
            anchors.fill: parent
            onClicked: {
                console.log("testing ----------------- Home inactive")
                homeButtonClicked()
            }
        }
    }
    Image {
        id: settings_clicked
        visible: true
        source: "qrc:/images/leftPanel/settings.png"
        x: ((parent.width/rootItemWidth)*38)
//        y: ((parent.height/rootItemHeight)*481)
        y: ((parent.height/rootItemHeight)*(481-offsetDisplacement))
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: settings_hover
        visible: true
        source: "qrc:/images/leftPanel/settings.png"
        x: ((parent.width/rootItemWidth)*38)
//        y: ((parent.height/rootItemHeight)*481)
        y: ((parent.height/rootItemHeight)*(481-offsetDisplacement))
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: settings_unclicked
        visible: true
        source: "qrc:/images/leftPanel/settings.png"
        x: ((parent.width/rootItemWidth)*38)
//        y: ((parent.height/rootItemHeight)*481)
        y: ((parent.height/rootItemHeight)*(481-offsetDisplacement))
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
        MouseArea
        {
            anchors.fill: parent
            onClicked:
            {
                console.log("Settings Clicked")
                settingsClicked()
            }
        }
    }
    Image {
        id: match_report_clicked
        visible: false
        source: "qrc:/images/leftPanel/match.png"
        x: ((parent.width/rootItemWidth)*38)
//        y: ((parent.height/rootItemHeight)*415)
        y: ((parent.height/rootItemHeight)*(415-offsetDisplacement))
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: match_report_hover
        visible: false
        source: "qrc:/images/leftPanel/match.png"
        x: ((parent.width/rootItemWidth)*38)
//        y: ((parent.height/rootItemHeight)*415)
        y: ((parent.height/rootItemHeight)*(415-offsetDisplacement))
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: match_report_unclicked
        visible: true
        source: "qrc:/images/leftPanel/match.png"
        x: ((parent.width/rootItemWidth)*38)
//        y: ((parent.height/rootItemHeight)*415)
        y: ((parent.height/rootItemHeight)*(415-offsetDisplacement))
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
        MouseArea
        {
            anchors.fill: parent
            onClicked:
            {
                if(sligterMode)
                {
                    cannotGenerate.text = sighterMatchText
                    cannotGenerate.visible = true
                }
                else
                {
                    if(1/*globalModelOfData.count >= 10*/)
                    {
                        //if (!isSaveGame)
                            showMatchReport()
                        // only show summary for now
                        //showSummary()
                    }
                    else
                    {
                        cannotGenerate.text = minimumShotsMatchReport
                        cannotGenerate.visible = true
                    }
                }
            }
        }
    }
    Image {
        id: summery_clicked
        visible: true
        source: "qrc:/images/leftPanel/summary.png"
        x: ((parent.width/rootItemWidth)*38)
//        y: ((parent.height/rootItemHeight)*348)
        y: ((parent.height/rootItemHeight)*(348-offsetDisplacement))
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: summery_mouse_hover
        visible: false
        source: "qrc:/images/leftPanel/summary.png"
        x: ((parent.width/rootItemWidth)*38)
//        y: ((parent.height/rootItemHeight)*348)
        y: ((parent.height/rootItemHeight)*(348-offsetDisplacement))
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: summery_unclicked
        visible: true
        source: "qrc:/images/leftPanel/summary.png"
        x: ((parent.width/rootItemWidth)*38)
//        y: ((parent.height/rootItemHeight)*348)
        y: ((parent.height/rootItemHeight)*(348-offsetDisplacement))
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
        MouseArea{
            anchors.fill: parent
            onClicked:
            {
                if(sligterMode)
                {
                    cannotGenerate.text = sighterSummaryText
                    cannotGenerate.visible = true
                }
                else
                {
                    if (isSaveGame)
                        return
//                    if(globalModelOfData.count >= 10)
//                    {
                        showSummary()
//                    }
//                    else
//                    {
//                        cannotGenerate.text = minimumShotsSummary
//                        cannotGenerate.visible = true
//                    }
                }
            }
        }
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
        color: "black"
        font.bold: true
//        Rectangle {
//            anchors.fill: parent
//            color: "red"
//            opacity: 0.5
//        }
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
            color: "#0072BC"
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
        id: homeText
        x: home_clicked.x + (home_clicked.width/2) - (width/2) - 15
        y: home_clicked.y + (home_clicked.height/2) - (height/2)
        text : qsTr("Home")
        width: implicitWidth
        height: implicitHeight
        color: "white"
        font.pointSize: 10

        visible: home_unclicked.visible
        opacity: 0
    }
    Text {
        id: matchReportText
        x: match_report_clicked.x + (match_report_clicked.width/2) - (width/2) - 15
        y: match_report_clicked.y + (match_report_clicked.height/2) - (height/2)
        text : qsTr("Match report")
        width: implicitWidth
        height: implicitHeight
        color: "white"
        font.pointSize: 10
        opacity: 0
    }
    Text {
        id: settingText
        x: settings_clicked.x + (settings_clicked.width/2) - (width/2) - 15
        y: settings_clicked.y + (settings_clicked.height/2) - (height/2)
        text : qsTr("Settings")
        width: implicitWidth
        height: implicitHeight
        color: "white"
        font.pointSize: 10
        opacity: 0
    }
    Text {
        id: summaryText
        x: summery_clicked.x + (summery_clicked.width/2) - (width/2) - 15
        y: summery_clicked.y + (summery_clicked.height/2) - (height/2)
        text : qsTr("Summary")
        width: implicitWidth
        height: implicitHeight
        color: "white"
        font.pointSize: 10
        opacity: 0
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
        home_clicked.enabled = false;
        home_unclicked.enabled = false;
        settings_clicked.enabled = false;
        settings_unclicked.enabled = false;
//        homeButtonClicked().visible = false;
    }
}
