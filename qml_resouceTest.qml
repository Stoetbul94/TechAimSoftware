import QtQuick 2.2
Item {
    property int rootItemWidth:6000
    property int rootItemHeight:3900
    Image {
        id: layer_0
        source: "images/layer_0.png"
        x: ((parent.width/rootItemWidth)*0)
        y: ((parent.height/rootItemHeight)*0)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: synthesis
        source: "images/synthesis.png"
        x: ((parent.width/rootItemWidth)*466)
        y: ((parent.height/rootItemHeight)*813)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: simulation
        source: "images/simulation.png"
        x: ((parent.width/rootItemWidth)*466)
        y: ((parent.height/rootItemHeight)*1099)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: stimulus
        source: "images/stimulus.png"
        x: ((parent.width/rootItemWidth)*466)
        y: ((parent.height/rootItemHeight)*1399)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: indentify
        source: "images/indentify.png"
        x: ((parent.width/rootItemWidth)*466)
        y: ((parent.height/rootItemHeight)*1699)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: synthesis_over
        source: "images/synthesis_over.png"
        x: ((parent.width/rootItemWidth)*466)
        y: ((parent.height/rootItemHeight)*813)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: simulation_over
        source: "images/simulation_over.png"
        x: ((parent.width/rootItemWidth)*466)
        y: ((parent.height/rootItemHeight)*1099)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: stimulus_over
        source: "images/stimulus_over.png"
        x: ((parent.width/rootItemWidth)*466)
        y: ((parent.height/rootItemHeight)*1399)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: indentify_over
        source: "images/indentify_over.png"
        x: ((parent.width/rootItemWidth)*466)
        y: ((parent.height/rootItemHeight)*1699)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: red_bg
        source: "images/red_bg.png"
        x: ((parent.width/rootItemWidth)*1650)
        y: ((parent.height/rootItemHeight)*434)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: layer_24
        source: "images/layer_24.png"
        x: ((parent.width/rootItemWidth)*1766)
        y: ((parent.height/rootItemHeight)*2328)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: logo_back_design
        source: "images/logo_back_design.png"
        x: ((parent.width/rootItemWidth)*-655)
        y: ((parent.height/rootItemHeight)*-26)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: logo
        source: "images/logo.png"
        x: ((parent.width/rootItemWidth)*248)
        y: ((parent.height/rootItemHeight)*63)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: bottom_arrow_tools
        source: "images/bottom_arrow_tools.png"
        x: ((parent.width/rootItemWidth)*167)
        y: ((parent.height/rootItemHeight)*637)
        opacity: 0.70196078431373
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: bottom_arrow_vaults
        source: "images/bottom_arrow_vaults.png"
        x: ((parent.width/rootItemWidth)*167)
        y: ((parent.height/rootItemHeight)*2097)
        opacity: 0.70196078431373
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: arrows_side
        source: "images/arrows_side.png"
        x: ((parent.width/rootItemWidth)*417)
        y: ((parent.height/rootItemHeight)*964)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Text {
        id: profilewidget
        text: qsTr("ProfileWidget")
        font.pixelSize: 45
        font.family: "CenturyGothic"
        color: "#89959d"
        smooth: true
        x: ((parent.width/rootItemWidth)*547)
        y: ((parent.height/rootItemHeight)*131)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: close
        source: "images/close.png"
        x: ((parent.width/rootItemWidth)*5678)
        y: ((parent.height/rootItemHeight)*107)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: maximize
        source: "images/maximize.png"
        x: ((parent.width/rootItemWidth)*5385)
        y: ((parent.height/rootItemHeight)*98)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: minimize
        source: "images/minimize.png"
        x: ((parent.width/rootItemWidth)*5084)
        y: ((parent.height/rootItemHeight)*152)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Text {
        id: tools
        text: qsTr("Tools")
        font.pixelSize: 20.5
        font.family: "CenturyGothic"
        color: "#ffffff"
        smooth: true
        x: ((parent.width/rootItemWidth)*334)
        y: ((parent.height/rootItemHeight)*631)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Text {
        id: synthesis_1
        text: qsTr("Synthesis")
        font.pixelSize: 14.5
        font.family: "CenturyGothic"
        color: "#ffffff"
        smooth: true
        x: ((parent.width/rootItemWidth)*677)
        y: ((parent.height/rootItemHeight)*965)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Text {
        id: simulation_2
        text: qsTr("Simulation")
        font.pixelSize: 14.5
        font.family: "CenturyGothic"
        color: "#ffffff"
        smooth: true
        x: ((parent.width/rootItemWidth)*684)
        y: ((parent.height/rootItemHeight)*1251)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Text {
        id: stimulus_3
        text: qsTr("Stimulus")
        font.pixelSize: 14.5
        font.family: "CenturyGothic"
        color: "#ffffff"
        smooth: true
        x: ((parent.width/rootItemWidth)*667)
        y: ((parent.height/rootItemHeight)*1550)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Text {
        id: identify_debugger
        text: qsTr("Identify Debugger")
        font.pixelSize: 14.5
        font.family: "CenturyGothic"
        color: "#ffffff"
        smooth: true
        x: ((parent.width/rootItemWidth)*675)
        y: ((parent.height/rootItemHeight)*1854)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Text {
        id: vaults_location
        text: qsTr("Vaults location")
        font.pixelSize: 20.5
        font.family: "CenturyGothic"
        color: "#ffffff"
        smooth: true
        x: ((parent.width/rootItemWidth)*333)
        y: ((parent.height/rootItemHeight)*2088)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: ok
        source: "images/ok.png"
        x: ((parent.width/rootItemWidth)*4751)
        y: ((parent.height/rootItemHeight)*3706)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
    Image {
        id: cancel
        source: "images/cancel.png"
        x: ((parent.width/rootItemWidth)*5163)
        y: ((parent.height/rootItemHeight)*3709)
        opacity: 1
        width: ((parent.width/rootItemWidth)*sourceSize.width)
        height: ((parent.height/rootItemHeight)*sourceSize.height)
    }
}
