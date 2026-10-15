// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
import QtQuick 1.1
import com.nokia.symbian 1.1

Page {
    id: page

    tools: ToolBarLayout {
        ToolButton { iconSource: "toolbar-back"; onClicked: pageStack.pop() }
    }

    ListHeading {
        id: heading
        anchors { top: parent.top; left: parent.left; right: parent.right }
        ListItemText { anchors.fill: heading.paddingItem; role: "Heading"; text: qsTr("About") }
    }

    Flickable {
        anchors { top: heading.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
        contentHeight: column.height + 2 * platformStyle.paddingLarge
        clip: true

        Column {
            id: column
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: platformStyle.paddingLarge }
            spacing: platformStyle.paddingMedium

            Label { text: "SimpleAuth"; font.pixelSize: platformStyle.fontSizeLarge * 1.3 }
            Label { text: qsTr("version %1").arg(app.version); color: platformStyle.colorNormalMid }
            Label {
                text: qsTr("Qt %1, camera picture turned %2°").arg(app.qtVersion).arg(app.cameraRotation)
                font.pixelSize: platformStyle.fontSizeSmall
                color: platformStyle.colorNormalMid
            }
            Label {
                width: parent.width; wrapMode: Text.Wrap
                text: qsTr("Two-factor sign-in codes for Symbian Anna and Belle, compatible with Google Authenticator: time-based (TOTP) and counter-based (HOTP) codes, SHA1/SHA256/SHA512, 6-8 digits.")
            }
            Label {
                width: parent.width; wrapMode: Text.Wrap
                font.pixelSize: platformStyle.fontSizeSmall
                color: "#ffb347"
                text: qsTr("The keys are stored encrypted (AES-256) under your PIN or passphrase. A forgotten PIN cannot be reset: keep your sites' recovery codes.")
            }
            Label {
                width: parent.width; wrapMode: Text.Wrap
                font.pixelSize: platformStyle.fontSizeSmall
                color: platformStyle.colorNormalMid
                text: qsTr("Works offline; the app never uses the network. GPL-3.0-or-later. QR code reading: ZXing (Apache License 2.0).")
            }
        }
    }
}
