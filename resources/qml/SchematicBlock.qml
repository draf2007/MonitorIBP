import QtQuick

Rectangle {
    id: block

    property string title
    property string iconPath
    property string blockState: "off"
    property string big
    property string sub
    property string note
    property bool showBar: false
    property int barLevel: 0
    property bool barFromRed: false
    property bool overload: false

    property color stateColor: "gray"
    property color stateTint: "lightgray"

    width: 150
    height: 140
    radius: 4
    border.width: 2
    border.color: stateColor
    color: Qt.tint(theme.colors.win, Qt.rgba(stateColor.r, stateColor.g, stateColor.b, 0.09))

    readonly property bool dimmed: blockState === "off" || blockState === "stale"

    Column {
        anchors.fill: parent
        anchors.margins: 9
        anchors.topMargin: 7
        spacing: 3

        Row {
            spacing: 7
            height: 36

            Rectangle {
                width: 34; height: 34; radius: 17
                anchors.verticalCenter: parent.verticalCenter
                color: block.stateTint
                border.width: 2
                border.color: block.stateColor

                SchematicIcon {
                    anchors.centerIn: parent
                    path: block.iconPath
                    color: block.stateColor
                }
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: block.title
                font.pixelSize: 14
                font.bold: true
                color: block.dimmed ? theme.colors.off : theme.colors.fg
            }
        }

        Text {
            width: parent.width
            text: block.big
            elide: Text.ElideRight
            font.pixelSize: 18
            font.bold: true
            color: block.blockState === "alarm" ? theme.colors.alarm
                 : block.dimmed ? theme.colors.off : theme.colors.fg
        }

        Text {
            width: parent.width
            text: block.sub
            elide: Text.ElideRight
            font.pixelSize: 12
            color: block.dimmed ? theme.colors.off : theme.colors.muted
        }

        Rectangle {
            visible: block.showBar && !block.overload
            width: parent.width
            height: 10
            radius: 2
            border.width: 1
            border.color: theme.colors.line
            clip: true
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0;  color: block.barFromRed ? theme.colors.alarm : theme.colors.good }
                GradientStop { position: block.barFromRed ? 0.35 : 0.65; color: theme.colors.warn }
                GradientStop { position: 1.0;  color: block.barFromRed ? theme.colors.good : theme.colors.alarm }
            }
            Rectangle {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.margins: 1
                width: (parent.width - 2) * (100 - block.barLevel) / 100
                color: theme.colors.bg
            }
            opacity: block.dimmed ? 0.4 : 1.0
        }

        Row {
            visible: block.overload
            spacing: 6
            SchematicIcon {
                width: 26; height: 26
                path: "M10.3 4.6Q12 1.9 13.7 4.6L21.5 18Q22.9 20.8 19.8 20.8H4.2Q1.1 20.8 2.5 18ZM12 10v5M12 17.8v.2"
                color: theme.colors.alarm
                fill: theme.colors.alarmTint
                lineWidth: 2
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "Перегрузка"
                font.pixelSize: 13
                font.bold: true
                color: theme.colors.alarm
            }
        }

        Text {
            width: parent.width
            text: block.note
            visible: text !== "" && !(block.overload && text === "Перегрузка")
            elide: Text.ElideRight
            font.pixelSize: 12
            font.bold: true
            color: block.blockState === "warn" ? theme.colors.warn
                 : block.blockState === "alarm" ? theme.colors.alarm
                 : block.dimmed ? theme.colors.off : theme.colors.fg
        }
    }
}
