// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
//
// An account's otpauth:// link as a QR code, for another authenticator (a new phone,
// Google Authenticator) to scan.
import QtQuick 1.1
import com.nokia.symbian 1.1
import SimpleAuth 1.0

Page {
    id: page
    property string link
    property string title

    tools: ToolBarLayout {
        ToolButton { iconSource: "toolbar-back"; onClicked: pageStack.pop() }
    }

    ListHeading {
        id: heading
        anchors { top: parent.top; left: parent.left; right: parent.right }
        ListItemText { anchors.fill: heading.paddingItem; role: "Heading"; text: page.title }
    }

    Flickable {
        anchors { top: heading.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
        contentHeight: column.height + 2 * platformStyle.paddingLarge
        clip: true

        Column {
            id: column
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: platformStyle.paddingLarge }
            spacing: platformStyle.paddingLarge

            QrCode {
                id: code
                width: parent.width
                height: width
                text: page.link
            }
            Label {
                width: parent.width
                visible: !code.valid
                wrapMode: Text.Wrap
                text: qsTr("This account's link is too long for a QR code.")
            }
            Label {
                width: parent.width
                wrapMode: Text.Wrap
                font.pixelSize: platformStyle.fontSizeSmall
                color: "#ffb347"
                text: qsTr("Anyone who sees this code can make your sign-in codes. Show it only to your own devices.")
            }
        }
    }
}
