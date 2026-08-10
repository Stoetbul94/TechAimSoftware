import QtQuick 2.15
// The four discipline plates side by side, on the Tech Aim dark surface, at
// the size the left pane actually draws them.
Item {
    width: 620; height: 300
    Rectangle { anchors.fill: parent; color: "#15161a" }
    Grid {
        anchors.centerIn: parent
        columns: 2; rowSpacing: 14; columnSpacing: 14
        Repeater {
            model: [ { k: "AR10",    n: "10M AIR RIFLE" },
                     { k: "AP10",    n: "10M AIR PISTOL" },
                     { k: "PRONE50", n: "50M RIFLE PRONE" },
                     { k: "3P50",    n: "50M RIFLE 3 POS" } ]
            Rectangle {
                width: 280; height: 128; radius: 10
                color: "#26272c"; border.color: "#e8003d"; border.width: 1
                DisciplineArt {
                    id: da
                    anchors.top: parent.top; anchors.topMargin: 8
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 180; height: width * (64/120)
                    discipline: modelData.k
                }
                Text {
                    anchors.top: da.bottom; anchors.topMargin: 6
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: modelData.n
                    color: "white"; font.bold: true; font.pixelSize: 15
                }
            }
        }
    }
}
