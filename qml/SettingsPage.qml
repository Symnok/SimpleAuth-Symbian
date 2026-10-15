// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
//
// The clock check: time-based codes are only right when the phone's UTC time is. The page
// shows it next to a correction the user can step, to compare with an accurate clock.
// Then the lock: auto-lock time and changing the PIN or passphrase.
import QtQuick 1.1
import com.nokia.symbian 1.1

Page {
    id: page
    property string utc: app.utcTime()

    tools: ToolBarLayout {
        ToolButton { iconSource: "toolbar-back"; onClicked: pageStack.pop() }
    }

    Timer {
        interval: 250
        running: page.status == PageStatus.Active
        repeat: true
        onTriggered: page.utc = app.utcTime()
    }

    function offsetText(s) {
        return (s > 0 ? "+" : "") + s + " " + qsTr("s")
    }

    ListHeading {
        id: heading
        anchors { top: parent.top; left: parent.left; right: parent.right }
        ListItemText { anchors.fill: heading.paddingItem; role: "Heading"; text: qsTr("Settings") }
    }

    property alias flickable: flick   // for tools/autotest.qml

    Flickable {
        id: flick
        anchors { top: heading.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
        contentHeight: column.height + 2 * platformStyle.paddingLarge
        clip: true

        Column {
            id: column
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: platformStyle.paddingLarge }
            spacing: platformStyle.paddingMedium

            Label { text: qsTr("Clock (UTC) used for codes") }
            Label {
                text: page.utc
                font.pixelSize: platformStyle.fontSizeLarge * 1.8
                font.bold: true
            }
            Label {
                width: parent.width
                wrapMode: Text.Wrap
                font.pixelSize: platformStyle.fontSizeSmall
                color: platformStyle.colorNormalMid
                text: qsTr("Phone time zone: UTC%1. Compare the clock above with an accurate UTC clock (e.g. time.is/UTC). If it is off by more than a few seconds, codes may be refused: fix the phone's time and time zone, or correct it here.").arg(app.zoneOffset())
            }

            Label { text: qsTr("Correction: %1").arg(page.offsetText(app.timeOffset)) }
            Row {
                spacing: platformStyle.paddingSmall
                Button { width: (column.width - 3 * platformStyle.paddingSmall) / 4; text: "-10"; onClicked: app.timeOffset = app.timeOffset - 10 }
                Button { width: (column.width - 3 * platformStyle.paddingSmall) / 4; text: "-1"; onClicked: app.timeOffset = app.timeOffset - 1 }
                Button { width: (column.width - 3 * platformStyle.paddingSmall) / 4; text: "+1"; onClicked: app.timeOffset = app.timeOffset + 1 }
                Button { width: (column.width - 3 * platformStyle.paddingSmall) / 4; text: "+10"; onClicked: app.timeOffset = app.timeOffset + 10 }
            }
            Button {
                width: parent.width
                text: qsTr("Reset correction")
                enabled: app.timeOffset != 0
                onClicked: app.timeOffset = 0
            }

            Item { width: 1; height: platformStyle.paddingLarge }
            Label { text: qsTr("Security") }
            Button {
                width: parent.width
                text: qsTr("Auto-lock: %1").arg(page.autoLockName(app.autoLock))
                onClicked: autoLockDialog.open()
            }
            Button {
                width: parent.width
                text: app.secretKind == "pin" ? qsTr("Change PIN") : qsTr("Change passphrase")
                onClicked: pageStack.push(changePage)
            }

            Item { width: 1; height: platformStyle.paddingLarge }
            Label { text: qsTr("Backup") }
            Button {
                width: parent.width
                text: qsTr("Folder: %1").arg(page.nativePath(app.backupDir))
                onClicked: page.openFolderDialog()
            }
            Row {
                spacing: platformStyle.paddingSmall
                Button {
                    width: (column.width - platformStyle.paddingSmall) / 2
                    text: qsTr("Back up now")
                    onClicked: app.backup()
                }
                Button {
                    width: (column.width - platformStyle.paddingSmall) / 2
                    text: qsTr("Restore")
                    onClicked: pageStack.push(restorePage)
                }
            }
            Label {
                width: parent.width
                wrapMode: Text.Wrap
                font.pixelSize: platformStyle.fontSizeSmall
                color: platformStyle.colorNormalMid
                text: app.secretKind == "pin"
                      ? qsTr("A backup is encrypted with the current PIN; restoring it asks for that PIN, even after the PIN here has changed. Keep a copy off the phone.")
                      : qsTr("A backup is encrypted with the current passphrase; restoring it asks for that passphrase, even after it has changed here. Keep a copy off the phone.")
            }
        }
    }

    Component { id: changePage; ChangeSecretPage {} }
    Component { id: restorePage; RestorePage {} }

    function nativePath(path) { return path.replace(/\//g, "\\") }

    function openFolderDialog() {
        folderChoices.clear()
        var dirs = app.backupDirChoices()
        for (var i = 0; i < dirs.length; ++i) folderChoices.append({ path: dirs[i] })
        folderDialog.open()
    }

    ListModel { id: folderChoices }

    SelectionDialog {
        id: folderDialog
        titleText: qsTr("Backup folder")
        model: folderChoices
        delegate: Item {
            width: parent ? parent.width : 300
            height: (typeof privateStyle != "undefined") ? privateStyle.menuItemHeight : 56
            Rectangle { anchors.fill: parent; color: folderMouse.pressed ? "#3d5a80" : "transparent" }
            Label {
                anchors { left: parent.left; leftMargin: platformStyle.paddingLarge; right: parent.right; verticalCenter: parent.verticalCenter }
                text: page.nativePath(model.path) + (model.path == app.backupDir ? "   *" : "")
                color: "white"
            }
            MouseArea {
                id: folderMouse
                anchors.fill: parent
                onClicked: { folderDialog.selectedIndex = index; folderDialog.accept() }
            }
        }
        onAccepted: if (selectedIndex >= 0) app.backupDir = folderChoices.get(selectedIndex).path
    }

    function autoLockName(seconds) {
        if (seconds == 0) return qsTr("at once")
        if (seconds < 60) return qsTr("after %1 s").arg(seconds)
        return qsTr("after %1 min").arg(Math.round(seconds / 60))
    }

    SelectionDialog {
        id: autoLockDialog
        titleText: qsTr("Lock when the app has been in the background")
        model: ListModel {
            ListElement { seconds: 0 }
            ListElement { seconds: 60 }
            ListElement { seconds: 300 }
            ListElement { seconds: 900 }
        }
        // The stock delegate shows modelData (a string list); this model has roles, and the
        // phone theme's dialog text is hard to read - so: our own rows, white on the dialog.
        delegate: Item {
            width: parent ? parent.width : 300
            height: (typeof privateStyle != "undefined") ? privateStyle.menuItemHeight : 56
            Rectangle { anchors.fill: parent; color: rowMouse.pressed ? "#3d5a80" : "transparent" }
            Label {
                anchors { left: parent.left; leftMargin: platformStyle.paddingLarge; right: parent.right; verticalCenter: parent.verticalCenter }
                text: page.autoLockName(model.seconds) + (model.seconds == app.autoLock ? "   *" : "")
                color: "white"
            }
            MouseArea {
                id: rowMouse
                anchors.fill: parent
                onClicked: { autoLockDialog.selectedIndex = index; autoLockDialog.accept() }
            }
        }
        onAccepted: if (selectedIndex >= 0) app.autoLock = model.get(selectedIndex).seconds
    }
}
