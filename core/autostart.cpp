#include "autostart.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>

static const char *EntryName = "MonitorIBP";

static QString launchCommand()
{
    const QString exe = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    return QString("\"%1\" --minimized").arg(exe);
}

#if defined(Q_OS_WIN)

static const char *RunKeyUser = "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static const char *RunKeyAll  = "HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";

bool Autostart::isSupported() { return true; }

bool Autostart::isSetForAllUsers()
{
    QSettings reg(RunKeyAll, QSettings::NativeFormat);
    return reg.contains(EntryName);
}

bool Autostart::isEnabled()
{
    QSettings reg(RunKeyUser, QSettings::NativeFormat);
    return reg.contains(EntryName) || isSetForAllUsers();
}

bool Autostart::setEnabled(bool on)
{
    QSettings reg(RunKeyUser, QSettings::NativeFormat);
    if (on)
        reg.setValue(EntryName, launchCommand());
    else
        reg.remove(EntryName);
    reg.sync();
    return reg.status() == QSettings::NoError;
}

#elif defined(Q_OS_LINUX)

static QString desktopFilePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
           + "/autostart/" + EntryName + ".desktop";
}

bool Autostart::isSupported() { return true; }
bool Autostart::isSetForAllUsers() { return false; }

bool Autostart::isEnabled()
{
    return QFile::exists(desktopFilePath());
}

bool Autostart::setEnabled(bool on)
{
    const QString path = desktopFilePath();
    if (!on)
        return !QFile::exists(path) || QFile::remove(path);

    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    const QString text =
        "[Desktop Entry]\n"
        "Type=Application\n"
        "Name=Мониторинг ИБП\n"
        "Exec=" + launchCommand() + "\n"
                            "Icon=MonitorIBP\n"
                            "Terminal=false\n"
                            "X-GNOME-Autostart-enabled=true\n";
    file.write(text.toUtf8());
    return file.commit();
}

#else

bool Autostart::isSupported() { return false; }
bool Autostart::isEnabled() { return false; }
bool Autostart::isSetForAllUsers() { return false; }
bool Autostart::setEnabled(bool) { return false; }

#endif