import QtQuick 2.15

// ALL TARGETS — every lane in the display's ordered set.
//
// The grid FILLS the area it is given. On a projector showing six lanes that
// means six large faces, not six small ones in the top-left corner with the
// bottom half of the screen black. The column count is chosen to make the
// tiles as large and as square as the space allows, with a floor below which
// a lane number stops being readable across a room — past that the grid
// scrolls instead of shrinking further.
GridView {
    id: grid

    Theme { id: theme }

    readonly property int minTileWidth: 190
    readonly property int minTileHeight: 150

    function chooseColumns(w, h, n) {
        if (n <= 0 || w <= 0 || h <= 0)
            return 2
        var maxColumns = Math.max(1, Math.min(n, Math.floor(w / minTileWidth)))
        var best = 1
        var bestFit = -1
        for (var c = 1; c <= maxColumns; ++c) {
            var rows = Math.ceil(n / c)
            var tw = w / c
            var th = h / rows
            // Big, and square-ish: a very wide short tile wastes the face.
            var fit = Math.min(tw, th) - Math.abs(tw - th) * 0.25
            if (fit > bestFit) {
                bestFit = fit
                best = c
            }
        }
        return best
    }

    readonly property int columns: chooseColumns(width, height, count)
    readonly property int rowsShown: Math.max(1, Math.ceil(count / columns))
    readonly property bool compact: cellWidth < 260

    clip: true
    cellWidth: Math.max(minTileWidth, Math.floor(width / columns))
    cellHeight: Math.max(minTileHeight, Math.floor(height / rowsShown))
    model: DISPLAYLANES

    delegate: Item {
        width: grid.cellWidth
        height: grid.cellHeight

        RmsTargetCard {
            anchors.fill: parent
            anchors.margins: 5
            compact: grid.compact
            selected: DISPLAY.selectedLane === model.laneNumber
            // The model's row map, so the card and the single view are fed
            // from exactly the same place.
            lane: DISPLAYLANES.laneByNumber(model.laneNumber)
            // Clicking a target SELECTS that physical lane and opens it large.
            onActivated: DISPLAY.selectLane(model.laneNumber)
        }
    }
}
