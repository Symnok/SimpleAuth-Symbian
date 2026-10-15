// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
//
// First start (or the first start after updating from a version without the lock): a PIN
// or passphrase must be set before any code is shown.
import QtQuick 1.1
import com.nokia.symbian 1.1

Page {
    id: page
    property bool busy: false
    property alias fields: fields   // for tools/autotest.qml

    tools: ToolBarLayout {
        ToolButton { iconSource: "toolbar-back"; onClicked: Qt.quit() }
    }

    function submit() {
        page.busy = true
        work.start()   // let the busy indicator show before the half-second key derivation
    }
    Timer {
        id: work
        interval: 80
        onTriggered: {
            app.setupSecret(fields.secret, fields.confirm, fields.kind)
            page.busy = false
        }
    }

    ListHeading {
        id: heading
        anchors { top: parent.top; left: parent.left; right: parent.right }
        ListItemText { anchors.fill: heading.paddingItem; role: "Heading"; text: qsTr("Protect your accounts") }
    }

    Flickable {
        anchors { top: heading.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
        contentHeight: column.height + 2 * platformStyle.paddingLarge
        clip: true

        Column {
            id: column
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: platformStyle.paddingLarge }
            spacing: platformStyle.paddingMedium

            Label {
                width: parent.width
                wrapMode: Text.Wrap
                text: app.hasUnprotectedAccounts
                      ? qsTr("Choose a PIN or passphrase. Your accounts will be encrypted with it, and it will be asked for to open the app.")
                      : qsTr("Choose a PIN or passphrase. The accounts you add are encrypted with it, and it is asked for to open the app.")
            }
            SecretFields { id: fields; width: parent.width }
            Label {
                width: parent.width
                wrapMode: Text.Wrap
                font.pixelSize: platformStyle.fontSizeSmall
                color: "#ffb347"
                text: qsTr("If you forget it, the accounts cannot be recovered - there is no reset. Keep each site's recovery codes somewhere safe.")
            }
            Button {
                width: parent.width
                text: fields.kind == "pin" ? qsTr("Set PIN") : qsTr("Set passphrase")
                enabled: !page.busy && fields.secret != "" && fields.confirm != ""
                onClicked: page.submit()
            }
            BusyIndicator {
                anchors.horizontalCenter: parent.horizontalCenter
                running: page.busy
                visible: page.busy
            }
        }
    }
}
