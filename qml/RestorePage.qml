// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
//
// Restoring a backup: pick a file from the backup folder, give the PIN or passphrase it was
// made with, and its accounts are added to the ones here (encrypted under this phone's PIN).
import QtQuick 1.1
import com.nokia.symbian 1.1

Page {
    id: page
    property bool busy: false
    property string chosen
    property string chosenKind

    tools: ToolBarLayout {
        ToolButton { iconSource: "toolbar-back"; onClicked: pageStack.pop() }
    }

    function reload() {
        files.clear()
        var list = app.backupFiles()
        for (var i = 0; i < list.length; ++i) files.append({ name: list[i] })
    }
    Component.onCompleted: reload()

    // "simpleauth-2026-09-24-22-34.sauth" -> "2026-09-24 22:34": the full name does not fit
    // a phone's width. Other .sauth names are shown as they are.
    function label(name) {
        var m = /^simpleauth-(\d{4}-\d\d-\d\d)-(\d\d)-(\d\d)\.sauth$/.exec(name)
        return m ? m[1] + "  " + m[2] + ":" + m[3] : name
    }

    function choose(name) {
        page.chosen = name
        page.chosenKind = app.backupKind(name)
        secret.text = ""
        if (page.chosenKind == "") app.showNotice(qsTr("%1 is not a SimpleAuth backup.").arg(name))
    }

    Timer {
        id: work
        interval: 80
        onTriggered: {
            var ok = app.restore(page.chosen, secret.text)
            secret.text = ""
            page.busy = false
            if (ok) pageStack.pop()
        }
    }

    ListModel { id: files }

    ListHeading {
        id: heading
        anchors { top: parent.top; left: parent.left; right: parent.right }
        ListItemText { anchors.fill: heading.paddingItem; role: "Heading"; text: qsTr("Restore") }
    }

    Label {
        id: folderLabel
        anchors { top: heading.bottom; left: parent.left; right: parent.right; margins: platformStyle.paddingLarge }
        wrapMode: Text.Wrap
        font.pixelSize: platformStyle.fontSizeSmall
        color: platformStyle.colorNormalMid
        text: files.count == 0 ? qsTr("No backups (*.sauth) in %1. Choose the folder in Settings.").arg(app.backupDir.replace(/\//g, "\\"))
                               : qsTr("Backups in %1:").arg(app.backupDir.replace(/\//g, "\\"))
    }

    ListView {
        id: list
        anchors { top: folderLabel.bottom; topMargin: platformStyle.paddingSmall; left: parent.left; right: parent.right; bottom: form.top }
        clip: true
        model: files
        delegate: ListItem {
            id: row
            onClicked: page.choose(model.name)
            ListItemText {
                anchors { left: row.paddingItem.left; right: row.paddingItem.right; verticalCenter: parent.verticalCenter }
                role: "Title"
                text: page.label(model.name) + (model.name == page.chosen ? "   *" : "")
            }
        }
    }

    // The chosen file's PIN or passphrase.
    Column {
        id: form
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom; margins: platformStyle.paddingLarge }
        spacing: platformStyle.paddingMedium
        visible: page.chosen != "" && page.chosenKind != ""
        height: visible ? implicitHeight : 0

        Label {
            width: parent.width
            wrapMode: Text.Wrap
            text: page.chosenKind == "pin" ? qsTr("PIN of %1:").arg(page.chosen) : qsTr("Passphrase of %1:").arg(page.chosen)
        }
        TextField {
            id: secret
            width: parent.width
            echoMode: TextInput.Password
            inputMethodHints: page.chosenKind == "pin" ? Qt.ImhDigitsOnly | Qt.ImhNoPredictiveText
                                                       : Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase | Qt.ImhHiddenText
            enabled: !page.busy
        }
        Button {
            width: parent.width
            text: qsTr("Restore and merge")
            enabled: secret.text != "" && !page.busy
            onClicked: { page.busy = true; work.start() }
        }
        BusyIndicator {
            anchors.horizontalCenter: parent.horizontalCenter
            running: page.busy
            visible: page.busy
        }
    }
}
