// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
//
// Adding an account: either the setup key typed in (what sites show under "can't scan the
// QR code?"), or the otpauth:// link pasted whole.
import QtQuick 1.1
import com.nokia.symbian 1.1

Page {
    id: page

    tools: ToolBarLayout {
        ToolButton { iconSource: "toolbar-back"; onClicked: pageStack.pop() }
    }

    Component { id: scanPage; ScanPage {} }

    function add() {
        var ok = app.addAccount(issuerField.text, nameField.text, secretField.text,
                                typeRow.checkedButton == hotpButton ? "hotp" : "totp",
                                algorithmRow.checkedButton.text,
                                parseInt(digitsRow.checkedButton.text),
                                parseInt(periodField.text), counterField.text)
        if (ok) pageStack.pop()
    }

    ListHeading {
        id: heading
        anchors { top: parent.top; left: parent.left; right: parent.right }
        ListItemText { anchors.fill: heading.paddingItem; role: "Heading"; text: qsTr("Add account") }
    }

    Flickable {
        id: flick
        anchors { top: heading.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
        contentHeight: column.height + 2 * platformStyle.paddingLarge
        clip: true

        Column {
            id: column
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: platformStyle.paddingLarge }
            spacing: platformStyle.paddingMedium

            Button {
                width: parent.width
                text: qsTr("Scan QR code")
                onClicked: pageStack.push(scanPage)
            }
            Label {
                width: parent.width
                wrapMode: Text.Wrap
                font.pixelSize: platformStyle.fontSizeSmall
                color: platformStyle.colorNormalMid
                text: qsTr("Or enter the setup key the site shows under \"can't scan the code?\":")
            }

            Label { text: qsTr("Service (issuer)") }
            TextField {
                id: issuerField
                width: parent.width
                placeholderText: qsTr("e.g. Google")
                inputMethodHints: Qt.ImhNoPredictiveText
            }
            Label { text: qsTr("Account") }
            TextField {
                id: nameField
                width: parent.width
                placeholderText: qsTr("e.g. alice@gmail.com")
                inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
            }
            Label { text: qsTr("Secret key") }
            TextField {
                id: secretField
                width: parent.width
                placeholderText: qsTr("letters A-Z and digits 2-7")
                inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
            }

            Label { text: qsTr("Type") }
            ButtonRow {
                id: typeRow
                width: parent.width
                Button { id: totpButton; text: qsTr("Time-based") }
                Button { id: hotpButton; text: qsTr("Counter-based") }
            }

            // Most sites use the defaults (SHA1, 6 digits, 30 s); a link carries any others.
            CheckBox {
                id: advanced
                text: qsTr("Advanced options")
            }
            Column {
                width: parent.width
                spacing: platformStyle.paddingMedium
                visible: advanced.checked

                Label { text: qsTr("Algorithm") }
                ButtonRow {
                    id: algorithmRow
                    width: parent.width
                    Button { text: "SHA1" }
                    Button { text: "SHA256" }
                    Button { text: "SHA512" }
                }
                Label { text: qsTr("Digits") }
                ButtonRow {
                    id: digitsRow
                    width: parent.width
                    Button { text: "6" }
                    Button { text: "7" }
                    Button { text: "8" }
                }
                Label { text: qsTr("Period, seconds"); visible: !hotpButton.checked }
                TextField {
                    id: periodField
                    visible: !hotpButton.checked
                    width: parent.width
                    text: "30"
                    inputMethodHints: Qt.ImhDigitsOnly
                }
            }
            Label { text: qsTr("Counter"); visible: hotpButton.checked }
            TextField {
                id: counterField
                visible: hotpButton.checked
                width: parent.width
                text: "0"
                inputMethodHints: Qt.ImhDigitsOnly
            }

            Button {
                width: parent.width
                text: qsTr("Add account")
                onClicked: page.add()
            }

            Item { width: 1; height: platformStyle.paddingLarge }

            Label {
                width: parent.width
                wrapMode: Text.Wrap
                text: qsTr("Or paste the otpauth:// link (the text inside the site's QR code):")
            }
            TextField {
                id: linkField
                width: parent.width
                placeholderText: "otpauth://totp/..."
                inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase | Qt.ImhUrlCharactersOnly
            }
            Button {
                width: parent.width
                text: qsTr("Add from link")
                enabled: linkField.text != ""
                onClicked: if (app.addFromLink(linkField.text)) pageStack.pop()
            }
        }
    }
}
