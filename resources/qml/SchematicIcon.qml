import QtQuick
import QtQuick.Shapes

Item {
    id: icon
    property string path
    property color color
    property real lineWidth: 1.8
    property color fill: "transparent"

    width: 22
    height: 22

    Shape {
        width: 24
        height: 24
        scale: icon.width / 24
        transformOrigin: Item.TopLeft
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            strokeColor: icon.color
            strokeWidth: icon.lineWidth
            fillColor: icon.fill
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            PathSvg { path: icon.path }
        }
    }
}
