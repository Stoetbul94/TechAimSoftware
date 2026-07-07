import QtQuick 2.3

// One series rendered as a Tech Aim SeriesCard: header chips (score, inner-10,
// MPI, group) + body (per-series target plot on the left, shot table on the
// right). All values come from MatchReportInfo's existing functions and the
// backend — no calculation is duplicated or changed here.
Item {
    id: seriesComp

    property int seriesIndex: 0 //0 is invalid, series starts with 1

    function update()
    {
        visible = false
        visible = (seriesIndex-1)*10 < globalModelOfData.count
    }

    Connections {
        target: MODREADER
        onShootCountChanged: {
            update()
        }
    }

    SeriesCard {
        id: card
        anchors.fill: parent
        seriesTitle: qsTr("Series ") + seriesComp.seriesIndex
        chips: [
            { l: qsTr("Score"),    v: mri.getSeriesTotal(seriesComp.seriesIndex) + " (" + mri.getSeriesTotalNonDecimal(seriesComp.seriesIndex) + ")", unit: "" },
            { l: qsTr("Inner 10"), v: "" + mri.getSeriesInner(seriesComp.seriesIndex), unit: "" },
            { l: qsTr("MPI"),      v: MODREADER.getXMPI(seriesComp.seriesIndex).toFixed(1) + ", " + MODREADER.getYMPI(seriesComp.seriesIndex).toFixed(1), unit: "mm" },
            { l: qsTr("Group"),    v: MODREADER.getGroup(seriesComp.seriesIndex-1).toFixed(1), unit: "mm" }
        ]

        Row {
            id: bodyRow
            anchors.fill: parent
            spacing: 16

            // ── Per-series target plot (existing coordinate math, only this
            //     series' shots visible) ─────────────────────────────────────
            Image {
                id: shootingcanvas
                source: gameRange == 10 ? (centerPanel.gameMode ? (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/pistol.png" : "qrc:/images/centerPanel/pistol_blue.png")
                                 : (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/rifle.png" : "qrc:/images/centerPanel/rifle_blue.png"))
                                        : (centerPanel.gameMode ? (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/50_meter.png" : "qrc:/images/centerPanel/50_meter_blue.png")
                                                    : (shootingPage.isBackgroudBlack ? "qrc:/images/centerPanel/black_50_Rifle.png" : "qrc:/images/centerPanel/blue_50_Rifle.png"))
                width: Math.min(parent.height, 170)
                height: width
                opacity: 1
                anchors.verticalCenter: parent.verticalCenter

                Rectangle {
                    id: shootingMianRect
                    color: "transparent"
                    anchors.fill: parent
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
                        width: shootingcanvas.height/gameRatio
                        height: width
                        Rectangle
                        {
                            id: mainItemRect
                            Component.onCompleted:
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

                            onVisibleChanged: {
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

                            anchors.fill: parent
                            radius:parent.width/2
                            color: greenColor //shootingPage.isPalletRed ? "green" : "white"
                            Text{
                                anchors.centerIn: parent
                                text: index+1
                                visible: false
                            }
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

            // ── Shot table ────────────────────────────────────────────────
            MatchReportInfo {
                id: mri
                isMatchSummary: false
                seriesIndex: seriesComp.seriesIndex
                width: bodyRow.width - shootingcanvas.width - bodyRow.spacing
                height: parent.height
            }
        }
    }
}
