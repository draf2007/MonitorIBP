import QtQuick
import QtQuick.Layouts
import "Icons.js" as Icons

Rectangle {
    id: root
    color: theme.colors.win

    property var icons: Icons.sets[theme.iconSet] || Icons.sets.scheme

    function stateColor(state) {
        switch (state) {
        case "ok":    return theme.colors.accent
        case "warn":  return theme.colors.warn
        case "alarm": return theme.colors.alarm
        case "stale": return theme.colors.muted
        default:      return theme.colors.off
        }
    }
    function stateTint(state) {
        switch (state) {
        case "ok":    return theme.colors.accentTint
        case "warn":  return theme.colors.warnTint
        case "alarm": return theme.colors.alarmTint
        default:      return theme.colors.offBg
        }
    }

    function lineColor(state) {
            switch (state) {
            case "discharge":
            case "warn":  return theme.colors.warn
            case "alarm": return theme.colors.alarm
            default:      return theme.colors.accent
            }
        }

    LogoBackground {
        anchors.fill: parent
        color: theme.colors.fg
    }

    Rectangle {
        id: banner
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 12
        height: 36
        radius: 3

        readonly property string bannerState: schematic.banner.state
        readonly property bool isOff: bannerState === "off"

        color: isOff ? "transparent" : root.stateTint(bannerState)
        border.color: isOff ? theme.colors.line : root.stateColor(bannerState)
        border.width: 1
        Behavior on color { ColorAnimation { duration: 400 } }
        Behavior on border.color { ColorAnimation { duration: 400 } }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 10

            Text {
                text: schematic.banner.text
                font.pixelSize: 15
                font.bold: true
                color: banner.isOff ? theme.colors.muted : root.stateColor(banner.bannerState)
            }

            Rectangle {
                visible: alarmText.text !== ""
                Layout.fillWidth: true
                Layout.maximumWidth: alarmText.implicitWidth + 18
                Layout.minimumWidth: 40
                height: 22
                radius: 11
                color: "transparent"
                border.color: theme.colors.alarm
                border.width: 1

                Text {
                    id: alarmText
                    anchors.fill: parent
                    anchors.leftMargin: 9
                    anchors.rightMargin: 9
                    verticalAlignment: Text.AlignVCenter
                    text: schematic.banner.alarm
                    font.pixelSize: 12
                    font.bold: true
                    color: theme.colors.alarm
                    elide: Text.ElideRight
                }
            }

            Item { Layout.fillWidth: true }

            Text {
                text: schematic.banner.meta
                font.pixelSize: 12
                color: theme.colors.muted
            }
        }
    }

    FlowLine {
        anchors.fill: parent
        points: [Qt.point(mainsBlock.x + mainsBlock.width + 4, mainsBlock.y + mainsBlock.height / 2),
                 Qt.point(upsBlock.x - 4,                      upsBlock.y + upsBlock.height / 2)]
        lineState: schematic.lines["in"]
        activeColor: root.lineColor(lineState)
        idleColor: theme.colors.line
    }

    FlowLine {
        anchors.fill: parent
        points: [Qt.point(upsBlock.x + upsBlock.width + 4, upsBlock.y + upsBlock.height / 2),
                 Qt.point(loadBlock.x - 4,                 loadBlock.y + loadBlock.height / 2)]
        lineState: schematic.lines["out"]
        activeColor: root.lineColor(lineState)
        idleColor: theme.colors.line
    }

    FlowLine {
        anchors.fill: parent
        readonly property point upEnd:   Qt.point(upsBlock.x + upsBlock.width / 2, upsBlock.y + upsBlock.height + 4)
        readonly property point downEnd: Qt.point(batteryBlock.x + batteryBlock.width / 2, batteryBlock.y - 4)
        points: lineState === "charge" ? [upEnd, downEnd] : [downEnd, upEnd]
        lineState: schematic.lines["battery"]
        activeColor: root.lineColor(lineState)
        idleColor: theme.colors.line
    }

    FlowLine {
        anchors.fill: parent
        readonly property real topY: upsBlock.y - 60
        points: [Qt.point(mainsBlock.x + mainsBlock.width / 2, mainsBlock.y - 4),
                 Qt.point(mainsBlock.x + mainsBlock.width / 2, topY),
                 Qt.point(loadBlock.x + loadBlock.width / 2,   topY),
                 Qt.point(loadBlock.x + loadBlock.width / 2,   loadBlock.y - 4)]
        lineState: schematic.lines["bypass"]
        activeColor: root.lineColor(lineState)
        idleColor: theme.colors.line
    }

    SchematicBlock {
        id: upsBlock
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 140
        title: "ИБП"
        iconPath: root.icons.ups
        blockState: schematic.ups.state
        stateColor: root.stateColor(blockState)
        stateTint: root.stateTint(blockState)
        big: schematic.ups.title
        sub: schematic.ups.temp
        note: schematic.ups.note
    }

    SchematicBlock {
        id: mainsBlock
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.top: upsBlock.top
        title: "Сеть"
        iconPath: root.icons.mains
        blockState: schematic.mains.state
        stateColor: root.stateColor(blockState)
        stateTint: root.stateTint(blockState)
        big: schematic.mains.voltage
        sub: schematic.mains.freq
        note: schematic.mains.note
    }

    SchematicBlock {
        id: loadBlock
        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.top: upsBlock.top
        title: "Нагрузка"
        iconPath: root.icons.load
        blockState: schematic.load.state
        stateColor: root.stateColor(blockState)
        stateTint: root.stateTint(blockState)
        big: schematic.load.voltage
        sub: schematic.load.percent
        note: schematic.load.note
        showBar: true
        barLevel: schematic.load.level
        overload: schematic.load.overload
    }

    SchematicBlock {
        id: batteryBlock
        anchors.horizontalCenter: upsBlock.horizontalCenter
        anchors.top: upsBlock.bottom
        anchors.topMargin: 64
        title: "Батарея"
        iconPath: root.icons.bat
        blockState: schematic.battery.state
        stateColor: root.stateColor(blockState)
        stateTint: root.stateTint(blockState)
        big: schematic.battery.voltage
        sub: schematic.battery.detail
        note: schematic.battery.note
        showBar: true
        barLevel: schematic.battery.level
        barFromRed: true
    }
}
