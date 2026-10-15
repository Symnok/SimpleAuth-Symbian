// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
//
// Choosing a new PIN or passphrase: the kind, then the secret twice. Shared by the setup
// and change pages.
import QtQuick 1.1
import com.nokia.symbian 1.1

Column {
    id: root
    property string kind: pinButton.checked ? "pin" : "passphrase"
    property alias secret: first.text
    property alias confirm: second.text
    // Both fields take this directly: binding the second to the first's inputMethodHints does
    // not carry the value over on the phone (the confirmation got the full keyboard).
    property int hints: kind == "pin" ? Qt.ImhDigitsOnly | Qt.ImhNoPredictiveText
                                      : Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase | Qt.ImhHiddenText
    spacing: platformStyle.paddingMedium

    function clear() { first.text = ""; second.text = "" }

    ButtonRow {
        width: parent.width
        Button { id: pinButton; text: qsTr("PIN") }
        Button { text: qsTr("Passphrase") }
    }
    Label {
        width: parent.width
        wrapMode: Text.Wrap
        font.pixelSize: platformStyle.fontSizeSmall
        color: platformStyle.colorNormalMid
        text: root.kind == "pin"
              ? qsTr("At least 4 digits. Quick to type; guards against someone picking up the phone.")
              : qsTr("At least 8 characters. Also protects the accounts if the phone's files are copied.")
    }
    TextField {
        id: first
        width: parent.width
        echoMode: TextInput.Password
        placeholderText: root.kind == "pin" ? qsTr("New PIN") : qsTr("New passphrase")
        inputMethodHints: root.hints
    }
    TextField {
        id: second
        width: parent.width
        echoMode: TextInput.Password
        placeholderText: root.kind == "pin" ? qsTr("The same PIN again") : qsTr("The same passphrase again")
        inputMethodHints: root.hints
    }
}
