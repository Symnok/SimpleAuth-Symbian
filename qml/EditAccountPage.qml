// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
//
// Editing an account (as in Aegis): its service and account names can be changed; its
// secret key can be shown and copied (to set it up in another authenticator by hand), but
// not changed - it and the code settings are fixed by the site.
import QtQuick 1.1
import com.nokia.symbian 1.1

Page {
    id: page
    property int row: -1
    property bool showSecret: false
    property alias issuerField: issuerField   // for tools/autotest.qml
    property alias nameField: nameField

    tools: ToolBarLayout {
        ToolButton { iconSource: "toolbar-back"; onClicked: pageStack.pop() }
    }

    function save() {
        if (app.accounts.setNames(row, issuerField.text, nameField.text)) {
            app.showNotice(qsTr("Saved."))
            pageStack.pop()
        } else {
            app.showNotice(qsTr("Enter a name for the account."))
        }
    }

    // "ABCDEFGH..." -> "ABCD EFGH ...": easier to read off and type elsewhere.
    function grouped(key) {
        var out = ""
        for (var i = 0; i < key.length; i += 4) out += (i ? " " : "") + key.substr(i, 4)
        return out
    }

    ListHeading {
        id: heading
        anchors { top: parent.top; left: parent.left; right: parent.right }
        ListItemText { anchors.fill: heading.paddingItem; role: "Heading"; text: qsTr("Edit account") }
    }

    Flickable {
        anchors { top: heading.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
        contentHeight: column.height + 2 * platformStyle.paddingLarge
        clip: true

        Column {
            id: column
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: platformStyle.paddingLarge }
            spacing: platformStyle.paddingMedium

            Label { text: qsTr("Service (issuer)") }
            TextField {
                id: issuerField
                width: parent.width
                // A binding, not Component.onCompleted: pageStack.push() sets row only after
                // the page is made. Typing replaces the binding, as it should.
                text: app.accounts.issuer(page.row)
                inputMethodHints: Qt.ImhNoPredictiveText
            }
            Label { text: qsTr("Account") }
            TextField {
                id: nameField
                width: parent.width
                text: app.accounts.name(page.row)
                inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
            }
            Button {
                width: parent.width
                text: qsTr("Save")
                onClicked: page.save()
            }

            Item { width: 1; height: platformStyle.paddingLarge }

            Label { text: qsTr("Secret key") }
            Label {
                width: parent.width
                wrapMode: Text.WrapAnywhere
                font.family: "Courier"
                font.pixelSize: platformStyle.fontSizeLarge
                text: page.showSecret ? page.grouped(app.accounts.secret(page.row)) : "•••• •••• •••• ••••"
            }
            Row {
                spacing: platformStyle.paddingSmall
                Button {
                    width: (column.width - platformStyle.paddingSmall) / 2
                    text: page.showSecret ? qsTr("Hide") : qsTr("Show")
                    onClicked: page.showSecret = !page.showSecret
                }
                Button {
                    width: (column.width - platformStyle.paddingSmall) / 2
                    text: qsTr("Copy")
                    onClicked: {
                        app.copyText(app.accounts.secret(page.row))
                        app.showNotice(qsTr("Secret key copied. Anyone who has it can make your codes."))
                    }
                }
            }
            Label {
                width: parent.width
                wrapMode: Text.Wrap
                font.pixelSize: platformStyle.fontSizeSmall
                color: platformStyle.colorNormalMid
                text: app.accounts.details(page.row)
            }
            Label {
                width: parent.width
                wrapMode: Text.Wrap
                font.pixelSize: platformStyle.fontSizeSmall
                color: "#ffb347"
                text: qsTr("The key and these settings come from the site and cannot be changed. Show or copy the key only to set this account up in another authenticator.")
            }
        }
    }
}
