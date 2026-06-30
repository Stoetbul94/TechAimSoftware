import QtQuick 2.4
import QtQuick.Controls 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Window 2.2
//import QtQuick.Controls.Styles 1.3

Item {
    id: rootItem
    property int rootItemWidth:1366
    property int rootItemHeight:724

    property bool demoMode: true
    property bool connectToMaster: false
    property alias username_loginPage: name_text_field.text
    property int gameMode: 0 // 0 -> pistol, 1 -> rifel
    property int gameEvent: 0
    property int gameType: 1 // 1->sighter and 0 -> match
    property int papermode: 0
    property bool mod_connected: false
    property bool popupMode: false
    property bool showComportConnector: true
    property bool showLaneConnector: false
    property bool hideFreePractice: isDefaultIcon

    property string licColor: "darkgrey"
    signal loadSavedGame()
    signal sighterStartedFromServer()
    signal matchStartedFromServer()
    signal backHomeFromServer()

    onGameModeChanged: {
        APPSETTINGS.setGameMode(gameMode)
    }

    onGameEventChanged: {
        APPSETTINGS.setGameEvent(gameEvent)
    }

    onUsername_loginPageChanged: {
        console.log("**********??????????????????????*********", username_loginPage)
        APPSETTINGS.setUsername(username_loginPage)
    }

    onGameTypeChanged: {
        console.log("***************************************** ", gameType)
        if (gameType === 0)
            shootingPage.loadGameInMatchMode()
    }

    Component.onCompleted: {
        if (gameRange == 10) {
            if (APPSETTINGS.getGame_distance() < 5 || APPSETTINGS.getGame_distance() > 10) {
                gameDistanceDia.visible = true
            }
        }

        MODREADER.connectedModbus()
        mod_connected = MODREADER.isModBusConnected() //empty for auto detect
        if (!MODREADER.isValidLicence()) {
            //            invalidLicence.visible = true
        } else if (!mod_connected && popupMode) {
            modBusConnector.visible = true
        }

        name_text_field.text = MODREADER.getUserName()
        port_name_text_field.text = MODREADER.getPortNumber()
        netowrk_path_text.text = MODREADER.getNetworkPath()
        APPSETTINGS.setSetaSettingsFilePathFromQML(netowrk_path_text.text)
    }

    onVisibleChanged: {
        MODREADER.setOnLoginPage(visible)
    }

    ModConnectorDialog {
        id: modBusConnector
        width: 300
        height: 100
        //        x: parent.width/2 - width/2
        //        y: parent.height/2 - height/2
        visible: false
    }

    MessageDialog
    {
        id: invalidUserName
        title: "Warning"
        //text: popupMode && !port_name_text_field.visible ? "Please enter user name to login" : "Please enter a valid user name and port name."
        text: qsTr("Please enter user name to login")
        visible: false
    }

    MessageDialog
    {
        id: invalidLicence
        title: "Error"
        text: "Licence has expired, Please contact raosrinu2004@gmail.com or rahul.mishra@tachustechnology.com"
        visible: false

        onAccepted: {
            Qt.quit()
        }
    }

    MessageDialog
    {
        id: gameDistanceDia
        title: "Error"
        text: "Entered distance is not in the range of 5m to 10 m."
        visible: false

        onAccepted: {
            Qt.quit()
        }
    }

    MessageDialog
    {
        id: masterConnection
        title: "Error"
        text: "Master system is not connected, Please Click \"Connect\" button."
        visible: false
    }
    MessageDialog
    {
        id: validateLogin
        title: "Error"
        text: "Srinu"
        visible: false
    }

    MessageDialog
    {
        id: contactUsDia
        title: "Info"
        text: isDefaultIcon ? "Please contact us contact@tachustechnology.com"
                            : " Contact us on contact@seta-online.com"
        visible: false
    }

    Popup {
        id: popup
        width: 300
        height: 300
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

        x: parent.width/2 - width/2
        y: parent.height/2 - height/2

    }


    //Collect the conenction status and update
    Connections {
        target: APPSETTINGS
        onUserNameChanged : {
            username_loginPage = name
            console.log("*******************", name)
            name_text_field.text = name
        }

        onPortNumberChanged : {
            port_name_text_field.text = port
        }

        onLaneNumberChanged : {
           lane_number_text = lane_number
        }

        onStartSighter : {
            if (visible) {
                perfromStart()
            }

            sighterStartedFromServer()
        }

        onStartMatch : {
            if (visible) {
                perfromStart()
            }

            matchStartedFromServer()
        }

        onBackHome : {
            console.log("********************************", visible)
            if (!visible) {
                backHomeFromServer()
            }
        }
    }

    Connections {
        target: MODREADER
        onMasterConnectionChanged : {
            console.log("Master connection changed .....,",isConnected)
            disableControls();

        }
        onMatchDetails : {

            console.log("Match Details in qml .....",gametype,matchmode,sighterTime,matchtime,sigherTime,matchpf )
//            shootingPage.set
            gameEvent = matchmode
            gameMode = gametype
            shootingPage.applyServerSettings(sighterTime,matchtime,sigherTime,matchpf)
            //APPSETTINGS.setMotor_movement_time(matchpf, sigherTime)
        }
        onStartMatchFromServer : {
            console.log("Match Started .............")
            perfromStart()
        }

        onMatchDetailsSetaModification : {
            console.log("Match Details in qml onMatchDetailsSetaModification .....",gametype,matchmode)
            gameEvent = matchmode
            gameMode = gametype
        }

//        onShootCountChanged: {
//            if (globalModelOfData.count === shotCount) {
//                var logData = "Game over "+ shotCount
//                MODREADER.appendToLogFile(logData)
//                return
//            }
        }
    Rectangle {
        id: fullRect
        color: "#202020"
        anchors.fill: parent
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            gameEventList.visible = false
            userHistoryList.visible = false
            //papermodeList.visible = false
        }
    }

    Rectangle {
        id: bgRect
        width: bg.paintedWidth
        height: bg.paintedHeight
        color: "transparent"
        anchors.centerIn: parent
    }

    function validate()
    {
        if(username_loginPage === "" && !isSaveGame)
        {
            invalidUserName.visible = true
            return false
        }

        //        if (port_name_text_field.visible && port_name_text_field.text === "" && !popupMode)
        //        {
        //            invalidUserName.visible = true
        //            return false
        //        }

        return true
    }

    function reset()
    {
        username_loginPage = ""
        gameMode = 0
        gameEvent = 0
        papermode = 0
    }

    function getGameEventText(index)
    {
        var text = "gameEventText"
//        if (index === 0)
//            text = qsTr("10 Shots Match")
//        else if (index === 1)
//            text = qsTr("20 Shots Match")
//        else if (index === 2)
//            text = qsTr("30 Shots Match")
//        else if (index === 3)
//            text = qsTr("40 Shots Match")
//        else if (index === 4)
//            text = qsTr("60 Shots Match")
//        else
//            text = qsTr("Free Practice")

        // if 15 shhots
        if (APPSETTINGS.getIs15Shoot()) {
            if (index === 0)
                text = qsTr("10")
            else if (index === 1)
                text = qsTr("15")
            else if (index === 2)
                text = qsTr("20")
            else if (index === 3)
                text = qsTr("30")
            else if (index === 4)
                text = qsTr("40")
            else
                text = qsTr("Free Practice")

            return text;
        }
	 else {
            if (index === 0)
                text = qsTr("10")
            else if (index === 1)
                text = qsTr("20")
            else if (index === 2)
                text = qsTr("30")
            else if (index === 3)
                text = qsTr("40")
            else if (index === 4)
                text = qsTr("60")
            else
                text = qsTr("Free Practice")
            return text;
        }
    }

    function getPaperModeText(index)
    {
        //        if (index === 0)
        //            return "Standard"
        //        if (index === 1)
        //            return "Dual Shots"
        //        if (index === 2)
        //            return "Pro Mode"
        //        else
        //            return "Multiple Shots"

        if (index === 0)
            return "Standard"
        else
            return "Pro Mode"
    }

    Image {
        id: bgRectImg
        source: "qrc:/images/loginPage/bgRectImg.png"
        x: ((parent.width/rootItemWidth)*0)
        y: ((parent.height/rootItemHeight)*0)
        opacity: 1
        anchors.fill: parent
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: bg
        source: isDefaultIcon ? "qrc:/images/loginPage/bg_tachus.png" : "qrc:/images/loginPage/login_page_29Oct.png"
        x: ((parent.width/rootItemWidth)*0)
        y: ((parent.height/rootItemHeight)*0)
        opacity: 1
        anchors.fill: parent
        //        fillMode: Image.PreserveAspectFit
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: red
        source: "qrc:/images/loginPage/red.png"
        x: ((parent.width/rootItemWidth)*202)
        //y: ((parent.height/rootItemHeight)*291)
        anchors.top: shots_40_match.bottom
        anchors.topMargin: shots_40_match.height/3
        opacity: 0
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: green
        source: "qrc:/images/loginPage/green.png"
        x: ((parent.width/rootItemWidth)*202)
        //y: ((parent.height/rootItemHeight)*363)
        anchors.top: red.bottom
        opacity: 0
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
//    Image {
//        id: image_icon
//        source: "qrc:/images/loginPage/image_icon.png"
//        x: ((parent.width/rootItemWidth)*354)
//        y: ((parent.height/rootItemHeight)* 14)
//        opacity: 1
//        width: ((parent.width/rootItemWidth)*sourceSize.width)
//        height: ((parent.height/rootItemHeight)*sourceSize.height)
//    }
//    Image {
//        id: upload_image_icon
//        source: "qrc:/images/loginPage/upload_image_icon.png"
//        x: ((parent.width/rootItemWidth)*386)
//        y: ((parent.height/rootItemHeight)*91)
//        opacity: 1
//        width: ((parent.width/rootItemWidth)*sourceSize.width)
//        height: ((parent.height/rootItemHeight)*sourceSize.height)

//        visible: false

//        MouseArea {
//            property bool onItem: false
//            anchors.fill: parent
//            hoverEnabled: true
//            onEntered: {
//                onItem = true
//            }

//            onExited: {
//                onItem = false
//            }

//            ToolTip.visible: onItem
//            ToolTip.text: qsTr("Upload image")
//        }

//    }

    Image {
        id: name
        source: "qrc:/images/loginPage/name.png"
        x: ((parent.width/rootItemWidth)*290)
        y: ((parent.height/rootItemHeight)*100) - ((parent.height/rootItemHeight)*sourceSize.height)*0.05
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width*0.9)
        height: ((parent.height/rootItemHeight)*sourceSize.height)*1.1
        horizontalAlignment: Image.AlignHCenter
        verticalAlignment: Image.AlignHCenter

        Rectangle {
            anchors.fill: parent
            color: "#0072BC"
        }
    }

    Text {
        id: nameText
        text: "Name"
        //x: ctm_over.x
        anchors.right:name.left
        anchors.rightMargin: 15
        x: ((parent.width/rootItemWidth)*235)
        y: ((parent.height/rootItemHeight)*105)
        //anchors.topMargin:
       // anchors.verticalCenter: com_port_dummy_rect.verticalCenter
        font.bold: Text
        font.pointSize: 10
        visible: showComportConnector
    }

    TextInput {
        id: name_text_field
        anchors.right: name_drop_down.left
        anchors.left: name.left
        anchors.top: name.top
        anchors.bottom: name.bottom
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        anchors.topMargin: 5
        font.pixelSize: 0.8* height
        horizontalAlignment: TextInput.AlignHCenter
        font.bold: TextInput
        maximumLength: 20
        color: gameEventText.color
        onTextChanged: {
            username_loginPage = text
        }

//        onFocusChanged: {
//            if (cursorVisible)
//                userHistoryList.visible = true
//        }
    }

    Image {
        id: name_drop_down
        source: "qrc:/images/loginPage/combo_down.png"
        anchors.right: name.right
        anchors.top: name.top
        height: name.height
        width: name.height
        opacity: 1

        MouseArea {
            anchors.fill: parent
            onClicked: {
                if (APPSETTINGS.getUserHistoryCount()>0)
                    userHistoryList.visible = true
            }
        }
    }

    Image {
        id: demo
        source: "qrc:/images/loginPage/demo.png"
        x: ((parent.width/rootItemWidth)*211)
        y: ((parent.height/rootItemHeight)*480)
        opacity: demo_over.opacity === 1 ? 0 : 1
        width: ((bgRect.width/rootItemWidth)*sourceSize.width)
        height: ((bg.height/rootItemHeight)*sourceSize.height)
        visible: false
        //fillMode: Image.PreserveAspectFit
    }
    Image {
        id: demo_over
        source: "qrc:/images/loginPage/demo_over.png"
        x: ((parent.width/rootItemWidth)*211)
        y: ((parent.height/rootItemHeight)*480)

        opacity: demoMode ? 1 : 0
        width: ((bgRect.width/rootItemWidth)*sourceSize.width)
        height: ((bg.height/rootItemHeight)*sourceSize.height)
        visible: false
        //fillMode: Image.PreserveAspectFit
        MouseArea {
            id: demo_mouse
            anchors.fill: demo_over
            onClicked: {
                demoMode = !demoMode
            }
        }
    }

    Rectangle {
        id: com_port_dummy_rect
        anchors.fill: red
        color: "white"
    }

    Text {
        id: portNameLable
        text: "Port"
        x: ((parent.width/rootItemWidth)*235)
        y: ((parent.height/rootItemHeight)*110)
        anchors.right:portnamebg.left

        anchors.rightMargin: 20

        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignHCenter
        anchors.verticalCenter: com_port_dummy_rect.verticalCenter
        font.bold: Text
        font.pointSize: 10
        visible: showComportConnector
    }

    Image {
        id: portnamebg
        source: "qrc:/images/loginPage/name.png"
        x: ((parent.width/rootItemWidth)*235)
        y: ((parent.height/rootItemHeight)*100)
        anchors.left: lanenamebg.left
        //anchors.leftMargin: 20
         anchors.top: portNameLable.top
        horizontalAlignment: Image.AlignHCenter
        verticalAlignment: Image.AlignHCenter
       // anchors.topMargin: 5
        opacity: 1
        width: 100
        height: ((parent.height/rootItemHeight)*sourceSize.height)
        visible: showComportConnector
    }

    TextInput {
        id: port_name_text_field
        anchors.fill: portnamebg
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        anchors.topMargin: 5
        font.pixelSize: 0.5* height
        horizontalAlignment: TextInput.AlignHCenter
        font.bold: TextInput
        maximumLength: 5
        visible: portnamebg.visible
    }

    Rectangle {
        id: lane_dummy_rect
        anchors.fill: green
        color: "white"
    }

    Rectangle {
        id: temp_dummy_rect
        anchors.top: lane_dummy_rect.bottom
        anchors.left: lane_dummy_rect.left
        anchors.right: lane_dummy_rect.right
        anchors.bottom: reset_over.bottom
        color: "white"
    }

    Image {
        id: ctm
        source: "qrc:/images/loginPage/demo.png"
        x: demo_over.x
        //y: demo_over.y - 50
        anchors.verticalCenter: lane_dummy_rect.verticalCenter
        opacity: ctm_over.opacity === 1 ? 0 : 1
        width: ctm_over.width //((bgRect.width/rootItemWidth)*sourceSize.width)
        height: ctm_over.height//((bg.height/rootItemHeight)*sourceSize.height)
        visible: false //showLaneConnector
        //fillMode: Image.PreserveAspectFit
    }
    Image {
        id: ctm_over
        source: "qrc:/images/loginPage/demo_over.png"
        x: demo_over.x
//        y: demo_over.y - 50
        anchors.verticalCenter: lane_dummy_rect.verticalCenter

        opacity: 1/*connectToMaster ? 1 : 0*/
        width: ((bgRect.width/rootItemWidth)*sourceSize.width)
        height: ((bg.height/rootItemHeight)*sourceSize.height)
        visible: false//showLaneConnector
        //fillMode: Image.PreserveAspectFit
        MouseArea {
            id: ctm_mouse
            anchors.fill: ctm_over
            onClicked: {
                connectToMaster = !connectToMaster
            }
        }
    }

    Text {
        id: laneNameLable
        text: "Lane Name"
        anchors.left: ctm_over.right
        anchors.leftMargin: -40
        anchors.top: ctm_over.top
        anchors.topMargin: 5
        font.pointSize: 10
        visible: showLaneConnector
        opacity: 0
    }

    Image {
        id: lanenamebg
        source: "qrc:/images/loginPage/name.png"
        anchors.right: start.right
        anchors.rightMargin: 10
        anchors.top: laneNameLable.top
        anchors.topMargin: 5
        opacity: 0
        width: start.width*0.8
        height: ((parent.height/rootItemHeight)*sourceSize.height)
        visible: showLaneConnector
    }

    TextInput {
        id: lane_name_text_field
        anchors.fill: lanenamebg
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        anchors.topMargin: 5
        font.pixelSize: 0.5* height
        horizontalAlignment: TextInput.AlignHCenter
        visible: lanenamebg.visible
        opacity: lanenamebg.opacity
    }

    Image {
        id: masterConnectBtn
        source: "qrc:/images/loginPage/start.png"
//        anchors.left: reset_over.left
//        anchors.top: lanenamebg.top
//        anchors.topMargin: -5
        anchors.verticalCenter: lanenamebg.verticalCenter
        anchors.horizontalCenter: start.horizontalCenter
        height: start.height
        width: start.width
        opacity: 1
//        width: reset_over.width
//        height: ((parent.height/rootItemHeight)*sourceSize.height)
        visible: showLaneConnector

        MouseArea {
            anchors.fill: parent
            onClicked: {
                console.log("Connect clicked ", name_text_field.text)
                if (name_text_field.text == "")
                {
                    masterConnection.text = "Please enter a valid shooter name."
                    masterConnection.visible = true
                    return;
                }

                MODREADER.connectToMaster(name_text_field.text)
            }
        }
    }
    Text {
        id: masterCntText
        x: masterConnectBtn.x + (masterConnectBtn.width/2) - (width/2)
        y: masterConnectBtn.y + (masterConnectBtn.height/2) - (height/2) - 2
        text : isDefaultIcon ? qsTr("Connect to TCMA") : qsTr("Connect to SCMA")
        width: implicitWidth
        height: implicitHeight
        color: "white"
        font.pointSize: 12
        visible: masterConnectBtn.visible
    }

    Image {
        id: pistol
        source: "qrc:/images/loginPage/pistol.png"
        x: ((parent.width/rootItemWidth)*229)
        y: ((bgRectImg.height/rootItemHeight)*212)
        opacity: pistol_over.opacity === 1 ? 0 : 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: pistol_over
        source: "qrc:/images/loginPage/pistol_over.png"
        x: ((parent.width/rootItemWidth)*229)
        y: ((bgRectImg.height/rootItemHeight)*212)
        opacity: gameMode === 0 ? 1 : 0
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }

    MouseArea {
        id: pistolMouse
        anchors.fill: pistol_over
        onClicked : {
            papermode = 0
            gameMode = 0
        }
    }

    Image {
        id: rifle
        source: "qrc:/images/loginPage/rifle.png"
        x: ((parent.width/rootItemWidth)*403)
        y: ((parent.height/rootItemHeight)*212)
        opacity: rifle_over.opacity === 1 ? 0 : 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: rifle_over
        source: "qrc:/images/loginPage/rifle_over.png"
        x: ((parent.width/rootItemWidth)*403)
        y: ((parent.height/rootItemHeight)*(192+ 20))
        opacity: gameMode === 1 ? 1 : 0
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    MouseArea {
        id: rifleMouse
        anchors.fill: rifle_over
        onClicked: {
            papermode = 0
            gameMode = 1
        }
    }

    Image {
        id: shots_40_match
        source: "qrc:/images/loginPage/shots_40_match.png"
        x: ((parent.width/rootItemWidth)*299)
        y: ((parent.height/rootItemHeight)*269)
        opacity: 1
        width: ((bgRect.width/rootItemWidth)*sourceSize.width)
        height: ((bg.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: shots_40_match_text_field
        source: "qrc:/images/loginPage/shots_40_match_text_field.png"
        x: ((parent.width/rootItemWidth)*299)
        y: ((parent.height/rootItemHeight)*269)
        opacity: 0
        //        width: 0.75 * shots_40_match.width
        //        height: shots_40_match.height
        width: ((bgRect.width/rootItemWidth)*sourceSize.width)
        height: ((bg.height/rootItemHeight)*sourceSize.height)
    }

    Text {
        id: gameEventText
        width: implicitWidth
        height: implicitHeight
        anchors.verticalCenter: shots_40_match.verticalCenter
        anchors.horizontalCenter: shots_40_match.horizontalCenter
        anchors.horizontalCenterOffset: -0.15*shots_40_match.width

        text : getGameEventText(gameEvent)
        color: "white"
        font.pixelSize: (0.6*shots_40_match.height)
    }

    MouseArea {
        id: gameEventMouse
        anchors.fill: shots_40_match
        onClicked: {
            //papermodeList.visible = false
            gameEventList.visible = true
        }
    }

    ListView {
        id: gameEventList
        anchors.top: shots_40_match.bottom
        anchors.topMargin: 5
        anchors.left: shots_40_match.left
        width: shots_40_match.width
        height: 6*shots_40_match.height
        visible: false
        z: 10

        model:6

        delegate: Rectangle {
            width: parent.width
            height: shots_40_match.height
            border.width: 1
            border.color: "black"
            color: gameEvent === index ? "red" : "#2698d5"

            visible: (hideFreePractice && index === gameEventList.count - 1) ? false : true

            onVisibleChanged: {
                color = (gameEvent === index) ? "red" : "#2698d5"
            }

            Text {
                width: implicitWidth
                height: implicitHeight
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
                text: getGameEventText(index)
                color: "white"
                font.pixelSize: (0.8*parent.height)
            }
            MouseArea {
                anchors.fill: parent
                hoverEnabled: true

                onEntered: {
                    parent.color = "green"
                }

                onExited: {
                    parent.color = (gameEvent === index) ? "red" : "#2698d5"
                }

                onClicked: {
                    gameEvent = index
                    gameEventList.visible = false
                }
            }
        }
    }

    ListView {
        id: userHistoryList
        anchors.top: name.bottom
        anchors.topMargin: 5
        anchors.right: name_drop_down.left
        width: name.width - name_drop_down.width
        height: 3*name.height
        visible: false
        clip: true
        z: 10

        model: APPSETTINGS.getUserHistoryCount()

        ScrollBar.vertical: ScrollBar {}

        onVisibleChanged: {
            model = 0
            model = APPSETTINGS.getUserHistoryCount()
        }

        delegate: Rectangle {
            width: parent.width
            height: name.height
            border.width: 1
            border.color: "black"
            color: gameEvent === index ? "red" : "#2698d5"

            onVisibleChanged: {
                color = (gameEvent === index) ? "red" : "#2698d5"
            }

            Text {
                id: userHistoryText
                width: implicitWidth
                height: implicitHeight
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
                text: APPSETTINGS.getUserHistoryData(index)
                color: "white"
            }
            MouseArea {
                anchors.fill: parent
                hoverEnabled: true

                onEntered: {
                    parent.color = "green"
                }

                onExited: {
                    parent.color = (gameEvent === index) ? "red" : "#2698d5"
                }

                onClicked: {
                    username_loginPage = userHistoryText.text
                    userHistoryList.visible = false
                }
            }
        }
    }

    //    Image {
    //        id: standard
    //        source: "qrc:/images/loginPage/standard.png"
    //        x: ((parent.width/rootItemWidth)*300)
    //        y: ((parent.height/rootItemHeight)*375)
    //        opacity: gameMode === 0 ? 1 : 0
    //        width: ((bgRect.width/rootItemWidth)*sourceSize.width)
    //        height: ((bg.height/rootItemHeight)*sourceSize.height)
    //    }
    //    Image {
    //        id: standard1
    //        source: "qrc:/images/loginPage/standard1.png"
    //        x: ((parent.width/rootItemWidth)*300)
    //        y: ((parent.height/rootItemHeight)*375)
    //        opacity: 0//standard.opacity === 1 ? 0 : 1
    //        width: ((bgRect.width/rootItemWidth)*sourceSize.width)
    //        height: ((bg.height/rootItemHeight)*sourceSize.height)
    //    }
    //    Image {
    //        id: standard_text_field
    //        source: "qrc:/images/loginPage/standard_text_field.png"
    //        x: ((parent.width/rootItemWidth)*300)
    //        y: ((parent.height/rootItemHeight)*375)
    //        opacity: standard.opacity === 1 ? 0 : 1
    //        width: ((bgRect.width/rootItemWidth)*sourceSize.width)
    //        height: ((bg.height/rootItemHeight)*sourceSize.height)
    //    }

    //    Text {
    //        id: paperModeText
    //        width: implicitWidth
    //        height: implicitHeight
    //        anchors.verticalCenter: standard_text_field.verticalCenter
    //        anchors.horizontalCenter: standard_text_field.horizontalCenter

    //        text : getPaperModeText(papermode)
    //        color: "white"
    //    }

    //    MouseArea {
    //        anchors.fill: standard
    //        onClicked: {
    //            if (gameMode === 0)
    //            {
    //                papermodeList.visible = true
    //                gameEventList.visible = false
    //            }
    //        }
    //    }

    //    ListView {
    //        id: papermodeList
    //        anchors.top: standard_text_field.bottom
    //        anchors.topMargin: 2
    //        anchors.left: standard_text_field.left
    //        width: standard_text_field.width
    //        visible: false
    //        z: 10

    //        model: 2

    //        height: papermodeList.count * standard_text_field.height

    //        delegate: Rectangle {
    //            width: parent.width
    //            height: standard_text_field.height
    //            border.width: 1
    //            border.color: "black"
    //            color: papermode === index ? "red" : "#2698d5"

    //            onVisibleChanged: {
    //                color = (papermode === index) ? "red" : "#2698d5"
    //            }

    //            Text {
    //                width: implicitWidth
    //                height: implicitHeight
    //                anchors.verticalCenter: parent.verticalCenter
    //                anchors.horizontalCenter: parent.horizontalCenter
    //                text: getPaperModeText(index)
    //                color: "white"
    //            }
    //            MouseArea {
    //                anchors.fill: parent
    //                hoverEnabled: true

    //                onEntered: {
    //                    parent.color = "green"
    //                }

    //                onExited: {
    //                    parent.color = (papermode === index) ? "red" : "#2698d5"
    //                }

    //                onClicked: {
    //                    papermode = index
    //                    papermodeList.visible = false
    //                }
    //            }
    //        }
    //    }

    Image {
        id: start
        source: "qrc:/images/loginPage/start.png"
        x: ((parent.width/rootItemWidth)*317)
        y: ((parent.height/rootItemHeight)*466)
        opacity: 1
        width: ((bgRect.width/rootItemWidth)*sourceSize.width)
        height: ((bg.height/rootItemHeight)*sourceSize.height)
    }

    MouseArea {
        id: startMouse
        anchors.fill: start
        onClicked: {
            if (!appMode) // in demo mode
            {
                MODREADER.appendToLogFile("Application running in demo mode")
                if (connectToMaster && !MODREADER.isMasterSystemConnected()) {
                    console.log(connectToMaster + " --- " + !MODREADER.isMasterSystemConnected())
                    // show message that master system is not connected
                    masterConnection.text = "Master system is not connected, Please Click \"Connect\" button."
                    masterConnection.visible = true
                    return;
                }
                rootItem.visible = false
            } else {
                MODREADER.appendToLogFile("Application running in Live mode")
                if (connectToMaster && !MODREADER.isMasterSystemConnected()) {
                    MODREADER.appendToLogFile("Master application required")
                    // show message that master system is not connected
                    masterConnection.text = "Master system is not connected, Please Click \"Connect\" button."
                    masterConnection.visible = true
                    return;
                }

                if (masterConnectBtn && port_name_text_field.text != "")
                {
                    MODREADER.appendToLogFile("Application with port text field")
                    MODREADER.connectedModbus(port_name_text_field.text)
                    mod_connected = MODREADER.isModBusConnected()
                }

                if (!MODREADER.isModBusConnected()) // we need validation only if port are connected
                {
                    MODREADER.appendToLogFile("Com port not connected")
                    validateLogin.text = "Com port not connected"
                    validateLogin.visible = true
                    //if (popupMode)
                    //modBusConnector.visible = true
                    // else TextInput is provided to given the port name
                }else if (!MODREADER.isHardwareConnected()) {
                    validateLogin.text = "Hardware not connected."
                    validateLogin.visible = true
                }else if (!MODREADER.checkAutoFeedMode()) {
                    validateLogin.text = "Auto feed mode is off"
                    validateLogin.visible = false
                }else if (validate()) {
                    MODREADER.appendToLogFile("Validation was successful")
                    rootItem.visible = false
                } else {
                    MODREADER.appendToLogFile("Com-port connected but validation failed")
                }
            }
            //APPSETTINGS.autoSaveMatch()
            APPSETTINGS.saveMatch(true)
            APPSETTINGS.updateUserHistoryData(name_text_field.text)
            MODREADER.saveNameAndPort(name_text_field.text, port_name_text_field.text)
        }
        onPressed: {
            start.visible = false
            start_over.visible = true
        }
        onPressAndHold: {
            start.visible = false
            start_over.visible = true
        }
        onReleased: {
            start.visible = true
            start_over.visible = false
            if (mod_connected)
            {
                MODREADER.on_pushButton_clicked();
                MODREADER.on_pushButton_2_clicked();
            }
            MODREADER.resetShootinCount()
        }
    }

    function startButtonClickedOnLoadGame()
    {
        console.log("app mode "+appMode)
        //this function is only called during loadSavedGame()
        // so called loadSavedGame() before calling rootItem.visible = false
        if (!appMode) // in demo mode
        {
            rootItem.visible = false
        } else {
            //            if (!popupMode && port_name_text_field.text != "")
            //            {
            //                MODREADER.connectedModbus(port_name_text_field.text)
            //                mod_connected = MODREADER.isModBusConnected()
            //            }

            if (!mod_connected) // we need validation only if port are connected
            {
                if (popupMode)
                    modBusConnector.visible = true
                // else TextInput is provided to given the port name
            }else if (validate()) {
                rootItem.visible = false
            }
        }
    }

    Image {
        id: start_over
        source: "qrc:/images/loginPage/start_over.png"
        x: ((parent.width/rootItemWidth)*317)
        y: ((parent.height/rootItemHeight)*466)
        opacity: 1
        width: ((bgRect.width/rootItemWidth)*sourceSize.width)
        height: ((bg.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: device_conhnected
        source: "qrc:/images/loginPage/device_conhnected.png"
        x: ((parent.width/rootItemWidth)*35)
        y: ((parent.height/rootItemHeight)*648)
        opacity: mod_connected ? 1 : 0
        width: ((bgRect.width/rootItemWidth)*sourceSize.width)
        height: ((bg.height/rootItemHeight)*sourceSize.height)

        visible: false
    }
    Image {
        id: device_conhnected_blue
        source: "qrc:/images/loginPage/device_conhnected_blue.png"
        x: ((parent.width/rootItemWidth)*35)
        y: ((parent.height/rootItemHeight)*648)
        opacity: mod_connected ? 0 : 1
        width: ((bgRect.width/rootItemWidth)*sourceSize.width)
        height: ((bg.height/rootItemHeight)*sourceSize.height)

        visible: device_conhnected.visible

        MouseArea {
            anchors.fill: parent
            onClicked: {
                if (!mod_connected)
                {
                    if (popupMode)
                    {
                        MODREADER.connectedModbus()
                        mod_connected = MODREADER.isModBusConnected()
                        if (!mod_connected)
                            modBusConnector.visible = true
                    }
                } else {
                    MODREADER.disconnectModbus()
                    mod_connected = MODREADER.isModBusConnected()
                }
            }
        }
    }

    Image {
        id: license_details
        source: "qrc:/images/loginPage/license_details.png"
        x: ((parent.width/rootItemWidth)*259)
        y: ((parent.height/rootItemHeight)*646)
        opacity: 1
        width: ((bgRect.width/rootItemWidth)*sourceSize.width)
        height: ((bg.height/rootItemHeight)*sourceSize.height)

        visible: false
    }
    //    MouseArea {
    //        anchors.fill: license_details
    //        onClicked: {
    //        }
    //        onPressed: {
    //            license_details.visible = false
    //            license_details_over.visible = true
    //        }
    //        onPressAndHold: {
    //            license_details.visible = false
    //            license_details_over.visible = true
    //        }
    //        onReleased: {
    //            license_details.visible = true
    //            license_details_over.visible = false
    //        }
    //    }

    Image {
        id: license_details_over
        source: "qrc:/images/loginPage/license_details_over.png"
        x: ((parent.width/rootItemWidth)*259)
        y: ((parent.height/rootItemHeight)*646)
        opacity: 1
        visible: false
        width: ((bgRect.width/rootItemWidth)*sourceSize.width)
        height: ((bg.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: open_saved_files
        source: "qrc:/images/loginPage/open_saved_files.png"
        x: !device_conhnected.visible ? ((parent.width/rootItemWidth)*35) : ((parent.width/rootItemWidth)*411)
        y: ((parent.height/rootItemHeight)*646)
        opacity: 0
        width: ((bgRect.width/rootItemWidth)*sourceSize.width)
        height: ((bg.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: open_saved_files_crop
        source: "qrc:/images/loginPage/save_29Oct.png"
//        x: !device_conhnected.visible ? ((parent.width/rootItemWidth)*35) : ((parent.width/rootItemWidth)*411)
//        y: ((parent.height/rootItemHeight)*646)
        anchors.left: open_saved_files.left
        anchors.leftMargin: 10
        anchors.verticalCenter: open_saved_files.verticalCenter
        opacity: 1
        width: ((bgRect.width/rootItemWidth)*sourceSize.width)
        height: ((bg.height/rootItemHeight)*sourceSize.height)
    }
    MouseArea {
        anchors.fill: open_saved_files_crop
        onClicked: {
            APPSETTINGS.uploadGame()
            username_loginPage = APPSETTINGS.getUserName()
            gameMode = APPSETTINGS.getGameMode()
            gameEvent = APPSETTINGS.getGameEvent()
            gameType = APPSETTINGS.getGameType();

            if (gameType == 0) // 1 for sighter and 0 for Match
                MODREADER.changeSighterMode(false)

            console.log(" srinivas --- ", gameType)

            if (userName != "")
            {
                isSaveGame = true
                startButtonClickedOnLoadGame()
            }
        }
        onPressed: {
            open_saved_files.visible = false
            open_saved_files_over.visible = true
        }
        onPressAndHold: {
            open_saved_files.visible = false
            open_saved_files_over.visible = true
        }
        onReleased: {
            open_saved_files.visible = true
            open_saved_files_over.visible = false
        }
    }
    Rectangle {
        id: open_network_files
//        x: !device_conhnected.visible ? ((parent.width/rootItemWidth)*35) : ((parent.width/rootItemWidth)*411)
//        y: ((parent.height/rootItemHeight)*646)
        anchors.left: networkSwitch.right
        anchors.leftMargin: 10

        anchors.verticalCenter: open_saved_files.verticalCenter

        opacity: 1
        height: open_saved_files.height * 0.9
        width: height //((bgRect.width/rootItemWidth)*sourceSize.width)

        color: "transparent" //"#0093F4"
        visible: networkSwitch.checked

        Image {
            source: "qrc:/images/loginPage/network.png"
            anchors.centerIn: parent

            width: 0.95*parent.width
            height: width
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                netowrk_path_text.text = APPSETTINGS.selectSetaSettingsFile()
            }
        }
    }

    Text {
        id: netowrk_path_text
        anchors.left: open_network_files.right
        anchors.leftMargin: 10
        width: parent.width/4
        anchors.verticalCenter: open_network_files.verticalCenter

        text: ""
        visible: open_network_files.visible
    }

    Switch {
        id: networkSwitch
        anchors.left: open_saved_files_crop.right
        anchors.verticalCenter: open_network_files.verticalCenter
        text: checked ? qsTr("") : qsTr("No Network")
        checked: true

        onCheckedChanged: {
            MODREADER.setIsServerNetworkEnabled(checked)
        }
    }

    Image {
        id: open_saved_files_over
        source: "qrc:/images/loginPage/open_saved_files_over.png"
        x: open_saved_files.x
        y: ((parent.height/rootItemHeight)*646)
        opacity: 1
        visible: false
        width: ((bgRect.width/rootItemWidth)*sourceSize.width)
        height: ((bg.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: user_guide
        source: "qrc:/images/loginPage/user_guide.png"
        x: ((parent.width/rootItemWidth)*1004)
        y: ((parent.height/rootItemHeight)*646)
        opacity: 0
        width: ((bgRect.width/rootItemWidth)*sourceSize.width)
        height: ((bg.height/rootItemHeight)*sourceSize.height)
    }
    //    MouseArea {
    //        anchors.fill: user_guide
    //        onClicked: {
    //        }
    //        onPressed: {
    //            user_guide.visible = false
    //            user_guide_over.visible = true
    //        }
    //        onPressAndHold: {
    //            user_guide.visible = false
    //            user_guide_over.visible = true
    //        }
    //        onReleased: {
    //            user_guide.visible = true
    //            user_guide_over.visible = false
    //        }
    //    }
    Image {
        id: user_guide_over
        source: "qrc:/images/loginPage/user_guide_over.png"
        x: ((parent.width/rootItemWidth)*1004)
        y: ((parent.height/rootItemHeight)*646)
        opacity: 1
        visible: false
        width: ((bgRect.width/rootItemWidth)*sourceSize.width)
        height: ((bg.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: reset
        source: "qrc:/images/loginPage/reset.png"
        x: ((parent.width/rootItemWidth)*510)
        y: ((parent.height/rootItemHeight)*480)
        opacity: 0
        width: ((bgRect.width/rootItemWidth)*sourceSize.width)
        height: ((bg.height/rootItemHeight)*sourceSize.height)

        MouseArea {
            id:resetMouse
            anchors.fill: parent
            onClicked: rootItem.reset()
            onPressed: {
                reset.visible = false
                reset_over.visible = true
            }
            onPressAndHold: {
                reset.visible = false
                reset_over.visible = true
            }
            onReleased: {
                reset.visible = true
                reset_over.visible = false
            }
        }
    }

    Image {
        id: reset_over
        source: "qrc:/images/loginPage/reset_over.png"
        x: ((parent.width/rootItemWidth)*510)
        y: ((parent.height/rootItemHeight)*480)
        opacity: 0
        width: ((bgRect.width/rootItemWidth)*sourceSize.width)
        height: ((bg.height/rootItemHeight)*sourceSize.height)
    }

    Image {
        id: contact_us
        source: "qrc:/images/loginPage/contact_us.png"
        x: ((parent.width/rootItemWidth)*1185)
        y: ((parent.height/rootItemHeight)*646)
        opacity: 0
        width: ((bgRect.width/rootItemWidth)*sourceSize.width)
        height: ((bg.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: contact_us_crop
        source: "qrc:/images/loginPage/Contact us_29Oct.png"
        anchors.right: contact_us.right
        anchors.rightMargin: 6
        anchors.top: contact_us.top
        anchors.bottom: contact_us.bottom
        anchors.bottomMargin: 2
        opacity: 1
        width: 30 //((bgRect.width/rootItemWidth)*sourceSize.width)
        height: 30 //((bg.height/rootItemHeight)*sourceSize.height)

//        Rectangle{
//            anchors.fill: parent
//            color: "red"
//            opacity: 0.2
//        }
    }
    MouseArea {
        anchors.fill: contact_us_crop
        onClicked: {
            console.log("contact clicked................")
            contactUsDia.visible = true
        }

        onPressed: {
            contact_us.visible = false
            contact_us_over.visible = true
        }
        onPressAndHold: {
            contact_us.visible = false
            contact_us_over.visible = true
        }
        onReleased: {
            contact_us.visible = true
            contact_us_over.visible = false
        }
    }
    Image {
        id: contact_us_over
        source: "qrc:/images/loginPage/contact_us_over.png"
        x: ((parent.width/rootItemWidth)*1185)
        y: ((parent.height/rootItemHeight)*646)
        opacity: 0
        visible: false
        width: ((bgRect.width/rootItemWidth)*sourceSize.width)
        height: ((bg.height/rootItemHeight)*sourceSize.height)
    }

    // png text
    Text {
        id: eventText
        x: parent.width/5.8
        y: parent.height/4.26
        text : qsTr("EVENT")
        width: implicitWidth
        height: implicitHeight
        color: "black"
        font.pointSize: 14
        visible: false
    }

    Text {
        id: pistolText
        //        x: parent.width/5.1
        //        y: parent.height/3.35
        x: pistol.x + (pistol.width/2) - (width/2)
        y: pistol.y + (pistol.height/2) - (height/2) - 2
        text : qsTr("PISTOL")
        width: implicitWidth
        height: implicitHeight
        color: gameMode == 0 ? "white" : "grey"
        font.pointSize: 18
        font.bold: true
        opacity: 0
    }
    Image {
        id: pistol_img
        source: "qrc:/images/loginPage/iconPistol.png"
        anchors.fill: pistolText
        anchors.centerIn: pistolText
        opacity: 1
        width: ((bgRect.width/rootItemWidth)*sourceSize.width)
        height: ((bg.height/rootItemHeight)*sourceSize.height)
    }

    Text {
        id: rifleText
        //        x: parent.width/3
        //        y: parent.height/3.35
        x: rifle.x + (rifle.width/2) - (width/2)
        y: rifle.y + (rifle.height/2) - (height/2) - 2
        text : qsTr("RIFLE")
        width: implicitWidth
        height: implicitHeight
        color: gameMode == 1 ? "white" : "grey"
        font.pointSize: 18
        font.bold: true
        opacity: 0
    }
    Image {
        id: rifle_img
        source: "qrc:/images/loginPage/iconRifle.png"
        anchors.fill: rifleText
        anchors.centerIn: rifleText
        opacity: 1
        width: ((bgRect.width/rootItemWidth)*sourceSize.width)
        height: ((bg.height/rootItemHeight)*sourceSize.height)
    }

    Text {
        id: startText
        //        x: parent.width/3.7
        //        y: parent.height/1.5
        x: start.x + (start.width/2) - (width/2)
        y: start.y + (start.height/2) - (height/2) - 2
        text : qsTr("START")
        width: implicitWidth
        height: implicitHeight
        color: "white"
        font.pointSize: 20
    }
    Text {
        id: restartText
        x: reset.x + (reset.width/2) - (width/2)
        y: reset.y + (reset.height/2) - (height/2) - 2
        text : qsTr("RESET")
        width: implicitWidth
        height: implicitHeight
        color: "white"
        font.pointSize: 12
    }
    Text {
        id: dConnectionText
        x: device_conhnected.x + (device_conhnected.width/2) - (width/2) - 10
        y: device_conhnected.y + (device_conhnected.height/2) - (height/2) - 2
        text : device_conhnected.opacity == 1 ? qsTr("Device connected") : qsTr("Device not connected")
        width: implicitWidth
        height: implicitHeight
        color: "white"
        font.pointSize: 10
        font.bold: true

        visible: device_conhnected.visible
    }
    Text {
        id: licenseText
        x: license_details.x + 10
        y: license_details.y + (license_details.height/2) - (height/2) - 2
        text : qsTr("License")
        width: implicitWidth
        height: implicitHeight
        color: "white"
        font.pointSize: 10
        font.bold: true

        visible: license_details.visible
    }

    Text {
        id: saveFileText
        x: open_saved_files.x + 10
        y: open_saved_files.y + (open_saved_files.height/2) - (height/2) - 2
        text : qsTr("Saved files")
        width: implicitWidth
        height: implicitHeight
        color: "white"
        font.pointSize: 10
        font.bold: true
        opacity: open_saved_files.opacity
    }

    //    Rectangle {
    //        anchors.verticalCenter: open_saved_files.verticalCenter
    //        anchors.left: open_saved_files.right
    //        anchors.leftMargin: 5
    //        anchors.right: contact_us.left
    //        anchors.rightMargin: 5
    //        color: "lightgrey"
    //        height: saveFileText.height
    //    }

    Text {
        id: userGuideText
        x: user_guide.x + 10
        y: user_guide.y + (user_guide.height/2) - (height/2) - 2
        text : qsTr("User guide")
        width: implicitWidth
        height: implicitHeight
        color: "white"
        font.pointSize: 10
        font.bold: true
        opacity: user_guide.opacity
    }
    Text {
        id: contactText
        x: contact_us.x + 10
        y: contact_us.y + (contact_us.height/2) - (height/2) - 2
        text : qsTr("Contact us")
        width: implicitWidth
        height: implicitHeight
        color: "white"
        font.pointSize: 10
        font.bold: true
        opacity: contact_us.opacity
    }

    Rectangle {
        visible: eulaPage.visible
        color: "lightgrey"
        anchors.fill: parent
    }

    Rectangle {
        id: eulaPage
        width: 600
        height: 400
        anchors.centerIn: parent
        visible: !APPSETTINGS.isEulaAccepted() || !MODREADER.isValidLicence()
        //        visible: false

        ScrollView {
            id: eulaScroll
            anchors.fill: parent
            anchors.bottomMargin: 20
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            contentHeight: isDefaultIcon ? eulaFirstImage.height
                                         : eulaFirstImage.height + eulaSecondImage.height
            clip: true

            Image {
                id: eulaFirstImage

                anchors.top: parent.top
                anchors.left: parent.left
                width: 500
                height: 900
                source: isDefaultIcon ? "qrc:/images/loginPage/End User Agreement Tachus-1.png"
                                      : "qrc:/images/loginPage/End User Agreement SETA-1.png";
                clip: true
            }
            Image {
                id: eulaSecondImage

                anchors.top: eulaFirstImage.bottom
                anchors.left: parent.left
                width: 500
                height: 400
                source: "qrc:/images/loginPage/End User Agreement SETA-2.png";
                clip: true
                visible: !isDefaultIcon
            }
        }

        //        Button {
        //            anchors.top: eulaScroll.bottom
        //            anchors.right: parent.right
        //            width: 50
        //            height: 20

        //            text: qsTr("Accept")
        //            onClicked: {
        //                APPSETTINGS.eulaAccepted()
        //                eulaPage.visible = false
        //            }

        //            style: ButtonStyle {
        //                background: Image {
        //                    id: acceptButtonBG
        //                    source: "qrc:/images/loginPage/reset.png"
        //                    anchors.fill: parent
        //                }
        //            }
        //        }
        Image {
            id: acceptBtn
            source: "qrc:/images/loginPage/reset.png"
            anchors.top: eulaScroll.bottom
            anchors.right: parent.right
            width: 50
            height: 20
            opacity: 1
            //            width: 90
            //            height: ((parent.height/rootItemHeight)*sourceSize.height)

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    APPSETTINGS.eulaAccepted()
                    eulaPage.visible = false
                }
            }
        }
        Text {
            id: acceptBTNText
            x: acceptBtn.x + (acceptBtn.width/2) - (width/2)
            y: acceptBtn.y + (acceptBtn.height/2) - (height/2) - 2
            text : qsTr("Accept")
            width: implicitWidth
            height: implicitHeight
            color: "white"
            font.pointSize: 12
        }
    }

    Rectangle {
        id: licRect
        width: eulaPage.width
        height: eulaPage.height
        x: eulaPage.x
        y: eulaPage.y
        visible: !MODREADER.isValidLicence()
        //visible: false
        color: "lightgrey"

        Rectangle {
            width: 200
            height: 120
            anchors.centerIn: parent
            color: "transparent"
            border.color: licColor
            Rectangle {
                id: licHeaderRect
                color: licColor
                width: parent.width
                height: 30
                anchors.top: parent.top

                Text {
                    id: licHeader
                    text: qsTr("Lincence verification Process")
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    color: "white"
                }
            }

            Rectangle {
                id: emailLabelRect
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.top: licHeaderRect.bottom
                anchors.topMargin: 20
                width: licEmail.width
                height: 20
                color: "transparent"
                Text {
                    id: licEmail
                    text: qsTr("e-mail id")
                    height: implicitHeight
                    width: implicitWidth
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                }
            }

            Rectangle {
                id: errorLabelRect
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.top: emailLabelRect.bottom
                anchors.topMargin: 5
                width: licError.width
                height: 20
                color: "transparent"
                Text {
                    id: licError
                    text: qsTr("Error")
                    height: implicitHeight
                    width: implicitWidth
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    visible: false
                    color: "red"
                }
            }

            TextField {
                id: licTextInput
                width: parent.width - emailLabelRect.width - 30
                height: 20
                anchors.top: licHeaderRect.bottom
                anchors.topMargin: emailLabelRect.anchors.topMargin
                anchors.left: emailLabelRect.right
                anchors.leftMargin: 10
                placeholderText: "Please enter Licenced user id"
            }

            Rectangle {
                id: cancelButton
                width: 50
                height: 20
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 10
                color: licColor

                Text {
                    text: qsTr("Cancel")
                    anchors.centerIn: parent
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        Qt.quit()
                    }
                }
            }

            Rectangle {
                id: validateButton
                width: 70
                height: 20
                anchors.right: cancelButton.left
                anchors.rightMargin: 10
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 10
                color: licColor

                Text {
                    text: qsTr("Validate")
                    anchors.centerIn: parent
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        var ret = MODREADER.validateLicence(licTextInput.text)
                        if (ret === 1) {
                            // no lic file available
                            licError.text = "No Licence file available."
                            licError.visible = true
                        } else if (ret === 0) {
                            licError.text = "Invalid e-mail id."
                            licError.visible = true
                        } else if (ret === 2) {
                            licError.text = "Lincence file expired"
                            licError.visible = true
                        } else if (ret === 3) {
                            licRect.visible = false
                        }
                    }

                    onPressed: {
                        parent.color = "white"
                    }

                    onReleased: {
                        parent.color = licColor
                    }
                }
            }
        }

    }

    function disableControls()
    {
        console.log("Inside disable controls ....")

        rifleMouse.visible = false;
        pistolMouse.visible = false;
        startMouse.visible = false;
        resetMouse.visible = false;
        gameEventList.enabled = false;
        gameEventMouse.visible = false;
//        start_over.visible = false;
//        rifleMouse.anchors.fill = undefined
//        pistolMouse.anchors.fill = undefined
    }

    function perfromStart()
    {
        if (!appMode) // in demo mode
        {
            MODREADER.appendToLogFile("Application running in demo mode")
            if (connectToMaster && !MODREADER.isMasterSystemConnected()) {
                console.log(connectToMaster + " --- " + !MODREADER.isMasterSystemConnected())
                // show message that master system is not connected
                masterConnection.text = "Master system is not connected, Please Click \"Connect\" button."
                masterConnection.visible = true
                return;
            }
            rootItem.visible = false
        } else {
            MODREADER.appendToLogFile("Application running in Live mode")
            if (connectToMaster && !MODREADER.isMasterSystemConnected()) {
                MODREADER.appendToLogFile("Master application required")
                // show message that master system is not connected
                masterConnection.text = "Master system is not connected, Please Click \"Connect\" button."
                masterConnection.visible = true
                return;
            }

            if (masterConnectBtn && port_name_text_field.text != "")
            {
                MODREADER.appendToLogFile("Application with port text field")
                MODREADER.connectedModbus(port_name_text_field.text)
                mod_connected = MODREADER.isModBusConnected()
            }

            if (!MODREADER.isModBusConnected()) // we need validation only if port are connected
            {
                MODREADER.appendToLogFile("Com port not connected")
                validateLogin.text = "Com port not connected"
                validateLogin.visible = true
                //if (popupMode)
                //modBusConnector.visible = true
                // else TextInput is provided to given the port name
            }else if (!MODREADER.isHardwareConnected()) {
                validateLogin.text = "Hardware not connected."
                validateLogin.visible = true
            }else if (!MODREADER.checkAutoFeedMode()) {
                validateLogin.text = "Auto feed mode is off"
                validateLogin.visible = false
            }else if (validate()) {
                MODREADER.appendToLogFile("Validation was successful")
                rootItem.visible = false
            } else {
                MODREADER.appendToLogFile("Com-port connected but validation failed")
            }
        }
        //APPSETTINGS.autoSaveMatch()
        APPSETTINGS.saveMatch(true)
//        shootingPage.startFromServer();
    }
}
