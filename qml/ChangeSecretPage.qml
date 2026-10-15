// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
//
// Changing the PIN or passphrase (or switching between the two): the accounts are encrypted
// again under the new one.
import QtQuick 1.1
import com.nokia.symbian 1.1

Page {
    id: page
    property bool busy: false

    tools: ToolBarLayout {
        ToolButton { iconSource: "toolbar-back"; onClicked: pageStack.pop() }
    }

    Timer {
        id: work
        interval: 80
        onTriggered: {
            var ok = app.changeSecret(current.text, fields.secret, fields.confirm, fields.kind)
            page.busy = false
            if (ok) pageStack.pop()
        }
    }

    ListHeading {
        id: heading
        anchors { top: parent.top; left: parent.left; right: parent.right }
        ListItemText { anchors.fill: heading.paddingItem; role: "Heading"; text: qsTr("Change PIN or passphrase") }
    }

    Flickable {
        anchors { top: heading.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
        contentHeight: column.height + 2 * platformStyle.paddingLarge
        clip: true

        Column {
            id: column
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: platformStyle.paddingLarge }
            spacing: platformStyle.paddingMedium

            Label { text: app.secretKind == "pin" ? qsTr("Current PIN") : qsTr("Current passphrase") }
            TextField {
                id: current
                width: parent.width
                echoMode: TextInput.Password
                inputMethodHints: app.secretKind == "pin" ? Qt.ImhDigitsOnly | Qt.ImhNoPredictiveText
                                                          : Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase | Qt.ImhHiddenText
            }
            Item { width: 1; height: platformStyle.paddingMedium }
            SecretFields { id: fields; width: parent.width }
            Button {
                width: parent.width
                text: qsTr("Change")
                enabled: !page.busy && current.text != "" && fields.secret != "" && fields.confirm != ""
                onClicked: { page.busy = true; work.start() }
            }
            BusyIndicator {
                anchors.horizontalCenter: parent.horizontalCenter
                running: page.busy
                visible: page.busy
            }
        }
    }
}
