// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
//
// The window: a page stack that follows app.state (setup -> locked <-> unlocked), the
// notice banner, and the foreground signal auto-lock needs.
import QtQuick 1.1
import com.nokia.symbian 1.1
import com.nokia.extras 1.1

PageStackWindow {
    id: window
    showStatusBar: true
    showToolBar: true
    platformSoftwareInputPanelEnabled: true

    Component { id: setupPage; SetupPage {} }
    Component { id: unlockPage; UnlockPage {} }
    Component { id: accountsPage; AccountsPage {} }

    property string routedState

    function route() {
        routedState = app.state
        pageStack.clear()
        if (app.state == "setup") pageStack.push(setupPage, {}, true)
        else if (app.state == "locked") pageStack.push(unlockPage, {}, true)
        else pageStack.push(accountsPage, {}, true)
    }

    Component.onCompleted: route()

    Connections {
        target: app
        // Changing the PIN re-announces the state without changing it: stay put then.
        onStateChanged: if (app.state != window.routedState) route()
        onNotice: {
            banner.text = text
            banner.open()
        }
    }

    Connections {
        target: typeof symbian != "undefined" ? symbian : null
        onForegroundChanged: app.setForeground(symbian.foreground)
    }

    InfoBanner {
        id: banner
        timeout: 3000
    }

    // Desktop testing only (see main.cpp): empty on the phone, so nothing is loaded.
    Loader { source: saAutotest }
}
