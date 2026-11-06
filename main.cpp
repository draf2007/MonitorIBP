#include "mainwindow.h"
#include "thememanager.h"
#include "resources/version.h"

#include <QApplication>
#include <QDebug>
#include <QFontDatabase>
#include <QStyleFactory>
#include <QIcon>
#include <QSystemTrayIcon>
#include <QLocalServer>
#include <QLocalSocket>

static QString instanceKey(){
    return "MonitorIBP-" + qEnvironmentVariable("USERNAME", qEnvironmentVariable("USER"));
}

static bool activateRunningInstance(){
    QLocalSocket socket;
    socket.connectToServer(instanceKey());
    if (!socket.waitForConnected(500))
        return false;
    socket.write("show");
    socket.waitForBytesWritten(500);
    return true;
}

static void setupAppFont()
{
    const int id = QFontDatabase::addApplicationFont(":/resources/fonts/JetBrainsMonoNerdFontPropo-Regular.ttf");
    QFontDatabase::addApplicationFont(":/resources/fonts/JetBrainsMonoNerdFontPropo-Bold.ttf");
    if (id < 0) {
        qDebug() << "Шрифт не загружен, остаётся системный";
        return;
    }
    const QString family = QFontDatabase::applicationFontFamilies(id).constFirst();
    qDebug() << "Шрифт загружен, семейство:" << family;
    const QFont font(family, 9);
    QApplication::setFont(font);

    const char *ownFontClasses[] = { "QMenuBar", "QMenu", "QTipLabel", "QMessageBoxLabel" };
    for (const char *className : ownFontClasses)
        QApplication::setFont(font, className);
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setOrganizationName("Ruselt");
    QApplication::setApplicationName("IBP");
    QApplication::setApplicationVersion(APP_VERSION_STR);
    QGuiApplication::setDesktopFileName("MonitorIBP");
    if (activateRunningInstance())
        return 0;
    QApplication::setWindowIcon(QIcon(":/resources/icons/app.svg"));

    QApplication::setStyle(QStyleFactory::create("Fusion"));
    setupAppFont();

    ThemeManager theme;

    MainWindow w(&theme);

    QLocalServer::removeServer(instanceKey());
    QLocalServer instanceServer;
    instanceServer.listen(instanceKey());
    QObject::connect(&instanceServer, &QLocalServer::newConnection, &w, [&instanceServer, &w]() {
        instanceServer.nextPendingConnection()->deleteLater();
        w.showFromTray();
    });

    const bool startMinimized = QCoreApplication::arguments().contains("--minimized");
    if (!startMinimized)
        w.show();
    else if (!QSystemTrayIcon::isSystemTrayAvailable())
        w.showMinimized();
    return QApplication::exec();
}