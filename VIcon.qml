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

    // Stroke widths are authored in grid units, so the Scale below turns them
    // into a FRACTIONAL number of device pixels at most sizes - 1.9 units at
    // 22 px is 1.74 px, which leaves every stroke edge a partial pixel and the
    // icon reads soft. Snap the DEVICE width to a whole pixel (never below 1)
    // and convert back to grid units, so the same authored value stays sharp
    // at any size. One uniform rule: no per-icon special cases, no geometry
    // moved, and filled silhouettes (strokeWidth 0) are untouched.
    readonly property real deviceScale:
        (viewBoxW > 0 && width > 0) ? width / viewBoxW : 1
    readonly property real snappedStrokeWidth:
        strokeWidth > 0 ? Math.max(1, Math.round(strokeWidth * deviceScale)) / deviceScale : 0

    Shape {
        anchors.fill: parent
        // NO layer here. A layer rasterises this Shape in its own unscaled
        // coordinates and clips it to the item's pixel bounds BEFORE the Scale
        // below is applied, so any icon drawn smaller than its authoring grid
        // lost everything past viewBox * (width/viewBoxW) - the rifle at the
        // 92 px the left pane draws lost its whole barrel and front sight.
        // Rendering the geometry unlayered draws it at final resolution.
        transform: Scale {
            xScale: root.width > 0 ? root.width / root.viewBoxW : 1
            yScale: root.height > 0 ? root.height / root.viewBoxH : 1
        }
        ShapePath {
            strokeColor: root.strokeWidth > 0 ? root.color : "transparent"
            strokeWidth: root.snappedStrokeWidth
            fillColor: root.filled ? root.color : "transparent"
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            PathSvg { path: root.pathData }
        }
    }
}
