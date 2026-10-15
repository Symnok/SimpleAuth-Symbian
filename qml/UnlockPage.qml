// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
//
// The lock screen: the PIN or passphrase opens the encrypted accounts.
import QtQuick 1.1
import com.nokia.symbian 1.1

Page {
    id: page
    property bool busy: false
    property int wait: app.retrySeconds()
    property alias field: field   // for tools/autotest.qml

    tools: ToolBarLayout {
        ToolButton { iconSource: "toolbar-back"; onClicked: Qt.quit() }
    }

    function submit() {
        if (field.text == "" || page.busy || page.wait > 0) return
        page.busy = true
        work.start()   // let the busy indicator show before the half-second key derivation
    }
    Timer {
        id: work
        interval: 80
        onTriggered: {
            var secret = field.text
            field.text = ""
            app.unlock(secret)
            page.busy = false
            page.wait = app.retrySeconds()
        }
    }
    Timer {
        interval: 1000
        running: page.status == PageStatus.Active
        repeat: true
        onTriggered: page.wait = app.retrySeconds()
    }

    Column {
        anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; margins: platformStyle.paddingLarge * 2 }
        spacing: platformStyle.paddingLarge

        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            source: "qrc:/icon.svg"
            sourceSize.width: platformStyle.graphicSizeLarge * 1.5
            sourceSize.height: platformStyle.graphicSizeLarge * 1.5
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "SimpleAuth"
            font.pixelSize: platformStyle.fontSizeLarge * 1.3
        }
        TextField {
            id: field
            width: parent.width
            echoMode: TextInput.Password
            placeholderText: app.secretKind == "pin" ? qsTr("PIN") : qsTr("Passphrase")
            inputMethodHints: app.secretKind == "pin" ? Qt.ImhDigitsOnly | Qt.ImhNoPredictiveText
                                                      : Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase | Qt.ImhHiddenText
            enabled: !page.busy && page.wait == 0
            Keys.onReturnPressed: page.submit()
            Keys.onEnterPressed: page.submit()
        }
        Button {
            width: parent.width
            text: qsTr("Unlock")
            enabled: field.text != "" && !page.busy && page.wait == 0
            onClicked: page.submit()
        }
        Label {
            width: parent.width
            visible: page.wait > 0
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            color: "#ff8a80"
            text: qsTr("Too many wrong attempts. Try again in %1 s.").arg(page.wait)
        }
        BusyIndicator {
            anchors.horizontalCenter: parent.horizontalCenter
            running: page.busy
            visible: page.busy
        }
    }
}
