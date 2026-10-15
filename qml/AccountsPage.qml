// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
//
// The account list: each row's code with a countdown bar. Tap copies the code; press and
// hold for the row menu.
import QtQuick 1.1
import com.nokia.symbian 1.1

Page {
    id: page

    Component { id: addPage; AddAccountPage {} }
    Component { id: settingsPage; SettingsPage {} }
    Component { id: aboutPage; AboutPage {} }
    Component { id: showQrPage; ShowQrPage {} }
    Component { id: editPage; EditAccountPage {} }

    tools: ToolBarLayout {
        ToolButton { iconSource: "toolbar-back"; onClicked: Qt.quit() }
        ToolButton { iconSource: "toolbar-add"; onClicked: pageStack.push(addPage) }
        ToolButton { iconSource: "toolbar-menu"; onClicked: menu.open() }
    }

    Menu {
        id: menu
        MenuLayout {
            MenuItem { text: qsTr("Settings"); onClicked: pageStack.push(settingsPage) }
            MenuItem { text: qsTr("About"); onClicked: pageStack.push(aboutPage) }
            MenuItem { text: qsTr("Lock now"); onClicked: app.lock() }
            // The same as the Back button on this page.
            MenuItem { text: qsTr("Exit"); onClicked: Qt.quit() }
        }
    }

    ContextMenu {
        id: contextMenu
        property int row: -1
        property bool hotp: false
        MenuLayout {
            MenuItem { text: qsTr("Copy code"); onClicked: app.copyCode(contextMenu.row) }
            MenuItem {
                text: qsTr("Next code")
                visible: contextMenu.hotp
                onClicked: app.accounts.nextCode(contextMenu.row)
            }
            MenuItem {
                text: qsTr("Edit")
                onClicked: pageStack.push(editPage, { row: contextMenu.row })
            }
            MenuItem {
                text: qsTr("Show QR code")
                onClicked: pageStack.push(showQrPage, { link: app.accounts.link(contextMenu.row),
                                                        title: app.accounts.title(contextMenu.row) })
            }
            MenuItem {
                text: qsTr("Delete")
                onClicked: {
                    deleteDialog.row = contextMenu.row
                    deleteDialog.title = app.accounts.title(contextMenu.row)
                    deleteDialog.open()
                }
            }
        }
    }

    QueryDialog {
        id: deleteDialog
        property int row: -1
        property string title
        titleText: qsTr("Delete account")
        message: qsTr("Delete \"%1\"? Make sure two-factor sign-in is turned off at the site or you have another way in - the key cannot be recovered.").arg(title)
        acceptButtonText: qsTr("Delete")
        rejectButtonText: qsTr("Cancel")
        onAccepted: app.accounts.remove(row)
    }

    ListHeading {
        id: heading
        anchors { top: parent.top; left: parent.left; right: parent.right }
        ListItemText { anchors.fill: heading.paddingItem; role: "Heading"; text: qsTr("Accounts") }
    }

    ListView {
        id: list
        anchors { top: heading.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
        model: app.accounts
        clip: true

        delegate: ListItem {
            id: item
            height: codeText.height + titleText.height + platformStyle.paddingLarge * 2 + 6
            property bool urgent: !model.hotp && model.remaining <= 5

            onClicked: app.copyCode(index)
            onPressAndHold: {
                contextMenu.row = index
                contextMenu.hotp = model.hotp
                contextMenu.open()
            }

            Column {
                anchors {
                    left: item.paddingItem.left
                    right: side.left; rightMargin: platformStyle.paddingMedium
                    verticalCenter: parent.verticalCenter
                }
                Label {
                    id: titleText
                    width: parent.width
                    elide: Text.ElideRight
                    font.pixelSize: platformStyle.fontSizeSmall
                    color: platformStyle.colorNormalMid
                    text: model.issuer != "" && model.name != "" ? model.issuer + " (" + model.name + ")"
                                                                  : model.issuer + model.name
                }
                Label {
                    id: codeText
                    text: model.code
                    font.pixelSize: platformStyle.fontSizeLarge * 1.6
                    font.bold: true
                    color: item.urgent ? "#ff6b6b" : "#6bb8ff"
                }
            }

            // TOTP: seconds left; HOTP: a button for the next code.
            Item {
                id: side
                anchors { right: item.paddingItem.right; verticalCenter: parent.verticalCenter }
                width: platformStyle.graphicSizeMedium
                height: width
                Label {
                    anchors.centerIn: parent
                    visible: !model.hotp
                    text: model.remaining
                    color: item.urgent ? "#ff6b6b" : platformStyle.colorNormalLight
                }
                ToolButton {
                    anchors.centerIn: parent
                    visible: model.hotp
                    iconSource: "toolbar-refresh"
                    onClicked: app.accounts.nextCode(index)
                }
            }

            // The countdown bar along the bottom of the row.
            Rectangle {
                visible: !model.hotp
                anchors { left: parent.left; bottom: parent.bottom; bottomMargin: 2 }
                height: 3
                width: parent.width * model.remaining / model.period
                color: item.urgent ? "#ff6b6b" : "#3d7ed6"
            }
        }
    }

    Label {
        anchors { centerIn: list; }
        width: parent.width - 2 * platformStyle.paddingLarge
        visible: list.count == 0
        wrapMode: Text.Wrap
        horizontalAlignment: Text.AlignHCenter
        color: platformStyle.colorNormalMid
        text: qsTr("No accounts yet.\nTap + to add one with the key or link the site shows when you turn on two-factor sign-in.")
    }
}
