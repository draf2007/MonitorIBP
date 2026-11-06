import QtQuick
import QtQuick.Effects

Item {
    id: background
    property color color
    property real strength: 0.08
    clip: true
    layer.enabled: true

    Item {
        id: pattern
        anchors.centerIn: parent
        width: Math.hypot(background.width, background.height) + 350
        height: width
        rotation: -25
        opacity: background.strength

        Image {
            id: tiles
            anchors.fill: parent
            source: "qrc:/resources/image/logo_tile.png"
            sourceSize: Qt.size(250, 350)
            fillMode: Image.Tile
            visible: false
            layer.enabled: true
            layer.smooth: true
        }

        Rectangle {
            id: fill
            anchors.fill: parent
            color: background.color
            visible: false
            layer.enabled: true
        }

        MultiEffect {
            anchors.fill: parent
            source: fill
            maskEnabled: true
            maskSource: tiles
            maskThresholdMin: 0.5
            maskSpreadAtMin: 1.0
        }
    }
}