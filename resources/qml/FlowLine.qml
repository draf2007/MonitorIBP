import QtQuick
import QtQuick.Shapes
import "FlowPath.js" as FlowPath

Item {
    id: line

    property var points: []
    property real radius: 16
    property string lineState: "off"

    property color activeColor
    property color idleColor

    readonly property bool active: ["flow", "charge", "discharge", "warn", "alarm"].indexOf(lineState) >= 0
    readonly property bool slow: lineState === "charge"

    readonly property real step: 18
    readonly property real band: step * 4
    property real phase: 0

    Shape {
        anchors.fill: parent
        visible: !line.active
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            strokeColor: line.idleColor
            strokeWidth: 2
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            PathSvg { path: FlowPath.svgPath(line.points, line.radius) }
        }
    }

    Repeater {
        model: line.active ? FlowPath.marks(line.points, line.radius, line.step) : []

        delegate: Shape {
            required property var modelData

            readonly property real glow: {
                var r = ((modelData.s - line.phase) % line.band + line.band) % line.band
                var d = Math.min(r, line.band - r)
                return 0.5 * (1 + Math.cos(2 * Math.PI * d / line.band))
            }

            x: modelData.x
            y: modelData.y
            rotation: modelData.angle
            scale: 1 + 0.6 * glow
            opacity: 0.25 + 0.75 * glow
            preferredRendererType: Shape.CurveRenderer

            ShapePath {
                strokeColor: line.activeColor
                strokeWidth: 2.6
                fillColor: "transparent"
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                PathSvg { path: "M -3 -4.5 L 3 0 L -3 4.5" }
            }
        }
    }

    Timer {
            interval: 40
            repeat: true
            running: line.active && line.visible && schematic.animationsEnabled
            onTriggered: {
                var speed = line.slow ? 36 : 90
                line.phase = (line.phase + speed * interval / 1000) % line.band
            }
        }

    Behavior on activeColor { ColorAnimation { duration: 400 } }
}