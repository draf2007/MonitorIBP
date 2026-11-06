#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QToolButton>
#include <QQuickWidget>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QPointer>
#include "core/comportavailables.h"
#include "core/megatecupsmonitor.h"
#include "core/snmpupsmonitor.h"
#include "ui/logwindow.h"
#include "ui/connectionsettingswindow.h"
#include "core/snmptrapdata.h"
#include "core/snmptraplistener.h"
#include "ui/trapwindow.h"
#include "core/upsshutdownscenario.h"
#include "ui/upsidentwidget.h"
#include "ui/upsratingswidget.h"
#include "ui/thememanager.h"
#include "ui/upsschematicmodel.h"
#include "core/logsettings.h"
#include "core/measurementlog.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(ThemeManager *theme, QWidget *parent = nullptr);
    ~MainWindow() override;
    void showFromTray();
    bool minimizesToTray() const;

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    enum class ConnectionType{None, Com, Snmp};

    UpsSchematicModel *m_schematicModel = nullptr;
    QQuickWidget *m_schematicView = nullptr;
    QLabel *m_connectionStatusLabel;
    QString m_lastConnectedPort;
    QString m_pendingPort;
    ConnectionType m_lastConnectionType = ConnectionType::None;
    bool m_linkLost = false;
    SnmpConnectionParams m_lastSnmpParams;
    ThemeManager *m_theme = nullptr;
    SnmpConnectionParams m_pendingSnmpParams;
    UpsBatteryConfig m_batteryConfig;
    int m_upsBatteryCount = 0;
    UpsNominalSpecifications m_lastRatings;
    bool m_hasRatings = false;
    UpsIdentWidget *m_identWidget = nullptr;
    ConnectionType m_identWidgetType = ConnectionType::None;
    UpsRatingsWidget *m_ratingsWidget = nullptr;
    ConnectionType m_ratingsWidgetType = ConnectionType::None;
    Ui::MainWindow *ui;
    comPortAvailables *listPort;
    MegatecUpsMonitor *upsMonitor;
    SnmpUpsMonitor *snmpMonitor;
    LogWindow *logWindow;
    ConnectionSettingsWindow *connectionSettingsWindow;
    SnmpTrapSettings m_trapSettings;
    LogSettings m_logSettings;
    MeasurementLog *m_measurementLog = nullptr;
    SnmpTrapListener *m_trapListener;
    TrapWindow *m_trapWindow;
    UpsShutdownScenario *m_shutdownScenario;
    QString m_snmpCardType;
    QSystemTrayIcon *m_trayIcon = nullptr;
    QList<QPointer<QWidget>> m_hiddenWindows;
    bool m_quitting = false;
    bool m_trayHintShown = false;
    bool m_notifyModeValid = false;
    UpsSchematicModel::Mode m_notifyMode = UpsSchematicModel::Mode::Mains;
    QStringList m_notifiedAlarms;

    void hideToTray();
    void onComConnectionStatusChanged(bool connected);
    void onLinkStatusChanged(bool alive);
    void onSnmpConnectionStatusChanged(bool connected);
    void updateConnectionStatusBar(bool connected);
    void saveSnmpSettings(const SnmpConnectionParams &params);
    void onTrapSettingsTriggered();
    void onLogSettingsTriggered();
    void onTrapWindowTriggered();
    void startTrapListening();
    bool tryStartTrapListening();
    void autoStartTrapListening();
    void stopTrapListening();
    void onSnmpCardTypeTriggered();
    void onUpsParamsTriggered();
    UpsBatteryConfig effectiveBatteryConfig() const;
    void showRatings();
    UpsIdentWidget *identWidgetFor(ConnectionType type);
    UpsRatingsWidget *ratingsWidgetFor(ConnectionType type);
    void sendUpsCommand(UpsCommand command, const UpsShutdownParams &params = UpsShutdownParams());
    void onUpsStatusUpdated(const UpsStatus &upsStatus);
    QString settingsFilePath() const;
    void setupThemeMenu();
    void setupIconSetMenu(QMenu *themeMenu);
    void updateToolbarIcons();
    void setupTray();

    void quitApplication();
    void updateTrayIcon();
    void notify(const QString &text, QSystemTrayIcon::MessageIcon icon);
    void notifyModeChange();
    void resetNotifications();
    void updateSchematicAnimation();
    void setupAutostartAction();

    void quickConnect(bool automatic);
    void onConnectFailed(const QString &reason);
    bool m_autoConnecting = false;
};
#endif
