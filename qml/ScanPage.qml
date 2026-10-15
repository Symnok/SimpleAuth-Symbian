// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
//
// Scanning the QR code a site shows when two-factor sign-in is turned on. The camera runs
// only while this page is on screen and the app is in front. Cameras differ between phones,
// so the picture's rotation, the focus mode and the zoom can be set here; each is kept.
import QtQuick 1.1
import com.nokia.symbian 1.1
import SimpleAuth 1.0

Page {
    id: page
    property alias focusDialog: focusDialog   // for tools/autotest.qml

    tools: ToolBarLayout {
        ToolButton { iconSource: "toolbar-back"; onClicked: pageStack.pop() }
        // A quarter turn per tap. Reading works whichever way the picture shows.
        ToolButton { text: qsTr("Rotate"); onClicked: app.cameraRotation = app.cameraRotation + 90 }
        ToolButton { text: qsTr("Focus"); onClicked: page.openFocusDialog() }
    }

    function handle(text) {
        var lower = text.toLowerCase()
        if (lower.indexOf("otpauth-migration:") == 0) {
            app.showNotice(qsTr("This is a Google Authenticator export code. Importing those is not supported yet - scan the code the site shows instead."))
        } else if (lower.indexOf("otpauth:") == 0) {
            if (app.addFromLink(text)) {
                scanner.active = false
                pageStack.pop(null)   // back to the account list
            }
        } else {
            app.showNotice(qsTr("This QR code is not a two-factor setup code."))
        }
    }

    function focusName(mode) {
        if (mode == "") return qsTr("Automatic")
        if (mode == "auto") return qsTr("Autofocus")
        if (mode == "continuous") return qsTr("Continuous")
        if (mode == "macro") return qsTr("Macro (close up)")
        if (mode == "hyperfocal") return qsTr("Hyperfocal (fixed)")
        if (mode == "infinity") return qsTr("Infinity")
        if (mode == "manual") return qsTr("Manual")
        return mode
    }

    function openFocusDialog() {
        focusChoices.clear()
        focusChoices.append({ code: "" })
        for (var i = 0; i < scanner.focusModes.length; ++i)
            focusChoices.append({ code: scanner.focusModes[i] })
        focusDialog.open()
    }

    QrScanner {
        id: scanner
        anchors.fill: parent
        active: page.status == PageStatus.Active && (typeof symbian == "undefined" || symbian.foreground)
        frameRotation: app.cameraRotation
        focusMode: app.cameraFocusMode
        zoom: app.cameraZoom
        onFound: page.handle(text)

        MouseArea {
            anchors.fill: parent
            onClicked: scanner.refocus()
        }
    }

    // Where to hold the code.
    Rectangle {
        anchors.centerIn: parent
        width: Math.min(parent.width, parent.height) * 0.7
        height: width
        radius: 12
        color: "transparent"
        border.color: "white"
        border.width: 3
        opacity: 0.8
        visible: scanner.status == "scanning"
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: scanner.status == "starting"
        visible: running
        width: platformStyle.graphicSizeLarge
        height: width
    }

    // What the camera reports, for telling what a phone needs.
    Label {
        anchors { top: parent.top; left: parent.left; right: parent.right; margins: platformStyle.paddingSmall }
        wrapMode: Text.Wrap
        font.pixelSize: platformStyle.fontSizeSmall * 0.8
        color: "#c0ffffff"
        text: scanner.cameraInfo
    }

    // Zoom: lets a camera that will not focus close up stay back and still fill the frame.
    Column {
        anchors { right: parent.right; rightMargin: platformStyle.paddingSmall; verticalCenter: parent.verticalCenter }
        visible: scanner.maximumZoom > 1
        spacing: platformStyle.paddingSmall
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: scanner.zoom.toFixed(1) + "x"
            color: "white"
            font.pixelSize: platformStyle.fontSizeSmall
        }
        Slider {
            id: zoomSlider
            orientation: Qt.Vertical
            inverted: true   // zoom in upwards
            height: page.height * 0.45
            minimumValue: 1
            maximumValue: Math.min(scanner.maximumZoom, 8)
            stepSize: 0.1
            value: app.cameraZoom
            onValueChanged: if (pressed) app.cameraZoom = value
            onPressedChanged: if (!pressed) app.cameraZoom = value
        }
    }

    Rectangle {
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        height: hint.height + 2 * platformStyle.paddingLarge
        color: "#b0000000"
        Label {
            id: hint
            anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; margins: platformStyle.paddingLarge }
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            color: scanner.status == "error" ? "#ff8a80" : "white"
            text: scanner.status == "error" ? scanner.errorString
                                            : qsTr("Point the camera at the QR code. Tap the picture to focus. Blurred? Try Focus > Macro, or stay back and zoom in.")
        }
    }

    ListModel { id: focusChoices }

    SelectionDialog {
        id: focusDialog
        titleText: qsTr("Focus")
        model: focusChoices
        // The stock delegate shows modelData (a string list); this model has roles, and the
        // phone theme's dialog text is hard to read - so: our own rows, white on the dialog.
        delegate: Item {
            width: parent ? parent.width : 300
            height: (typeof privateStyle != "undefined") ? privateStyle.menuItemHeight : 56
            Rectangle { anchors.fill: parent; color: rowMouse.pressed ? "#3d5a80" : "transparent" }
            Label {
                anchors { left: parent.left; leftMargin: platformStyle.paddingLarge; right: parent.right; verticalCenter: parent.verticalCenter }
                text: page.focusName(model.code) + (model.code == app.cameraFocusMode ? "   *" : "")
                color: "white"
                elide: Text.ElideRight
            }
            MouseArea {
                id: rowMouse
                anchors.fill: parent
                onClicked: { focusDialog.selectedIndex = index; focusDialog.accept() }
            }
        }
        onAccepted: if (selectedIndex >= 0) app.cameraFocusMode = focusChoices.get(selectedIndex).code
    }
}
