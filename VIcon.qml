import QtQuick 2.15
import QtQuick.Shapes

// Crisp vector icon rendered from SVG path data on a 24x24 authoring grid.
// Scales to any size without blur (unlike the legacy raster PNGs). Stroke
// icons: leave filled=false. Solid silhouettes (pistol/rifle): filled=true,
// strokeWidth=0.
Item {
    id: root
    property string pathData: ""
    property color color: "white"
    property real strokeWidth: 1.9
    property bool filled: false
    // Authoring grid — override for non-square art (e.g. a long rifle).
    property real viewBoxW: 24
    property real viewBoxH: 24

    Shape {
        anchors.fill: parent
        layer.enabled: true
        layer.samples: 4
        transform: Scale {
            xScale: root.width > 0 ? root.width / root.viewBoxW : 1
            yScale: root.height > 0 ? root.height / root.viewBoxH : 1
        }
        ShapePath {
            strokeColor: root.strokeWidth > 0 ? root.color : "transparent"
            strokeWidth: root.strokeWidth
            fillColor: root.filled ? root.color : "transparent"
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            PathSvg { path: root.pathData }
        }
    }
}
