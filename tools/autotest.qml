// Desktop testing only: walks every page of the app and saves a screenshot of each
// (SA_SHOT_DIR), so QML binding errors and layout slips show up on a PC before a build goes
// to the phone. Loaded by main.qml when SA_AUTOTEST names this file (tools\build-desktop.cmd
// test sets it up); never part of the phone package.
//
// The steps are an array of functions, not a switch: Qt 4.7's JavaScriptCore drops the
// statements that follow a C++ method call inside a switch case.
import QtQuick 1.1

Item {
    id: test
    property int step: 0
    property string backupName
    property int hotpRow: -1
    property string hotpCode
    property string editSecret

    function shot(name) { app.screenshot(name); console.log("autotest: " + name) }
    function push(file, props) {
        var c = Qt.createComponent("qrc:/qml/" + file)
        if (c.status != Component.Ready) { console.log("autotest: cannot load " + file + ": " + c.errorString()); return null }
        return window.pageStack.push(c, props || {})
    }
    function pop() { window.pageStack.pop() }
    function check(what, ok) { console.log("autotest: " + (ok ? "ok   " : "FAIL ") + what) }

    function steps() { return [
        // First start after an update from a version without the lock: the settings file
        // holds one plain account (tools\build-desktop.cmd seeds it), and a PIN must be set.
        function() { shot("00-setup"); check("setup asked for", app.state == "setup" && app.hasUnprotectedAccounts) },
        function() {
            check("different PINs refused", !app.setupSecret("2468", "2469", "pin"))
            check("short PIN refused", !app.setupSecret("12", "12", "pin"))
            check("letters in PIN refused", !app.setupSecret("12ab", "12ab", "pin"))
            check("short passphrase refused", !app.setupSecret("abc", "abc", "passphrase"))
            check("PIN set", app.setupSecret("2468", "2468", "pin"))
            check("unlocked", app.state == "unlocked")
            check("old account kept", app.accounts.count == 1 && app.accounts.title(0) == "Legacy")
            app.accounts.remove(0)
        },
        function() { shot("01-empty") },
        function() { push("AddAccountPage.qml") },
        function() { shot("02-add") },
        function() {
            check("bad key refused", !app.addAccount("Bad", "x", "not base32!", "totp", "SHA1", 6, 30, ""))
            pop()
        },
        function() {
            // RFC 6238 SHA1 seed, as sites show it (Base32, grouped, lower case).
            check("typed key added", app.addAccount("Google", "alice@gmail.com", "gezd gnbv gy3t qojq gezd gnbv gy3t qojq", "totp", "SHA1", 6, 30, ""))
            check("link added", app.addFromLink("otpauth://totp/GitHub:bob?secret=JBSWY3DPEHPK3PXP&issuer=GitHub&digits=8&algorithm=SHA256"))
            check("hotp link added", app.addFromLink("otpauth://hotp/Example:carol?secret=GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ&counter=0"))
            check("bad link refused", !app.addFromLink("otpauth://totp/x?issuer=y"))
            check("count 3", app.accounts.count == 3)
        },
        function() {
            shot("03-list")
            check("hotp code 755224", app.accounts.code(2) == "755224")
            app.accounts.nextCode(2)
            check("hotp next 287082", app.accounts.code(2) == "287082")
            console.log("autotest: google code " + app.accounts.code(0) + ", github code " + app.accounts.code(1))
            app.copyCode(0)
        },
        function() { shot("04-copied") },
        function() { push("SettingsPage.qml") },
        function() { shot("05-settings"); app.timeOffset = 30 },
        function() { shot("06-settings-offset"); app.timeOffset = 0; pop() },
        function() { push("AboutPage.qml") },
        function() { shot("07-about"); pop() },
        function() { app.accounts.remove(1); check("count 2 after delete", app.accounts.count == 2) },
        function() { shot("08-after-delete"); push("ShowQrPage.qml", { link: app.accounts.link(0), title: app.accounts.title(0) }) },
        // tools/build-desktop.cmd test decodes this screenshot to check the code on screen.
        function() { shot("09-show-qr"); pop() },
        // The scan page with the fake camera (SA_SCAN_IMAGES): a few frames without a code,
        // then one with; the account is added and the page returns to the list by itself.
        function() { push("ScanPage.qml") },
        function() { shot("10-scan"); window.pageStack.currentPage.openFocusDialog() },
        function() { shot("10b-focus-dialog"); window.pageStack.currentPage.focusDialog.close() },
        function() {},
        function() {},
        function() {},
        function() {
            check("scanned account added", app.accounts.count == 3)
            check("back on the list", window.pageStack.depth == 1)
            shot("11-after-scan")
        },
        // The lock.
        function() { app.lock(); check("locked, list emptied", app.state == "locked" && app.accounts.count == 0) },
        function() { shot("12-unlock"); check("wrong PIN refused", !app.unlock("1111")) },
        function() { check("right PIN opens", app.unlock("2468") && app.accounts.count == 3) },
        function() { push("SettingsPage.qml") },
        function() { var f = window.pageStack.currentPage.flickable; f.contentY = Math.max(0, f.contentHeight - f.height) },
        function() { shot("13-settings-security"); pop(); push("ChangeSecretPage.qml") },
        function() { shot("14-change") },
        function() {
            check("change refused with wrong current PIN", !app.changeSecret("1357", "passphrase1", "passphrase1", "passphrase"))
            check("changed to a passphrase", app.changeSecret("2468", "passphrase1", "passphrase1", "passphrase"))
            check("still unlocked", app.state == "unlocked" && app.accounts.count == 3)
            app.lock()
        },
        function() {
            check("old PIN no longer opens", !app.unlock("2468"))
            check("passphrase opens", app.unlock("passphrase1") && app.accounts.count == 3)
            check("codes intact", app.accounts.code(0) != "")
        },
        // Backup (SA_BACKUP_DIR, emptied by tools\build-desktop.cmd), then restore into a
        // changed list under a different PIN: the missing account comes back, the counter of
        // the HOTP account that moved on since does not go back.
        function() {
            test.backupName = app.backup()
            check("backup written as simpleauth-yyyy-mm-dd-hh-mm.sauth",
                  /^simpleauth-\d{4}-\d\d-\d\d-\d\d-\d\d\.sauth$/.test(test.backupName))
            check("backup listed", app.backupFiles().length == 1 && app.backupFiles()[0] == test.backupName)
            check("backup made under the passphrase", app.backupKind(test.backupName) == "passphrase")
            test.hotpRow = -1
            for (var i = 0; i < app.accounts.count; ++i) if (app.accounts.link(i).indexOf("otpauth://hotp/") == 0) test.hotpRow = i
            app.accounts.nextCode(test.hotpRow)
            test.hotpCode = app.accounts.code(test.hotpRow)
            app.accounts.remove(test.hotpRow == 0 ? 1 : 0)
            check("one account removed", app.accounts.count == 2)
            check("PIN now", app.changeSecret("passphrase1", "9999", "9999", "pin"))
        },
        function() { push("RestorePage.qml") },
        function() { window.pageStack.currentPage.choose(test.backupName) },
        function() { shot("16-restore"); pop() },
        function() {
            check("restore refused with this phone's PIN", !app.restore(test.backupName, "9999"))
            check("restore with the backup's passphrase", app.restore(test.backupName, "passphrase1"))
            check("removed account back", app.accounts.count == 3)
            var row = -1
            for (var i = 0; i < app.accounts.count; ++i) if (app.accounts.link(i).indexOf("otpauth://hotp/") == 0) row = i
            check("HOTP counter kept, not rolled back", app.accounts.code(row) == test.hotpCode)
            check("restoring again adds nothing", app.restore(test.backupName, "passphrase1") && app.accounts.count == 3)
        },
        // Edit: rename through the page's own fields and Save; the key shows and copies but
        // stays as it is, and so does the code.
        function() {
            test.editSecret = app.accounts.secret(0)
            push("EditAccountPage.qml", { row: 0 })
        },
        function() {
            var p = window.pageStack.currentPage
            check("edit page shows the names", p.issuerField.text == app.accounts.issuer(0) && p.nameField.text == app.accounts.name(0))
            shot("18-edit")
            p.showSecret = true
        },
        function() {
            shot("19-edit-secret")
            var p = window.pageStack.currentPage
            p.issuerField.text = "Renamed"
            p.nameField.text = "someone@example.com"
            p.save()
        },
        function() {
            check("renamed", app.accounts.issuer(0) == "Renamed" && app.accounts.name(0) == "someone@example.com")
            check("key unchanged", app.accounts.secret(0) == test.editSecret)
            check("back on the list after save", window.pageStack.depth == 1)
            check("empty names refused", !app.accounts.setNames(0, " ", ""))
            shot("20-renamed")
        },
        function() { push("SettingsPage.qml") },
        function() { var f = window.pageStack.currentPage.flickable; f.contentY = Math.max(0, f.contentHeight - f.height) },
        function() { shot("17-settings-backup"); pop(); app.lock() },
        function() {
            check("restored list survives lock", app.unlock("9999") && app.accounts.count == 3)
            check("new name survives lock", app.accounts.issuer(0) == "Renamed")
            app.lock()
        },
        // Repeated wrong attempts: a wait (the earlier wrong one was reset by the success).
        function() {
            for (var i = 0; i < 5; ++i) app.unlock("wrong")
            check("wait after 5 wrong attempts", app.retrySeconds() > 0)
            check("right one refused while waiting", !app.unlock("9999"))
        },
        function() { shot("15-wait"); Qt.quit() }
    ] }

    Timer {
        interval: 700
        running: true
        repeat: true
        onTriggered: {
            var s = test.steps()
            if (test.step < s.length) s[test.step++]()
        }
    }
}
