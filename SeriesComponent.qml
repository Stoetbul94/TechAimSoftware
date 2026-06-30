import QtQuick 2.3

Item {
    id: seriesComp

    property int seriesIndex: 0 //0 is invalid, series starts with 1

    Component.onCompleted: {
        console.log("*******************************************", seriesIndex)
    }

    function update()
    {
        visible = false
        visible = (seriesIndex-1)*10 <= globalModelOfData.count
        console.log(visible," PDF series page for series ", seriesIndex, " count ", globalModelOfData.count)
    }

    Connections {
        target: MODREADER
        onShootCountChanged: {
            update()
        }
    }

    Rectangle {
        color: "transparent"
        anchors.fill: parent

        Column{
            anchors.fill: parent
            anchors.topMargin: 20
            anchors.leftMargin: 20

            Row {
                width: parent.width > 300 ? parent.width : 500
                height: parent.height
                spacing: 50
                Image {
                    id: shootingcanvas
//                    source: centerPanel.gameMode ? (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/pistol.png" : "qrc:/images/centerPanel/pistol_blue.png")
//                                                 : (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/rifle.png" : "qrc:/images/centerPanel/rifle_blue.png")
                    source: gameRange == 10 ? (centerPanel.gameMode ? (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/pistol.png" : "qrc:/images/centerPanel/pistol_blue.png")
                                     : (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/rifle.png" : "qrc:/images/centerPanel/rifle_blue.png"))
                                            : (centerPanel.gameMode ? (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/50_meter.png" : "qrc:/images/centerPanel/50_meter_blue.png")
                                                        : (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/black_50_Rifle.png" : "qrc:/images/centerPanel/blue_50_Rifle.png"))
                    width: 150 //parent.width < parent.height ? parent.width*0.45 : parent.height*0.45
                    height: width
                    opacity: 1
                    //anchors.top: parent.top
                    y: (mri.itemCount*7)

                    Rectangle {
                        id: shootingMianRect
                        color: "transparent"
                        anchors.fill: parent
//                        opacity: 0.5
                    }

                    Repeater
                    {
                        id:numberRepeater
                        model:globalModelOfData
                        delegate: numberDelegate
                    }

                    Component {
                        id:numberDelegate
                        Item {
                            id:mainItem
                            // 34.55 and 10.11 was given by abins (tachus)
                            //10 Meter Pistol Ratio=155.5÷4.5=34.55
                            //10 Meter Rifle Ratio=45.5÷4.5=10.11
                            //50 Meter Pistol Ratio=500/5.6=89.29
                            //50 Meter Rifle Ratio=154.4/5.6=27.57

                            property double gameRatio: gameRange == 10 ? (centerPanel.gameMode ? 155.5/APPSETTINGS.bullet_diameter() : 45.5/APPSETTINGS.bullet_diameter())
                                                                       : (centerPanel.gameMode ? 500/APPSETTINGS.bullet_diameter() : 154.4/APPSETTINGS.bullet_diameter())
//                            width: gameRange == 10 ? (centerPanel.gameMode ? shootingcanvas.height/34.55 : shootingcanvas.height/10.11 )
//                                                   : (centerPanel.gameMode ? shootingcanvas.height/89.29 /*size 500 pallet 5.6*/
//                                                               : shootingcanvas.height/27.57 /*size 154.4 pallet 5.6*/)
                            width: shootingcanvas.height/gameRatio
                            height: width
                            Rectangle
                            {
                                id: mainItemRect
                                Component.onCompleted:
                                {
                                    var xCor = MODREADER.getXCord(index+1)
                                    var yCor = MODREADER.getYCord(index+1)

//                                    var pistalWidthHeight = 155.5
//                                    var rifleWidthHeight = 45.5
                                    var pistalWidthHeight = gameRange == 10 ? 155.5 : 500
                                    var rifleWidthHeight = gameRange == 10 ? 45.5 : 154.4
                                    var shootingWidth = centerPanel.gameMode ? pistalWidthHeight : rifleWidthHeight
                                    var shootingHeight = centerPanel.gameMode ? pistalWidthHeight : rifleWidthHeight

                                    var offsetX = shootingMianRect.width/shootingWidth
                                    var offsetY = shootingMianRect.height/shootingHeight

                                    var centerX = shootingMianRect.width/2 //* offset
                                    var centerY = shootingMianRect.height/2 //* offset

                                    var bulletSize = 0//4.5/2

                                    mainItem.x = shootingMianRect.x + centerX+((xCor+bulletSize)*offsetX) - radius
                                    mainItem.y = shootingMianRect.y + centerY-((yCor+bulletSize)*offsetY) - radius
                                }

                                onVisibleChanged: {
                                    var xCor = MODREADER.getXCord(index+1)
                                    var yCor = MODREADER.getYCord(index+1)
//                                    console.log(seriesIndex, "on visible changed ----------------------------------121 ", xCor, "  ", yCor)

                                    var pistalWidthHeight = gameRange == 10 ? 155.5 : 500
                                    var rifleWidthHeight = gameRange == 10 ? 45.5 : 154.4
                                    var shootingWidth = centerPanel.gameMode ? pistalWidthHeight : rifleWidthHeight
                                    var shootingHeight = centerPanel.gameMode ? pistalWidthHeight : rifleWidthHeight

                                    var offsetX = shootingMianRect.width/shootingWidth
                                    var offsetY = shootingMianRect.height/shootingHeight

                                    var centerX = shootingMianRect.width/2 //* offset
                                    var centerY = shootingMianRect.height/2 //* offset

                                    var bulletSize = 0//4.5/2

                                    mainItem.x = shootingMianRect.x + centerX+((xCor+bulletSize)*offsetX) - radius
                                    mainItem.y = shootingMianRect.y + centerY-((yCor+bulletSize)*offsetY) - radius
//                                    console.log(shootingMianRect.x," ",shootingMianRect.y, " ", shootingMianRect.width,
//                                                " ", shootingMianRect.height, "---------------------------------- ",
//                                                mainItem.x, "  ", mainItem.y, " ", offsetX)
                                }

                                anchors.fill: parent
                                radius:parent.width/2
                                color: greenColor //shootingPage.isPalletRed ? "green" : "white"
                                Text{
                                    anchors.centerIn: parent
                                    text: index+1
                                    visible: false
                                }
//                                border.color: "red"
                                visible: index >= (seriesComp.seriesIndex - 1) *10 && index < seriesComp.seriesIndex*10 ? true : false

                                function refreshPelletItem()
                                {
                                    var xCor = MODREADER.getXCord(index+1)
                                    var yCor = MODREADER.getYCord(index+1)

                                    var pistalWidthHeight = gameRange == 10 ? 155.5 : 500
                                    var rifleWidthHeight = gameRange == 10 ? 45.5 : 154.4
                                    var shootingWidth = centerPanel.gameMode ? pistalWidthHeight : rifleWidthHeight
                                    var shootingHeight = centerPanel.gameMode ? pistalWidthHeight : rifleWidthHeight

                                    var offsetX = shootingMianRect.width/shootingWidth
                                    var offsetY = shootingMianRect.height/shootingHeight

                                    var centerX = shootingMianRect.width/2 //* offset
                                    var centerY = shootingMianRect.height/2 //* offset

                                    var bulletSize = 0//4.5/2

                                    mainItem.x = shootingMianRect.x + centerX+((xCor+bulletSize)*offsetX) - radius
                                    mainItem.y = shootingMianRect.y + centerY-((yCor+bulletSize)*offsetY) - radius
                                }
                            }

                            onVisibleChanged: {
                                mainItemRect.refreshPelletItem()
                            }
                        }
                    }
                }

                MatchReportInfo {
                    id: mri
                    isMatchSummary: false
                    seriesIndex: seriesComp.seriesIndex
                    width: parent.width - shootingcanvas.width - 50
                    height: parent.height
                }
            }
        }
    }
}
