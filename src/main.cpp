// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026
//
// This program is free software: you can redistribute it and/or modify it under the terms
// of the GNU General Public License as published by the Free Software Foundation, either
// version 3 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
// PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with this
// program; see the LICENSE file. If not, see <https://www.gnu.org/licenses/>.
#include "appcontroller.h"
#include "accountsmodel.h"
#include "qrcodeitem.h"
#include "qrscanner.h"

#include <QApplication>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QDeclarativeView>
#include <QSettings>
#include <QUrl>
#include <QtDeclarative>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QLatin1String("SimpleAuth"));
    app.setOrganizationName(QLatin1String("SimpleAuth"));

    // Desktop testing: SA_SETTINGS names an .ini file, so a test run never touches (or
    // depends on) the accounts kept in the registry. The phone keeps its private settings.
    QSettings *settings;
    const QByteArray settingsFile = qgetenv("SA_SETTINGS");
    if (!settingsFile.isEmpty())
        settings = new QSettings(QString::fromLocal8Bit(settingsFile), QSettings::IniFormat);
    else
        settings = new QSettings(QLatin1String("SimpleAuth"), QLatin1String("SimpleAuth"));

    qmlRegisterType<AccountsModel>();
    qmlRegisterType<QrCodeItem>("SimpleAuth", 1, 0, "QrCode");
    qmlRegisterType<QrScanner>("SimpleAuth", 1, 0, "QrScanner");

    AppController controller(settings);

    QDeclarativeView view;
    view.setResizeMode(QDeclarativeView::SizeRootObjectToView);
    view.rootContext()->setContextProperty(QLatin1String("app"), &controller);
    // Desktop testing: SA_AUTOTEST names tools/autotest.qml, which main.qml loads to walk the
    // pages by itself. Empty on the phone.
    QString autotest;
#ifndef Q_OS_SYMBIAN
    autotest = QString::fromLocal8Bit(qgetenv("SA_AUTOTEST"));
    if (!autotest.isEmpty()) autotest = QUrl::fromLocalFile(autotest).toString();
#endif
    view.rootContext()->setContextProperty(QLatin1String("saAutotest"), autotest);
    controller.setView(&view);
    view.setSource(QUrl(QLatin1String("qrc:/qml/main.qml")));

#if defined(Q_OS_SYMBIAN) || defined(Q_WS_SIMULATOR)
    view.setAttribute(Qt::WA_LockPortraitOrientation, true);
    view.showFullScreen();
#else
    view.resize(360, 640);
    view.show();
#endif

    const int rc = app.exec();
    delete settings;
    return rc;
}
