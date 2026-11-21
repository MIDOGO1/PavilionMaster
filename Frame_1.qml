import QtQuick
import QtQuick.Window

Window {
    id: window
    visible: true
    width: 1043
    height: 668
    title: "HP Pavilion 15 Gaming Control Center"
    color: "transparent"
    flags: Qt.FramelessWindowHint | Qt.Window

    // Supported processors: Ryzen 7 3750H, Ryzen 5 4600H, Ryzen 7 4800H,
    // Ryzen 5 5600H and Ryzen 7 5800H (the list lives in the C++ backend)
    readonly property bool supported: backend.cpuSupported

    // ---------- Accent color ----------
    // One entry per laptop color variant. `fill` is the swatch / selected-button color,
    // `ink` is the text + icon color that stays readable on top of that fill,
    // `vivid` is the same hue tuned for coloured text on the dark background.
    // `key` is what gets saved, so don't rename keys once released.
    readonly property var accentVariants: [
        { key: "acid_green",   name: "Acid Green",   fill: "#a4e51f", ink: "#0b1418", vivid: "#b8f53a" },
        { key: "ghost_white",  name: "Ghost White",  fill: "#e9eef2", ink: "#0b1418", vivid: "#f4f7fa" },
        { key: "ultra_violet", name: "Ultra Violet", fill: "#7c3aed", ink: "#ffffff", vivid: "#b18cff" }
    ]
    // Used until the user saves a choice
    readonly property var defaultAccent: ({ key: "", name: "Red", fill: "#a8201a", ink: "#ffffff", vivid: "#ff4d4d" })
    readonly property var themeAccent: {
        for (var i = 0; i < accentVariants.length; i++)
            if (accentVariants[i].key === backend.accentVariant)
                return accentVariants[i]
        return defaultAccent
    }

    Component.onCompleted: {
        if (!supported)
            unsupportedDialog.shown = true
        else if (backend.accentVariant === "")
            colorDialog.shown = true          // first launch
    }

    // ---------- Reusable pieces ----------

    // One row of the info panels: icon centered in a fixed slot + text
    component InfoRow: Item {
        property url icon
        property alias text: label.text
        property alias textColor: label.color
        width: 280; height: 24

        Item {
            width: 36; height: parent.height
            Image {
                anchors.centerIn: parent
                source: icon
            }
        }
        Text {
            id: label
            x: 40
            width: parent.width - 40
            height: parent.height
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            color: "#ffffff"
            font.family: "Inter"; font.pixelSize: 16; font.weight: Font.Medium
        }
    }

    // Pill button with centered icon + label, greys out when inactive
    component ActionButton: Rectangle {
        id: btn
        property url icon
        property string label
        property bool selected: false
        property bool active: true
        // Colors used while `selected` (the app's accent)
        property color accentFill: "#a8201a"
        property color accentInk: "#ffffff"
        signal clicked()

        readonly property bool lit: active && selected
        // Dark text/icons are used when the accent fill is light (Acid Green, Ghost White)
        readonly property bool darkInk: (accentInk.r * 0.299 + accentInk.g * 0.587 + accentInk.b * 0.114) < 0.5

        height: 42; width: 239; radius: 21
        color: !active ? "#2b2b2b" : (selected ? accentFill : "#393939")
        opacity: active ? 1.0 : 0.45
        scale: mouse.pressed ? 0.95 : 1.0

        Behavior on color   { ColorAnimation  { duration: 200 } }
        Behavior on opacity { NumberAnimation { duration: 200 } }
        Behavior on scale   { NumberAnimation { duration: 100; easing.type: Easing.OutQuad } }

        Row {
            anchors.centerIn: parent
            spacing: 10
            // Icons are white PNGs; a "_dark" twin is used when the accent needs dark ink
            Image {
                visible: btn.icon.toString() !== ""
                anchors.verticalCenter: parent.verticalCenter
                source: (btn.lit && btn.darkInk) ? btn.icon.toString().replace(".png", "_dark.png") : btn.icon
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                color: btn.lit ? btn.accentInk : "#ffffff"
                Behavior on color { ColorAnimation { duration: 200 } }
                font.family: "Inter"; font.pixelSize: 16; font.weight: Font.Medium
                text: btn.label
            }
        }

        MouseArea {
            id: mouse
            anchors.fill: parent
            enabled: btn.active
            hoverEnabled: true
            cursorShape: btn.active ? Qt.PointingHandCursor : Qt.ArrowCursor
            onClicked: btn.clicked()
        }
    }

    // Round window-control button (drawn glyphs, hover fill, press feedback)
    component WindowButton: Rectangle {
        id: wb
        property string kind: "close"          // "close" | "minimize" | "palette"
        property color hoverColor: "#e53935"
        property var swatchColors: []          // palette glyph: one dot per color
        signal clicked()

        width: 30; height: 30; radius: 15
        color: wbMouse.containsMouse ? hoverColor : "#1affffff"
        border.width: 1
        border.color: wbMouse.containsMouse ? Qt.lighter(hoverColor, 1.25) : "#26ffffff"
        scale: wbMouse.pressed ? 0.9 : 1.0
        antialiasing: true

        Behavior on color { ColorAnimation  { duration: 150 } }
        Behavior on scale { NumberAnimation { duration: 90; easing.type: Easing.OutQuad } }

        // Minimize glyph
        Rectangle {
            visible: wb.kind === "minimize"
            anchors.centerIn: parent
            width: 12; height: 2; radius: 1
            color: wbMouse.containsMouse ? "#ffffff" : "#d9ffffff"
        }
        // Palette glyph (cluster of three color dots)
        Item {
            visible: wb.kind === "palette"
            anchors.centerIn: parent
            width: 18; height: 16
            opacity: wbMouse.containsMouse ? 1.0 : 0.92
            Repeater {
                model: wb.swatchColors
                delegate: Rectangle {
                    required property var modelData
                    required property int index
                    width: 8; height: 8; radius: 4
                    x: index === 0 ? 5 : (index === 1 ? 0 : 10)
                    y: index === 0 ? 0 : 8
                    color: modelData
                    antialiasing: true
                }
            }
        }
        // Close glyph (two rotated bars)
        Item {
            visible: wb.kind === "close"
            anchors.centerIn: parent
            width: 12; height: 12
            Rectangle {
                anchors.centerIn: parent
                width: 14; height: 2; radius: 1; rotation: 45
                color: wbMouse.containsMouse ? "#ffffff" : "#d9ffffff"
                antialiasing: true
            }
            Rectangle {
                anchors.centerIn: parent
                width: 14; height: 2; radius: 1; rotation: -45
                color: wbMouse.containsMouse ? "#ffffff" : "#d9ffffff"
                antialiasing: true
            }
        }

        MouseArea {
            id: wbMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: wb.clicked()
        }
    }

    // Modal popup in the app's style: dimmed backdrop + glass card + two pill buttons
    component ModalDialog: Item {
        id: dlg
        property bool shown: false
        property string iconKind: "warning"          // "warning" | "blocked"
        property string title
        property string message
        property string note
        property string primaryLabel: "Continue"
        property string secondaryLabel: "Back"
        property color accentFill: "#a8201a"
        property color accentInk: "#ffffff"
        signal primaryClicked()
        signal secondaryClicked()

        anchors.fill: parent
        visible: opacity > 0
        opacity: shown ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation { duration: 180; easing.type: Easing.OutQuad } }

        // Dimmed backdrop (follows the card's rounded corners)
        Rectangle {
            anchors.fill: parent
            radius: 70
            color: "#d9040a0c"
            antialiasing: true
        }
        // Swallow clicks behind the popup (top bar stays draggable)
        MouseArea {
            anchors.fill: parent
            anchors.topMargin: 80
            hoverEnabled: true
        }

        Rectangle {
            id: card
            width: 540
            height: col.implicitHeight + 60
            anchors.centerIn: parent
            anchors.verticalCenterOffset: 20
            radius: 26
            color: "#0d1b20"
            border.width: 1
            border.color: "#2effffff"
            antialiasing: true
            scale: dlg.shown ? 1.0 : 0.92
            Behavior on scale { NumberAnimation { duration: 180; easing.type: Easing.OutBack } }

            Column {
                id: col
                x: 32; y: 30
                width: parent.width - 64
                spacing: 16

                Row {
                    spacing: 14
                    height: 48

                    Item {
                        width: 48; height: 48
                        // Warning badge
                        Rectangle {
                            visible: dlg.iconKind === "warning"
                            anchors.fill: parent
                            radius: 24
                            color: "#26f59e0b"
                            border.width: 2
                            border.color: "#f59e0b"
                            Text {
                                anchors.centerIn: parent
                                text: "!"
                                color: "#f59e0b"
                                font.family: "Inter"; font.pixelSize: 28; font.weight: Font.Bold
                            }
                        }
                        // Prohibited sign
                        Image {
                            visible: dlg.iconKind === "blocked"
                            anchors.fill: parent
                            source: Qt.resolvedUrl("assets/no_symbol.png")
                            fillMode: Image.PreserveAspectFit
                            smooth: true; mipmap: true
                        }
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        color: "#ffffff"
                        font.family: "Inter"; font.pixelSize: 24; font.weight: Font.DemiBold
                        text: dlg.title
                    }
                }

                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    lineHeight: 1.15
                    color: "#e6ffffff"
                    font.family: "Inter"; font.pixelSize: 15; font.weight: Font.Medium
                    text: dlg.message
                }

                Rectangle {
                    visible: dlg.note !== ""
                    width: parent.width; height: 1
                    color: "#1affffff"
                }
                Text {
                    visible: dlg.note !== ""
                    width: parent.width
                    wrapMode: Text.WordWrap
                    lineHeight: 1.15
                    color: "#99ffffff"
                    font.family: "Inter"; font.pixelSize: 13; font.weight: Font.Medium
                    text: dlg.note
                }

                Row {
                    spacing: 12
                    ActionButton {
                        width: 224
                        label: dlg.secondaryLabel
                        onClicked: dlg.secondaryClicked()
                    }
                    ActionButton {
                        width: 240
                        selected: true
                        accentFill: dlg.accentFill
                        accentInk: dlg.accentInk
                        label: dlg.primaryLabel
                        onClicked: dlg.primaryClicked()
                    }
                }
            }
        }
    }

    // Accent color popup: same dimmed backdrop + glass card as ModalDialog,
    // three rounded color swatches with their names, and a Save pill button
    component ColorPickerDialog: Item {
        id: picker
        property bool shown: false
        property var options: []            // [{ key, name, fill, ink }]
        property string currentKey: ""      // saved choice ("" = nothing saved yet)
        property string pendingKey: ""      // swatch highlighted inside the popup
        property color accentFill: "#a8201a"
        property color accentInk: "#ffffff"
        signal saved(string key)
        signal dismissed()

        readonly property var pendingOption: {
            for (var i = 0; i < options.length; i++)
                if (options[i].key === pendingKey)
                    return options[i]
            return null
        }

        anchors.fill: parent
        visible: opacity > 0
        opacity: shown ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation { duration: 180; easing.type: Easing.OutQuad } }
        onShownChanged: if (shown) pendingKey = currentKey

        // Dimmed backdrop (follows the card's rounded corners)
        Rectangle {
            anchors.fill: parent
            radius: 70
            color: "#d9040a0c"
            antialiasing: true
        }
        // Swallow clicks behind the popup (top bar stays draggable).
        // Clicking outside the card closes it, but only once a color has been saved before.
        MouseArea {
            anchors.fill: parent
            anchors.topMargin: 80
            hoverEnabled: true
            onClicked: if (picker.currentKey !== "") picker.dismissed()
        }

        Rectangle {
            id: pickerCard
            width: 540
            height: pickerCol.implicitHeight + 60
            anchors.centerIn: parent
            anchors.verticalCenterOffset: 20
            radius: 26
            color: "#0d1b20"
            border.width: 1
            border.color: "#2effffff"
            antialiasing: true
            scale: picker.shown ? 1.0 : 0.92
            Behavior on scale { NumberAnimation { duration: 180; easing.type: Easing.OutBack } }

            // Clicks on the card itself must not reach the backdrop
            MouseArea { anchors.fill: parent }

            Column {
                id: pickerCol
                x: 32; y: 30
                width: parent.width - 64
                spacing: 28

                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    color: "#ffffff"
                    font.family: "Inter"; font.pixelSize: 24; font.weight: Font.DemiBold
                    text: "Choose your Laptop Color Variant"
                }

                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 30

                    Repeater {
                        model: picker.options
                        delegate: Item {
                            id: opt
                            required property var modelData
                            readonly property bool picked: picker.pendingKey === modelData.key
                            width: 110; height: 156

                            Item {
                                id: box
                                width: 110; height: 110
                                scale: optMouse.pressed ? 0.94
                                       : ((optMouse.containsMouse || opt.picked) ? 1.05 : 1.0)
                                Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutQuad } }

                                // Selection ring
                                Rectangle {
                                    x: -7; y: -7
                                    width: parent.width + 14; height: parent.height + 14
                                    radius: 33
                                    color: "transparent"
                                    border.width: 3
                                    border.color: "#ffffff"
                                    antialiasing: true
                                    opacity: opt.picked ? 1.0 : 0.0
                                    Behavior on opacity { NumberAnimation { duration: 150 } }
                                }
                                // The color itself
                                Rectangle {
                                    anchors.fill: parent
                                    radius: 26
                                    color: opt.modelData.fill
                                    border.width: 1
                                    border.color: "#33ffffff"
                                    antialiasing: true
                                }
                            }

                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                y: 130
                                color: opt.picked ? "#ffffff" : "#99ffffff"
                                font.family: "Inter"; font.pixelSize: 16
                                font.weight: opt.picked ? Font.DemiBold : Font.Medium
                                text: opt.modelData.name
                                Behavior on color { ColorAnimation { duration: 150 } }
                            }

                            MouseArea {
                                id: optMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: picker.pendingKey = opt.modelData.key
                            }
                        }
                    }
                }

                // Greyed out until a swatch is picked; takes on the picked color as a live preview
                ActionButton {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 240
                    selected: true
                    active: picker.pendingOption !== null
                    label: "Save"
                    accentFill: picker.pendingOption ? picker.pendingOption.fill : picker.accentFill
                    accentInk:  picker.pendingOption ? picker.pendingOption.ink  : picker.accentInk
                    onClicked: picker.saved(picker.pendingKey)
                }
            }
        }
    }

    // ---------- Window content ----------

    Rectangle {
        anchors.fill: parent
        color: "#071013"
        radius: 70          // matches the corner curve baked into rectangle_4.png
        antialiasing: true

        // Drag area (top bar)
        MouseArea {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 80
            property point clickPos: Qt.point(0, 0)
            onPressed: (mouse) => clickPos = Qt.point(mouse.x, mouse.y)
            onPositionChanged: (mouse) => {
                window.x += mouse.x - clickPos.x
                window.y += mouse.y - clickPos.y
            }
        }

        Image { x: 0; y: 0; source: Qt.resolvedUrl("assets/rectangle_4.png") }

        // Title
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 29
            color: "#ffffff"
            font.family: "Inter"; font.pixelSize: 24; font.weight: Font.DemiBold
            text: "HP Pavilion 15 Gaming Control Center"
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 58
            color: "#ffffff"
            font.family: "Inter"; font.pixelSize: 16; font.weight: Font.Light
            text: "By @MIDO5659"
        }

        // Panel backgrounds
        Image { x: 41; y: 101; source: Qt.resolvedUrl("assets/rectangle_2.png") }
        Image { x: 35; y: 426; source: Qt.resolvedUrl("assets/rectangle_5.png") }
        Image { x: 689; y: 101; source: Qt.resolvedUrl("assets/rectangle_3.png") }

        // Laptop picture (supported) / prohibited sign (unsupported)
        Image {
            x: 41; y: 101
            source: Qt.resolvedUrl("assets/image_2.png")
            opacity: window.supported ? 1.0 : 0.0
            Behavior on opacity { NumberAnimation { duration: 250 } }
        }
        Image {
            width: 170; height: 170
            x: 41 + 149 - width / 2
            y: 101 + 147 - height / 2
            source: Qt.resolvedUrl("assets/no_symbol.png")
            fillMode: Image.PreserveAspectFit
            smooth: true; mipmap: true
            opacity: window.supported ? 0.0 : 1.0
            Behavior on opacity { NumberAnimation { duration: 250 } }
        }

        // Section headers
        Text {
            x: 357; y: 119
            color: "#ffffff"
            font.family: "Inter"; font.pixelSize: 24; font.weight: Font.Medium
            text: "System Information"
        }
        Text {
            x: 357; y: 288
            color: "#ffffff"
            font.family: "Inter"; font.pixelSize: 24; font.weight: Font.Medium
            text: "Battery Stats"
        }

        // System info rows (even 30px pitch)

        // Processor row: name followed by its support status
        Item {
            x: 356; y: 153
            width: 280; height: 24

            Item {
                width: 36; height: parent.height
                Image { anchors.centerIn: parent; source: Qt.resolvedUrl("assets/electronics.png") }
            }
            Row {
                x: 40
                height: parent.height
                spacing: 10
                Text {
                    height: parent.height
                    verticalAlignment: Text.AlignVCenter
                    color: "#ffffff"
                    font.family: "Inter"; font.pixelSize: 16; font.weight: Font.Medium
                    text: backend.cpuNameParsed
                }
                Text {
                    height: parent.height
                    verticalAlignment: Text.AlignVCenter
                    color: window.supported ? "#70ffb7" : "#ff6b6b"
                    font.family: "Inter"; font.pixelSize: 16; font.weight: Font.Medium
                    text: window.supported ? "Supported" : "Not Supported"
                }
            }
        }
        InfoRow { x: 356; y: 183; icon: Qt.resolvedUrl("assets/video_Card.png");  text: backend.gpuNameParsed }
        InfoRow { x: 356; y: 213; icon: Qt.resolvedUrl("assets/memory_Slot.png"); text: backend.ramSize }

        InfoRow { x: 356; y: 243; icon: Qt.resolvedUrl("assets/motherboard.png"); text: backend.moboName }

        // Battery rows
        InfoRow { x: 356; y: 321; icon: Qt.resolvedUrl("assets/full_Battery.png");     text: backend.battCharge }
        InfoRow { x: 356; y: 351; icon: Qt.resolvedUrl("assets/heart_with_Pulse.png"); text: backend.battHealth }

        // Live stats column (centered in the right panel, even 105px pitch)
        Item {
            x: 749; y: 133
            width: 193; height: 80
            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                color: "#ffffff"; font.family: "Angled"; font.pixelSize: 22
                text: backend.cpuTemp
            }
            Text {
                y: 28; width: parent.width
                horizontalAlignment: Text.AlignHCenter
                color: "#ffffff"; font.family: "AKONY"; font.pixelSize: 42; font.weight: Font.Bold
                text: backend.cpuWatts
            }
        }
        Item {
            x: 749; y: 238
            width: 193; height: 80
            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                color: "#ffffff"; font.family: "Angled"; font.pixelSize: 22
                text: backend.dgpuTemp
            }
            Text {
                y: 28; width: parent.width
                horizontalAlignment: Text.AlignHCenter
                color: "#ffffff"; font.family: "AKONY"; font.pixelSize: 42; font.weight: Font.Bold
                text: backend.dgpuWatts
            }
        }
        Item {
            x: 749; y: 343
            width: 193; height: 80
            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                color: "#ffffff"; font.family: "Angled"; font.pixelSize: 22
                text: "TOTAL"
            }
            Text {
                y: 28; width: parent.width
                horizontalAlignment: Text.AlignHCenter
                color: "#ffffff"; font.family: "AKONY"; font.pixelSize: 42; font.weight: Font.Bold
                text: backend.totalWatts
            }
        }

        // Fan section: spinning icon + status text + toggle button, laid out as one
        // group that is centered inside its panel (the panel image sits at 35,426, 636x210)
        Item {
            id: fanPanel
            x: 35; y: 426
            width: 636; height: 210

            readonly property bool turbo: backend.fanMode === "TURBO"
            // Text block is as wide as "TURBO ON" (or the button, if wider), so the group
            // never shifts or re-centers when the mode flips between TURBO and AUTO
            readonly property real blockWidth: Math.max(turboMeasure.implicitWidth, fanButton.width)

            Text {
                id: turboMeasure
                visible: false
                font.family: "AKONY"; font.pixelSize: 36; font.weight: Font.Bold
                text: "TURBO ON"
            }

            Row {
                anchors.centerIn: parent
                spacing: 32

                // Fan icon: spins slowly in AUTO, fast in TURBO (with a pulsing accent glow),
                // stands still when the device is unsupported. Speed eases between states.
                Item {
                    id: fanIcon
                    width: 109; height: 109
                    anchors.verticalCenter: parent.verticalCenter

                    property real angle: 0
                    property real targetSpeed: !window.supported ? 0 : (fanPanel.turbo ? 960 : 90)   // degrees per second
                    property real speed: targetSpeed
                    Behavior on speed { NumberAnimation { duration: 900; easing.type: Easing.InOutQuad } }

                    FrameAnimation {
                        id: fanTick
                        running: window.supported
                        onTriggered: fanIcon.angle = (fanIcon.angle + fanIcon.speed * fanTick.frameTime) % 360
                    }

                    // Turbo glow (disc + ring in the accent color), pulsing in step with the TURBO text
                    Item {
                        anchors.fill: parent
                        opacity: (window.supported && fanPanel.turbo) ? 1.0 : 0.0
                        Behavior on opacity { NumberAnimation { duration: 400; easing.type: Easing.InOutQuad } }

                        property real pulse: 0.0
                        SequentialAnimation on pulse {
                            running: window.supported && fanPanel.turbo
                            loops: Animation.Infinite
                            NumberAnimation { from: 0.0; to: 1.0; duration: 800; easing.type: Easing.InOutSine }
                            NumberAnimation { from: 1.0; to: 0.0; duration: 800; easing.type: Easing.InOutSine }
                        }

                        Rectangle {
                            anchors.centerIn: parent
                            width: 135; height: 135; radius: 68
                            color: window.themeAccent.vivid
                            opacity: 0.07 + 0.08 * parent.pulse
                            antialiasing: true
                        }
                        Rectangle {
                            anchors.centerIn: parent
                            width: 135; height: 135; radius: 68
                            color: "transparent"
                            border.width: 2
                            border.color: window.themeAccent.vivid
                            opacity: 0.30 + 0.40 * parent.pulse
                            antialiasing: true
                        }
                    }

                    Image {
                        anchors.fill: parent
                        source: Qt.resolvedUrl("assets/fan_Speed.png")
                        rotation: fanIcon.angle
                        smooth: true
                        antialiasing: true
                        opacity: window.supported ? 1.0 : 0.4
                        Behavior on opacity { NumberAnimation { duration: 200 } }
                    }
                }

                // Status text centered over the button
                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    width: fanPanel.blockWidth
                    spacing: 9

                    Item {
                        width: parent.width; height: 40
                        opacity: window.supported ? 1.0 : 0.4
                        Behavior on opacity { NumberAnimation { duration: 200 } }

                        Text {
                            id: turboText
                            anchors.fill: parent
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            color: window.themeAccent.vivid
                            font.family: "AKONY"; font.pixelSize: 36; font.weight: Font.Bold
                            text: "TURBO ON"
                            opacity: fanPanel.turbo ? 1.0 : 0.0

                            SequentialAnimation on opacity {
                                running: fanPanel.turbo
                                loops: Animation.Infinite
                                NumberAnimation { from: 1.0; to: 0.6; duration: 800; easing.type: Easing.InOutSine }
                                NumberAnimation { from: 0.6; to: 1.0; duration: 800; easing.type: Easing.InOutSine }
                            }
                            Behavior on opacity {
                                NumberAnimation { duration: 300; easing.type: Easing.InOutQuad }
                            }
                        }

                        Text {
                            id: autoText
                            anchors.fill: parent
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            color: "#ffffff"
                            font.family: "AKONY"; font.pixelSize: 36; font.weight: Font.Bold
                            text: "AUTO"
                            opacity: fanPanel.turbo ? 0.0 : 1.0
                            Behavior on opacity {
                                NumberAnimation { duration: 300; easing.type: Easing.InOutQuad }
                            }
                        }
                    }

                    // Fan toggle
                    ActionButton {
                        id: fanButton
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: 294; selected: true
                        accentFill: window.themeAccent.fill; accentInk: window.themeAccent.ink
                        active: window.supported
                        icon: Qt.resolvedUrl("assets/switches.png")
                        label: "Switch Turbo/Auto Fan mode"
                        onClicked: backend.toggleFan()
                    }
                }
            }
        }

        // Performance profiles
        ActionButton {
            x: 726; y: 455
            active: window.supported
            selected: backend.perfMode === "PERFORMANCE"
            accentFill: window.themeAccent.fill; accentInk: window.themeAccent.ink
            icon: Qt.resolvedUrl("assets/voltage.png")
            label: "Performance Mode"
            onClicked: if (backend.perfMode !== "PERFORMANCE") perfDialog.shown = true
        }
        ActionButton {
            x: 726; y: 505
            active: window.supported
            selected: backend.perfMode === "BALANCED"
            accentFill: window.themeAccent.fill; accentInk: window.themeAccent.ink
            icon: Qt.resolvedUrl("assets/scales.png")
            label: "Balanced Mode"
            onClicked: backend.setBalancedMode()
        }
        ActionButton {
            x: 726; y: 555
            active: window.supported
            selected: backend.perfMode === "QUIET"
            accentFill: window.themeAccent.fill; accentInk: window.themeAccent.ink
            icon: Qt.resolvedUrl("assets/leaf.png")
            label: "Quiet Mode"
            onClicked: backend.setQuietMode()
        }

        // Powered by (centered under the buttons)
        Text {
            x: 689 + (313 - width) / 2
            y: 605
            color: "#42ffffff"
            font.family: "Inter"; font.pixelSize: 14; font.weight: Font.Medium
            text: "Powered by RyzenADJ"
        }

        // Performance Mode warning
        ModalDialog {
            id: perfDialog
            accentFill: window.themeAccent.fill; accentInk: window.themeAccent.ink
            iconKind: "warning"
            title: "Performance Mode Warning"
            message: "Performance Mode pushes your processor past the power limits it was originally designed for. Running at these higher limits produces more heat and draws more power, which can cause instability and thermal throttling, and if used for long periods it may shorten the life of your hardware or cause permanent damage."
            note: "By pressing Continue you accept that you are using this feature entirely at your own risk. The developer (@MIDO5659) is not responsible or liable for any damage, data loss or any other issue that may happen to your device."
            primaryLabel: "Continue"
            secondaryLabel: "Back"
            onSecondaryClicked: shown = false
            onPrimaryClicked: {
                shown = false
                backend.setPerformanceMode()
            }
        }

        // Unsupported device notice (shown at startup)
        ModalDialog {
            id: unsupportedDialog
            accentFill: window.themeAccent.fill; accentInk: window.themeAccent.ink
            iconKind: "blocked"
            title: "Unsupported Processor"
            message: "This app only supports the Ryzen 7 3750H, Ryzen 5 4600H, Ryzen 7 4800H, Ryzen 5 5600H and Ryzen 7 5800H. Your detected processor is \"" + backend.cpuNameParsed + "\", so the controls will not work on this machine. You can still continue to check out the interface."
            note: "Fan and power controls are disabled on unsupported devices to protect your hardware."
            primaryLabel: "Continue"
            secondaryLabel: "Quit App"
            onSecondaryClicked: Qt.quit()
            onPrimaryClicked: {
                shown = false
                if (backend.accentVariant === "")
                    colorDialog.shown = true      // first launch: ask for the color next
            }
        }

        // Laptop color variant picker (shown on first launch, and from the palette button)
        ColorPickerDialog {
            id: colorDialog
            options: window.accentVariants
            currentKey: backend.accentVariant
            accentFill: window.themeAccent.fill
            accentInk: window.themeAccent.ink
            onSaved: (key) => {
                backend.saveAccentVariant(key)
                shown = false
            }
            onDismissed: shown = false
        }

        // Window controls (kept clear of the rounded corner)
        WindowButton {
            id: closeButton
            kind: "close"
            hoverColor: "#e53935"
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.topMargin: 22
            anchors.rightMargin: 38
            onClicked: Qt.quit()
        }
        WindowButton {
            id: minimizeButton
            kind: "minimize"
            hoverColor: "#f59e0b"
            anchors.top: closeButton.top
            anchors.right: closeButton.left
            anchors.rightMargin: 10
            onClicked: window.showMinimized()
        }
        WindowButton {
            id: colorButton
            kind: "palette"
            hoverColor: "#455a64"
            swatchColors: window.accentVariants.map(function(v) { return v.fill })
            anchors.top: closeButton.top
            anchors.right: minimizeButton.left
            anchors.rightMargin: 10
            onClicked: colorDialog.shown = true
        }
    }
}
