#include <QObject>
#include <QDebug>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QMessageBox>
#include <QCloseEvent>
#include <QActionGroup>
#include <QMenu>
#include <QLabel>
#include <QStyle>
#include <QQmlContext>
#include <QVBoxLayout>
#include <QApplication>
#include <QTimer>
#include <QShowEvent>
#include <QHideEvent>
#include <QSignalBlocker>
#include <QStatusBar>
#include "core/autostart.h"
#include "ui/mainwindow.h"
#include "ui_mainwindow.h"
#include "ui/trapsettingswindow.h"
#include "ui/logsettingswindow.h"
#include "core/dailylogfile.h"
#include "core/comportavailables.h"
#include "core/snmpupsmonitor.h"
#include "core/snmptraplistener.h"
#include "ui/snmpcardtypewindow.h"
#include "ui/upsparamswindow.h"
#include "ui/snmpidentwidget.h"
#include "ui/comidentwidget.h"
#include "core/snmpcardregistry.h"
#include "ui/snmpratingswidget.h"
#include "ui/comratingswidget.h"

static QString obfuscatePassword(const QString &plainTexet){
    if (plainTexet.isEmpty()) {
        return QString();
    }

    const QByteArray key = "IBP_monitor_salt";
    QByteArray data = plainTexet.toUtf8();

    for (int i = 0; i < data.size(); ++i) {
        data[i] = data[i]^key[i % key.size()];
    }
    return QString::fromLatin1(data.toBase64());

}

static QString deobfuscatePassword(const QString &obfuscatedTexet){
    if (obfuscatedTexet.isEmpty()) {
        return QString();
    }

    const QByteArray key = "IBP_monitor_salt";
    QByteArray data = QByteArray::fromBase64(obfuscatedTexet.toLatin1());

    for (int i = 0; i < data.size(); ++i) {
        data[i] = data[i]^key[i % key.size()];
    }
    return QString::fromUtf8(data);
}

static QString snmpVersionToString(SnmpVersion version){
    switch (version) {
    case SnmpVersion::V1: return "v1";
    case SnmpVersion::V2c: return "v2c";
    case SnmpVersion::V3: return "v3";
    }
    return "v1";
}

static SnmpVersion snmpVersionFromString(const QString &text){
    if (text == "v2c") return SnmpVersion::V2c;
    if (text == "v3") return SnmpVersion::V3;

    return SnmpVersion::V1;
}

static QString snmpSecurityLevelToString(SnmpSecurityLevel level){
    switch (level) {
    case SnmpSecurityLevel::NoAuthNoPriv: return "noAuthNoPriv";
    case SnmpSecurityLevel::AuthNoPriv: return "authNoPriv";
    case SnmpSecurityLevel::AuthPriv: return "authPriv";
    }
    return "v1";
}

static SnmpSecurityLevel snmpSecurityLevelFromString(QString level){
    if (level == "authNoPriv") return SnmpSecurityLevel::AuthNoPriv;
    if (level == "authPriv") return SnmpSecurityLevel::AuthPriv;

    return SnmpSecurityLevel::NoAuthNoPriv;
}

static QString snmpAuthProtocolToString(SnmpAuthProtocol protocol){
    return (protocol == SnmpAuthProtocol::SHA) ? "sha" : "md5";
}

static SnmpAuthProtocol snmpAuthProtocolFromString(const QString &text){
    return (text == "sha") ? SnmpAuthProtocol::SHA : SnmpAuthProtocol::MD5;
}

static QString snmpPrivProtocolToString(SnmpPrivProtocol protocol){
    return (protocol == SnmpPrivProtocol::AES) ? "aes" : "des";
}

static SnmpPrivProtocol snmpPrivProtocolFromString(const QString &text){
    return (text == "aes") ? SnmpPrivProtocol::AES : SnmpPrivProtocol::DES;
}

MainWindow::MainWindow(ThemeManager *theme, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_theme = theme;
    m_schematicModel = new UpsSchematicModel(this);

    m_schematicView = new QQuickWidget(ui->schematicGroupBox);
    m_schematicView->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_schematicView->rootContext()->setContextProperty("schematic", m_schematicModel);
    m_schematicView->rootContext()->setContextProperty("theme", m_theme);
    m_schematicView->setSource(QUrl("qrc:/resources/qml/UpsSchematic.qml"));

    QLayout *schematicLayout = ui->schematicGroupBox->layout();
    if (!schematicLayout)
        schematicLayout = new QVBoxLayout(ui->schematicGroupBox);
    schematicLayout->addWidget(m_schematicView);

    setupThemeMenu();
    updateToolbarIcons();
    for (QAction *action : {ui->actionQuickConnect, ui->actionQuickDisconnect}) {
        if (auto *button = qobject_cast<QToolButton *>(ui->toolBar->widgetForAction(action)))
            button->setToolButtonStyle(Qt::ToolButtonIconOnly);
    }
    connect(m_theme, &ThemeManager::themeChanged, this, &MainWindow::updateToolbarIcons);

    setupTray();

    m_connectionStatusLabel = new QLabel(this);
    m_connectionStatusLabel->setObjectName("connectionStatus");
    ui->statusbar->addPermanentWidget(m_connectionStatusLabel);

    QSettings settings(settingsFilePath(),QSettings::IniFormat);
    QString lastType =settings.value("connection/lastType").toString();
    m_lastConnectedPort = settings.value("connection/lastPort").toString();
    m_lastSnmpParams.ip = settings.value("connection/snmpIp").toString();
    m_lastSnmpParams.port = static_cast<quint16>(settings.value("connection/snmpPort").toUInt());
    m_lastSnmpParams.community = settings.value("connection/snmpCommunity").toString();
    m_lastSnmpParams.writeCommunity = settings.value("connection/snmpWriteCommunity").toString();
    m_lastSnmpParams.version = snmpVersionFromString(settings.value("connection/snmpVersion").toString());
    m_lastSnmpParams.username = settings.value("connection/snmpUsername").toString();
    m_lastSnmpParams.securityLevel = snmpSecurityLevelFromString(settings.value("connection/snmpSecurityLevel").toString());
    m_lastSnmpParams.authProtocol = snmpAuthProtocolFromString(settings.value("connection/snmpAuthProtocol").toString());
    m_lastSnmpParams.authPassword = deobfuscatePassword(settings.value("connection/snmpAuthPassword").toString());
    m_lastSnmpParams.privProtocol = snmpPrivProtocolFromString(settings.value("connection/snmpPrivProtocol").toString());
    m_lastSnmpParams.privPassword = deobfuscatePassword(settings.value("connection/snmpPrivPassword").toString());

    m_trapSettings.port = static_cast<quint16>(settings.value("trap/port", 162).toUInt());
    m_trapSettings.community = settings.value("trap/community").toString();
    m_trapSettings.allowedIp = settings.value("trap/allowedIp").toString();
    m_trapSettings.autoStart = settings.value("trap/autoStart", false).toBool();
    setupAutostartAction();

    m_logSettings.load(settings);

    m_snmpCardType = settings.value("snmp/cardType").toString();
    m_batteryConfig.cellsPerBattery = settings.value("ups/cellsPerBattery", 6).toInt();
    m_batteryConfig.batteryCount = settings.value("ups/batteryCount", 0).toInt();
    m_schematicModel->setBatteryConfig(m_batteryConfig);

    if (lastType == "snmp") {
        m_lastConnectionType = ConnectionType::Snmp;
    }else if (lastType == "com" || !m_lastConnectedPort.isEmpty()) {
        m_lastConnectionType = ConnectionType::Com;
    }

    updateConnectionStatusBar(false);

    listPort = new comPortAvailables(this);
    upsMonitor = new MegatecUpsMonitor(this);
    snmpMonitor = new SnmpUpsMonitor(this);
    snmpMonitor->setCardType(m_snmpCardType);
    logWindow = new LogWindow(this);

    connectionSettingsWindow = new ConnectionSettingsWindow(this);
    connectionSettingsWindow->setSnmpParams(m_lastSnmpParams);
    connectionSettingsWindow->setLastComPortName(m_lastConnectedPort);

    QObject::connect(ui->Log, &QAction::triggered,[this](){
        logWindow->show();
        logWindow->raise();
        logWindow->activateWindow();
    });

    QObject::connect(ui->Connect, &QAction::triggered,[this](){
        connectionSettingsWindow->show();
        connectionSettingsWindow->raise();
        connectionSettingsWindow->activateWindow();
    });

    QObject::connect(listPort, &comPortAvailables::portListSend, connectionSettingsWindow, &ConnectionSettingsWindow::updatePortList);
    QObject::connect(connectionSettingsWindow, &ConnectionSettingsWindow::refreshRequested, listPort, &comPortAvailables::availablesPort);

    QObject::connect(connectionSettingsWindow, &ConnectionSettingsWindow::connectRequested, this, [this](const QString &portName){
        m_autoConnecting = false;
        m_pendingPort = portName;
        logWindow->setPaused(false);

        if (snmpMonitor->isConnected()) {
            snmpMonitor->disconnectFromHost();
        }
        upsMonitor->connectToPort(portName);
    });

    QObject::connect(connectionSettingsWindow, &ConnectionSettingsWindow::snmpConnectRequested, this, [this](const SnmpConnectionParams &params){
        m_autoConnecting = false;
        m_pendingSnmpParams = params;
        logWindow->setPaused(false);

        saveSnmpSettings(params);
        if (upsMonitor->isConnected()) {
            upsMonitor->disconnectFromPort();
        }
        snmpMonitor->connectToHost(params);
    });

    QObject::connect(upsMonitor, &MegatecUpsMonitor::connectionStatusChanged, this, &MainWindow::onComConnectionStatusChanged);
    QObject::connect(snmpMonitor, &SnmpUpsMonitor::connectionStatusChanged, this, &MainWindow::onSnmpConnectionStatusChanged);

    QObject::connect(upsMonitor, &MegatecUpsMonitor::linkStatusChanged, this, &MainWindow::onLinkStatusChanged);
    QObject::connect(snmpMonitor, &SnmpUpsMonitor::linkStatusChanged, this, &MainWindow::onLinkStatusChanged);

    QObject::connect(ui->actionQuickDisconnect, &QAction::triggered, this, [this](){
        if (upsMonitor->isConnected()) {
            upsMonitor->disconnectFromPort();
        }
        if (snmpMonitor->isConnected()) {
            snmpMonitor->disconnectFromHost();
        }
        logWindow->setPaused(true);
    });

    QObject::connect(ui->actionQuickConnect, &QAction::triggered, this, [this](){
        quickConnect(false);
    });
    QObject::connect(upsMonitor, &MegatecUpsMonitor::connectFailed, this, &MainWindow::onConnectFailed);
    QObject::connect(snmpMonitor, &SnmpUpsMonitor::connectFailed, this, &MainWindow::onConnectFailed);

    QObject::connect(upsMonitor, &MegatecUpsMonitor::logMessage, logWindow, &LogWindow::appendLine);
    QObject::connect(upsMonitor, &MegatecUpsMonitor::errorOccurred, logWindow, &LogWindow::appendLine);
    QObject::connect(snmpMonitor, &SnmpUpsMonitor::logMessage, logWindow, &LogWindow::appendLine);
    QObject::connect(snmpMonitor, &SnmpUpsMonitor::errorOccurred, logWindow, &LogWindow::appendLine);

    QObject::connect(upsMonitor, &MegatecUpsMonitor::identificationReceived, this, [this](const UpsIdentification &info) {
        identWidgetFor(ConnectionType::Com)->setIdentification(info);
    });
    QObject::connect(snmpMonitor, &SnmpUpsMonitor::identificationReceived, this, [this](const UpsIdentification &info) {
        identWidgetFor(ConnectionType::Snmp)->setIdentification(info);
    });

    QObject::connect(upsMonitor, &MegatecUpsMonitor::ratingsReceived, this, [this](const UpsNominalSpecifications &ratings) {
        m_lastRatings = ratings;
        m_hasRatings = true;
        ratingsWidgetFor(ConnectionType::Com);
        showRatings();
    });
    QObject::connect(snmpMonitor, &SnmpUpsMonitor::ratingsReceived, this, [this](const UpsNominalSpecifications &ratings) {
        m_lastRatings = ratings;
        m_hasRatings = true;
        ratingsWidgetFor(ConnectionType::Snmp);
        showRatings();
    });

    ui->ptrtStatus->setVisible(false);
    QObject::connect(upsMonitor, &MegatecUpsMonitor::protocolDetected, this, [this](bool extended) {
        static_cast<ComIdentWidget *>(identWidgetFor(ConnectionType::Com))->setProtocol(extended);
        ui->ptrtStatus->clear();
        ui->ptrtStatus->setVisible(extended);
    });
    QObject::connect(upsMonitor, &MegatecUpsMonitor::ptrtStatusUpdated, ui->ptrtStatus, &PtrtStatusWidget::setStatus);

    QObject::connect(upsMonitor, &MegatecUpsMonitor::ptrtInfoReceived, this, [this](const UpsPtrtInfo &info) {
        static_cast<ComIdentWidget *>(identWidgetFor(ConnectionType::Com))->setPtrtInfo(info);
        m_upsBatteryCount = qMax(0, info.batteryCount);
        m_schematicModel->setBatteryConfig(effectiveBatteryConfig());
        showRatings();
    });

    QObject::connect(upsMonitor, &MegatecUpsMonitor::batteryLevelUpdated, m_schematicModel, &UpsSchematicModel::setBatteryLevel);
    QObject::connect(snmpMonitor, &SnmpUpsMonitor::batteryLevelUpdated, m_schematicModel, &UpsSchematicModel::setBatteryLevel);

    m_shutdownScenario = new UpsShutdownScenario(this);
    QObject::connect(m_shutdownScenario, &UpsShutdownScenario::commandRequested, this, &MainWindow::sendUpsCommand);
    QObject::connect(m_shutdownScenario, &UpsShutdownScenario::logMessage, logWindow, &LogWindow::appendLine);

    QObject::connect(upsMonitor, &MegatecUpsMonitor::statusUpdated, this, &MainWindow::onUpsStatusUpdated);
    QObject::connect(snmpMonitor, &SnmpUpsMonitor::statusUpdated, this, &MainWindow::onUpsStatusUpdated);

    ui->controlPanel->setScenario(m_shutdownScenario);
    ui->controlPanel->setDelays(settings.value("ups/shutdownDelay", 30).toInt(),
                                settings.value("ups/restartDelay", 30).toInt());

    ui->controlPanel->setDelays(settings.value("ups/shutdownDelay", 30).toInt(),
                                settings.value("ups/restartDelay", 30).toInt());

    const bool logBeeper = settings.value("log/beeper", true).toBool();
    ui->controlPanel->setBeeperLogEnabled(logBeeper);
    upsMonitor->setBeeperLogEnabled(logBeeper);
    snmpMonitor->setBeeperLogEnabled(logBeeper);
    QObject::connect(ui->controlPanel, &UpsControlPanel::beeperLogToggled, this, [this](bool enabled) {
        upsMonitor->setBeeperLogEnabled(enabled);
        snmpMonitor->setBeeperLogEnabled(enabled);
        QSettings settings(settingsFilePath(), QSettings::IniFormat);
        settings.setValue("log/beeper", enabled);
    });

    QObject::connect(ui->controlPanel, &UpsControlPanel::commandRequested, this, &MainWindow::sendUpsCommand);
    QObject::connect(ui->controlPanel, &UpsControlPanel::delaysChanged, this, [this](int shutdownDelaySec, int restartDelaySec) {
        QSettings settings(settingsFilePath(), QSettings::IniFormat);
        settings.setValue("ups/shutdownDelay", shutdownDelaySec);
        settings.setValue("ups/restartDelay", restartDelaySec);
    });

    QObject::connect(ui->actionTrapSettings, &QAction::triggered, this, &MainWindow::onTrapSettingsTriggered);
    QObject::connect(ui->actionLogSettings, &QAction::triggered, this, &MainWindow::onLogSettingsTriggered);

    QObject::connect(ui->actionSnmpCardType, &QAction::triggered, this, &MainWindow::onSnmpCardTypeTriggered);

    QObject::connect(ui->actionUpsParams, &QAction::triggered, this, &MainWindow::onUpsParamsTriggered);

    m_trapListener = new SnmpTrapListener(this);
    m_trapWindow = new TrapWindow(this);

    QObject::connect(m_trapListener, &SnmpTrapListener::logMessage, logWindow, &LogWindow::appendTrapLine);
    QObject::connect(m_trapListener, &SnmpTrapListener::errorOccurred, logWindow, &LogWindow::appendTrapLine);
    QObject::connect(m_trapListener, &SnmpTrapListener::trapReceived, m_trapWindow, &TrapWindow::addTrap);

    QObject::connect(ui->actionTrapWindow, &QAction::triggered, this, &MainWindow::onTrapWindowTriggered);

    QObject::connect(m_trapWindow, &TrapWindow::startRequested, this, &MainWindow::startTrapListening);
    QObject::connect(m_trapWindow, &TrapWindow::stopRequested, this, &MainWindow::stopTrapListening);

    if (m_logSettings.eventsAuto)
        logWindow->setLoggingEnabled(true);

    m_measurementLog = new MeasurementLog(this);
    QObject::connect(logWindow, &LogWindow::measurementLoggingToggled, m_measurementLog, &MeasurementLog::setRecording);
    QObject::connect(m_schematicModel, &UpsSchematicModel::modeChanged, m_measurementLog, &MeasurementLog::writeNow);
    logWindow->setMeasurementColumns(MeasurementLog::columnNames());
    QObject::connect(m_measurementLog, &MeasurementLog::rowWritten, logWindow, &LogWindow::appendMeasurementRow);
    if (m_logSettings.dataAuto)
        logWindow->setMeasurementLoggingEnabled(true);

    DailyLogFile::removeOlderThan(m_logSettings.keepDays);
    auto *logCleanupTimer = new QTimer(this);
    QObject::connect(logCleanupTimer, &QTimer::timeout, this, [this](){
        DailyLogFile::removeOlderThan(m_logSettings.keepDays);
    });
    logCleanupTimer->start(60 * 60 * 1000);

    QSettings startSettings(settingsFilePath(), QSettings::IniFormat);
    ui->actionAutoConnect->setChecked(startSettings.value("connection/autoConnect", true).toBool());
    connect(ui->actionAutoConnect, &QAction::toggled, this, [this](bool on) {
        QSettings settings(settingsFilePath(), QSettings::IniFormat);
        settings.setValue("connection/autoConnect", on);
    });

    if (ui->actionAutoConnect->isChecked()) {
        QTimer::singleShot(0, this, [this]() {
            const bool haveCom  = m_lastConnectionType == ConnectionType::Com  && !m_lastConnectedPort.isEmpty();
            const bool haveSnmp = m_lastConnectionType == ConnectionType::Snmp && !m_lastSnmpParams.ip.isEmpty();
            if (haveCom || haveSnmp)
                quickConnect(true);
        });
    }

    if (m_trapSettings.autoStart)
        QTimer::singleShot(0, this, &MainWindow::autoStartTrapListening);
}

MainWindow::~MainWindow()
{
    snmpMonitor->disconnectFromHost();
    delete ui;
}

void MainWindow::setupTray(){
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::quitApplication);
    connect(m_schematicModel, &UpsSchematicModel::modeChanged, this, &MainWindow::notifyModeChange);

    QSettings settings(settingsFilePath(), QSettings::IniFormat);
    ui->actionNotifications->setChecked(settings.value("ui/notifications", true).toBool());
    connect(ui->actionNotifications, &QAction::toggled, this, [this](bool on) {
        QSettings settings(settingsFilePath(), QSettings::IniFormat);
        settings.setValue("ui/notifications", on);
    });

    ui->actionMinimizeToTray->setChecked(settings.value("ui/minimizeToTray", true).toBool());
    connect(ui->actionMinimizeToTray, &QAction::toggled, this, [this](bool on) {
        QSettings settings(settingsFilePath(), QSettings::IniFormat);
        settings.setValue("ui/minimizeToTray", on);
    });

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        qDebug() << "Системный трей недоступен — крестик закрывает программу";
        ui->actionNotifications->setEnabled(false);
        ui->actionMinimizeToTray->setEnabled(false);
        return;
    }

    auto *trayMenu = new QMenu(this);
    QAction *openAction = trayMenu->addAction("Открыть");
    trayMenu->addSeparator();
    trayMenu->addAction(ui->actionExit);
    connect(openAction, &QAction::triggered, this, &MainWindow::showFromTray);

    m_trayIcon = new QSystemTrayIcon(QApplication::windowIcon(), this);
    m_trayIcon->setToolTip("Мониторинг ИБП");
    m_trayIcon->setContextMenu(trayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
                if (reason == QSystemTrayIcon::DoubleClick)
                    showFromTray();
            });

    connect(m_schematicModel, &UpsSchematicModel::modeChanged, this, &MainWindow::updateTrayIcon);
    updateTrayIcon();

    m_trayIcon->show();

    QApplication::setQuitOnLastWindowClosed(false);
}

bool MainWindow::minimizesToTray() const{
    return m_trayIcon && ui->actionMinimizeToTray->isChecked();
}

void MainWindow::hideToTray(){
    m_hiddenWindows.clear();
    const QList<QWidget *> windows = findChildren<QWidget *>();
    for (QWidget *w : windows) {
        if (w->isWindow() && w->isVisible()){
            m_hiddenWindows << w;
            w->hide();
        }
    }
    hide();
}

void MainWindow::showFromTray()
{
    showNormal();

    for (const QPointer<QWidget> &w : std::as_const(m_hiddenWindows)) {
        if (w)
            w->show();
    }
    m_hiddenWindows.clear();

    raise();
    activateWindow();
}

void MainWindow::quitApplication(){
    if (QWidget *modal = QApplication::activeModalWidget()) {
        showFromTray();
        modal->raise();
        modal->activateWindow();
        return;
    }

    m_quitting = true;
    if (close()) {
        QApplication::quit();
    } else {
        m_quitting = false;
    }
}

void MainWindow::closeEvent(QCloseEvent *event){
    if (!m_quitting && minimizesToTray()) {
        hideToTray();
        event->ignore();
        if (!m_trayHintShown) {
            m_trayIcon->showMessage("Мониторинг ИБП",
                                    "Программа продолжает работать в трее",
                                    QSystemTrayIcon::Information, 5000);
            m_trayHintShown = true;
        }
        return;
    }

    if (!m_quitting) {
        event->ignore();
        QTimer::singleShot(0, this, &MainWindow::quitApplication);
        return;
    }

    if (!m_trapWindow->confirmClose()) {
        event->ignore();
        return;
    }
    event->accept();
}

void MainWindow::onComConnectionStatusChanged(bool connected){
    resetNotifications();

    if (connected)
        m_measurementLog->startSession("COM", m_logSettings.comIntervalSec);
    else
        m_measurementLog->stopSession();

    m_schematicModel->setConnected(connected);

    connectionSettingsWindow->onConnectionStatusChanged(connected);
    m_linkLost = false;
    m_schematicModel->setLinkLost(false);

    if (connected) {
        m_upsBatteryCount = 0;
        m_schematicModel->setBatteryConfig(effectiveBatteryConfig());
        m_lastConnectionType = ConnectionType::Com;
        m_lastConnectedPort = m_pendingPort;
        QSettings settings(settingsFilePath(), QSettings::IniFormat);
        settings.setValue("connection/lastType", "com");
        settings.setValue("connection/lastPort", m_lastConnectedPort);
    } else {
        m_shutdownScenario->cancel("нет связи с ИБП по COM");
        ui->ptrtStatus->setVisible(false);
    }
    updateConnectionStatusBar(connected);
    ui->controlPanel->setConnected(connected);
    ui->controlPanel->setTimedTestAvailable(true);
}

void MainWindow::onSnmpConnectionStatusChanged(bool connected){
    resetNotifications();
    if (connected)
        m_measurementLog->startSession("SNMP", m_logSettings.snmpIntervalSec);
    else
        m_measurementLog->stopSession();

    m_schematicModel->setConnected(connected);

    connectionSettingsWindow->onConnectionStatusChanged(connected);
    m_linkLost = false;
    m_schematicModel->setLinkLost(false);

    if (connected) {
        m_upsBatteryCount = 0;
        m_schematicModel->setBatteryConfig(effectiveBatteryConfig());
        m_lastConnectionType = ConnectionType::Snmp;
        m_lastSnmpParams = m_pendingSnmpParams;
        QSettings settings(settingsFilePath(), QSettings::IniFormat);
        settings.setValue("connection/lastType", "snmp");
    } else { m_shutdownScenario->cancel("нет связи с картой"); }
    updateConnectionStatusBar(connected);
    ui->controlPanel->setConnected(connected);
    ui->controlPanel->setTimedTestAvailable(false);
}

void MainWindow::saveSnmpSettings(const SnmpConnectionParams &params){
    QSettings settings(settingsFilePath(),QSettings::IniFormat);
    settings.setValue("connection/snmpIp", params.ip);
    settings.setValue("connection/snmpPort", params.port);
    settings.setValue("connection/snmpCommunity", params.community);
    settings.setValue("connection/snmpWriteCommunity", params.writeCommunity);
    settings.setValue("connection/snmpVersion", snmpVersionToString(params.version));
    settings.setValue("connection/snmpUsername", params.username);
    settings.setValue("connection/snmpSecurityLevel", snmpSecurityLevelToString(params.securityLevel));
    settings.setValue("connection/snmpAuthProtocol", snmpAuthProtocolToString(params.authProtocol));
    settings.setValue("connection/snmpAuthPassword", obfuscatePassword(params.authPassword));
    settings.setValue("connection/snmpPrivProtocol", snmpPrivProtocolToString(params.privProtocol));
    settings.setValue("connection/snmpPrivPassword", obfuscatePassword(params.privPassword));
}

void MainWindow::onLinkStatusChanged(bool alive){
    m_linkLost = !alive;

    m_measurementLog->setLinkAlive(alive);
    m_schematicModel->setLinkLost(!alive);
    ui->controlPanel->setConnected(alive);
    updateConnectionStatusBar(true);

    if (alive)
        notify("Связь с ИБП восстановлена", QSystemTrayIcon::Information);
    else
        notify("Нет связи с ИБП", QSystemTrayIcon::Warning);
}

void MainWindow::updateConnectionStatusBar(bool connected){
    ui->actionQuickConnect->setEnabled(!connected);
    ui->actionQuickDisconnect->setEnabled(connected);

    QString state = !connected ? "отключено " : (m_linkLost ? "нет связи с ИБП " : "подключено ");
    m_connectionStatusLabel->setProperty("state", connected && m_linkLost ? "alarm" : "");
    m_connectionStatusLabel->style()->unpolish(m_connectionStatusLabel);
    m_connectionStatusLabel->style()->polish(m_connectionStatusLabel);
    QString text;

    if (m_lastConnectionType == ConnectionType::Snmp) {
        QString versionText;
        switch (m_lastSnmpParams.version) {
        case SnmpVersion::V1:
            versionText = "v1";
            break;
        case SnmpVersion::V2c:
            versionText = "v2c";
            break;
        case SnmpVersion::V3:
            versionText = "v3";
            break;
        default:
            versionText = "?";
            break;
        }
        text = QString("SNMP %1 %2:%3::%4 %5").arg(versionText,m_lastSnmpParams.ip
                                                   ,QString::number(m_lastSnmpParams.port)
                                                   ,m_lastSnmpParams.community
                                                   ,state);
    }else if (m_lastConnectionType == ConnectionType::Com) {
        text = QString("%1 %2").arg(m_lastConnectedPort, state);
    } else {
        text = QString("Подключение не настроено");
    }

    m_connectionStatusLabel->setText(text);
}

QString MainWindow::settingsFilePath() const{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);

    return configDir + "/settings.ini";
}

void MainWindow::onTrapSettingsTriggered(){
    TrapSettingsWindow dialog(this);
    dialog.setSettings(m_trapSettings);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    m_trapSettings = dialog.settings();
    QSettings settings(settingsFilePath(),QSettings::IniFormat);
    settings.setValue("trap/port", m_trapSettings.port);
    settings.setValue("trap/community", m_trapSettings.community);
    settings.setValue("trap/allowedIp", m_trapSettings.allowedIp);
    settings.setValue("trap/autoStart", m_trapSettings.autoStart);

    if (m_trapListener->isRunning()) {
        bool ok = m_trapListener->start(m_trapSettings);
        m_trapWindow->setListening(ok, m_trapSettings.port);
    }
}

void MainWindow::onLogSettingsTriggered(){
    LogSettingsWindow dialog(this);
    dialog.setSettings(m_logSettings);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    m_logSettings = dialog.settings();
    QSettings settings(settingsFilePath(), QSettings::IniFormat);
    m_logSettings.save(settings);
    DailyLogFile::removeOlderThan(m_logSettings.keepDays);

    if (m_measurementLog->inSession())
        m_measurementLog->setIntervalSec(m_measurementLog->link() == "COM" ? m_logSettings.comIntervalSec : m_logSettings.snmpIntervalSec);
}

void MainWindow::onTrapWindowTriggered(){
    m_trapWindow->show();
    m_trapWindow->raise();
    m_trapWindow->activateWindow();
}

bool MainWindow::tryStartTrapListening(){
    if (m_trapListener->isRunning()) {
        return true;
    }
    if (!m_trapListener->start(m_trapSettings)) {
        return false;
    }
    m_trapWindow->setListening(true, m_trapSettings.port);
    return true;
}

void MainWindow::startTrapListening(){
    if (!tryStartTrapListening()) {
        QMessageBox::warning(m_trapWindow, "Приём трапов",
                             QString("Не удалось открыть UDP-порт %1.\n"
                                     "Возможно, он занят другой программой или службой SNMP Trap.")
                                 .arg(m_trapSettings.port));
    }
}

void MainWindow::autoStartTrapListening(){
    if (tryStartTrapListening()) {
        return;
    }
    const QString text = QString("Не удалось запустить приём трапов: UDP-порт %1 занят").arg(m_trapSettings.port);
    logWindow->appendTrapLine(text);
    notify(text, QSystemTrayIcon::Warning);
}

void MainWindow::stopTrapListening(){
    m_trapListener->stop();
    m_trapWindow->setListening(false, m_trapSettings.port);
}

void MainWindow::onSnmpCardTypeTriggered(){
    SnmpCardTypeWindow dialog(this);
    dialog.setCardType(m_snmpCardType);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    m_snmpCardType = dialog.cardType();
    snmpMonitor->setCardType(m_snmpCardType);

    QSettings settings(settingsFilePath(), QSettings::IniFormat);
    settings.setValue("snmp/cardType", m_snmpCardType);
}

void MainWindow::showRatings(){
    if (!m_hasRatings || !m_ratingsWidget) {
        return;
    }
    m_ratingsWidget->setRatings(m_lastRatings, effectiveBatteryConfig());
}

UpsBatteryConfig MainWindow::effectiveBatteryConfig() const{
    UpsBatteryConfig config = m_batteryConfig;
    if (m_upsBatteryCount > 0) {
        config.batteryCount = m_upsBatteryCount;
        config.countFromUps = true;
    }
    return config;
}

void MainWindow::sendUpsCommand(UpsCommand command, const UpsShutdownParams &params){
    if (upsMonitor->isConnected()) {
        upsMonitor->sendControlCommand(command, params);
    } else {
        snmpMonitor->sendControlCommand(command, params);
    }
}

void MainWindow::onUpsStatusUpdated(const UpsStatus &upsStatus){
    m_measurementLog->setStatus(upsStatus, effectiveBatteryConfig().totalCells());
    m_shutdownScenario->onStatusUpdated(upsStatus);

    ui->controlPanel->setStatus(upsStatus);

    UpsStatus status = upsStatus;
    if (m_shutdownScenario->state() == UpsShutdownScenario::State::WaitingShutdown) {
        status.shutdownCountdownSec = m_shutdownScenario->secondsLeft();
    } else if (m_shutdownScenario->state() == UpsShutdownScenario::State::WaitingStartup) {
        status.startupCountdownSec = m_shutdownScenario->secondsLeft();
    } else if (m_shutdownScenario->state() == UpsShutdownScenario::State::ShuttingDown) {
        status.shutdownActive = true;
    }
    m_schematicModel->setStatus(status);

    static const QStringList modeAlarms = {
                                            "работа от батареи", "низкий заряд батареи", "байпас", "идёт тест",
                                            "выход выключен", "выход выключен по команде", "выключение по команде", "ИБП выключен"};
    QStringList newAlarms;
    for (const QString &alarm : upsStatus.alarms) {
        if (!m_notifiedAlarms.contains(alarm) && !modeAlarms.contains(alarm))
            newAlarms << alarm;
    }
    m_notifiedAlarms = upsStatus.alarms;
    if (!newAlarms.isEmpty())
        notify("Авария: " + newAlarms.join(", "), QSystemTrayIcon::Critical);
}

UpsRatingsWidget *MainWindow::ratingsWidgetFor(ConnectionType type){
    if (m_ratingsWidget && m_ratingsWidgetType == type) {
        return m_ratingsWidget;
    }

    delete m_ratingsWidget;

    if (type == ConnectionType::Snmp) {
        m_ratingsWidget = new SnmpRatingsWidget(ui->ratingsGroupBox);
    } else {
        m_ratingsWidget = new ComRatingsWidget(ui->ratingsGroupBox);
    }
    m_ratingsWidgetType = type;

    ui->ratingsGroupBox->layout()->addWidget(m_ratingsWidget);
    return m_ratingsWidget;
}

UpsIdentWidget *MainWindow::identWidgetFor(ConnectionType type){
    if (m_identWidget && m_identWidgetType == type) {
        return m_identWidget;
    }

    delete m_identWidget;

    if (type == ConnectionType::Snmp) {
        auto *snmpWidget = new SnmpIdentWidget(ui->identificationGroupBox);
        snmpWidget->setCardName(findSnmpCard(m_snmpCardType).name);
        m_identWidget = snmpWidget;
    } else {
        auto *comWidget = new ComIdentWidget(ui->identificationGroupBox);
        comWidget->setPortName(m_pendingPort);
        m_identWidget = comWidget;
    }
    m_identWidgetType = type;

    ui->identificationGroupBox->layout()->addWidget(m_identWidget);
    return m_identWidget;
}

void MainWindow::onUpsParamsTriggered(){
    UpsParamsWindow dialog(this);
    dialog.setBatteryConfig(m_batteryConfig);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    m_batteryConfig = dialog.batteryConfig();
    m_schematicModel->setBatteryConfig(effectiveBatteryConfig());
    showRatings();

    QSettings settings(settingsFilePath(), QSettings::IniFormat);
    settings.setValue("ups/cellsPerBattery", m_batteryConfig.cellsPerBattery);
    settings.setValue("ups/batteryCount", m_batteryConfig.batteryCount);
}

void MainWindow::setupThemeMenu()
{

    struct ThemeItem {
        const char *text;
        ThemeManager::Mode mode;
        const char *key;
    };
    const ThemeItem items[] = {
                                {"Системная", ThemeManager::Mode::System, "system"},
                                {"Светлая",   ThemeManager::Mode::Light,  "light"},
                                {"Тёмная",    ThemeManager::Mode::Dark,   "dark"},
                                };

    QSettings settings(settingsFilePath(), QSettings::IniFormat);
    const QString savedKey = settings.value("ui/theme", "system").toString();
    ThemeManager::Mode savedMode = ThemeManager::Mode::System;

    QMenu *themeMenu = ui->View->addMenu("Тема");
    QActionGroup *group = new QActionGroup(this);

    for (const ThemeItem &item : items) {
        QAction *action = themeMenu->addAction(item.text);
        action->setCheckable(true);
        group->addAction(action);

        if (savedKey == item.key) {
            action->setChecked(true);
            savedMode = item.mode;
        }

        const ThemeManager::Mode mode = item.mode;
        const QString key = item.key;
        connect(action, &QAction::triggered, this, [this, mode, key]() {
            m_theme->setMode(mode);
            QSettings settings(settingsFilePath(), QSettings::IniFormat);
            settings.setValue("ui/theme", key);
        });
    }

    if (!group->checkedAction())
        group->actions().constFirst()->setChecked(true);

    setupIconSetMenu(themeMenu);
    m_theme->setMode(savedMode);
}

void MainWindow::updateToolbarIcons()
{
    ui->actionQuickConnect->setIcon(m_theme->icon("connect"));
    ui->actionQuickDisconnect->setIcon(m_theme->icon("disconnect"));
    ui->Log->setIcon(m_theme->icon("log"));
    ui->actionTrapWindow->setIcon(m_theme->icon("traps"));
    ui->actionTrapWindow->setIconText(" SNMP-трапы");
    ui->Log->setIconText(" Лог");
}

void MainWindow::setupIconSetMenu(QMenu *themeMenu)
{
    struct IconSetItem {
        const char *text;
        const char *key;
    };
    const IconSetItem items[] = {
                                  {"Условные обозначения", "scheme"},
                                  {"Электрика",            "electric"},
                                  {"Серверная",            "server"},
                                  };

    QSettings settings(settingsFilePath(), QSettings::IniFormat);
    const QString savedKey = settings.value("ui/iconSet", "scheme").toString();

    themeMenu->addSeparator();
    QMenu *iconMenu = themeMenu->addMenu("Значки схемы");
    QActionGroup *group = new QActionGroup(this);

    for (const IconSetItem &item : items) {
        QAction *action = iconMenu->addAction(item.text);
        action->setCheckable(true);
        group->addAction(action);
        action->setChecked(savedKey == item.key);

        const QString key = item.key;
        connect(action, &QAction::triggered, this, [this, key]() {
            m_theme->setIconSet(key);
            QSettings settings(settingsFilePath(), QSettings::IniFormat);
            settings.setValue("ui/iconSet", key);
        });
    }

    if (!group->checkedAction()) {
        group->actions().constFirst()->setChecked(true);
        m_theme->setIconSet("scheme");
    } else {
        m_theme->setIconSet(savedKey);
    }
}

void MainWindow::updateTrayIcon(){
    if (!m_trayIcon)
        return;

    using Mode = UpsSchematicModel::Mode;
    QString icon = "nolink";
    QString text = "нет данных от ИБП";
    if (m_schematicModel->hasMode()) {
        switch (m_schematicModel->mode()) {
        case Mode::Mains:      icon = "mains";   text = "работа от сети";       break;
        case Mode::Battery:    icon = "battery"; text = "работа от батарей";    break;
        case Mode::LowBattery: icon = "lowbat";  text = "низкий заряд батареи"; break;
        case Mode::Bypass:     icon = "bypass";  text = "байпас";               break;
        case Mode::Test:       icon = "test";    text = "тест батареи";         break;
        case Mode::OutputOff:  icon = "outoff";  text = "выход отключён";       break;
        case Mode::Fault:      icon = "fault";   text = "авария";               break;
        }
    }
    m_trayIcon->setIcon(QIcon(QString(":/resources/icons/tray/tray_%1.svg").arg(icon)));
    m_trayIcon->setToolTip("Мониторинг ИБП\n" + text);
}

void MainWindow::notify(const QString &text, QSystemTrayIcon::MessageIcon icon){
    if (!m_trayIcon || !ui->actionNotifications->isChecked())
        return;
    m_trayIcon->showMessage("Мониторинг ИБП", text, icon, 10000);
}

void MainWindow::resetNotifications(){
    m_notifyModeValid = false;
    m_notifiedAlarms.clear();
}

void MainWindow::notifyModeChange(){
    using Mode = UpsSchematicModel::Mode;
    if (!m_schematicModel->hasMode())
        return;

    const Mode mode = m_schematicModel->mode();
    const bool initial = !m_notifyModeValid;
    const Mode previous = m_notifyMode;
    m_notifyMode = mode;
    m_notifyModeValid = true;

    if (mode == Mode::Test || (!initial && mode == previous))
        return;
    if (mode == Mode::Mains) {
        if (!initial && previous != Mode::Test)
            notify("ИБП работает от сети", QSystemTrayIcon::Information);
        return;
    }

    QString text;
    QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Warning;
    switch (mode) {
    case Mode::Battery:    text = "Пропала входная сеть, работа от батарей"; break;
    case Mode::LowBattery: text = "Низкий заряд батареи";  icon = QSystemTrayIcon::Critical; break;
    case Mode::Bypass:     text = "ИБП перешёл на байпас"; break;
    case Mode::OutputOff:  text = "Выход ИБП отключён";    break;
    case Mode::Fault:      text = "Авария ИБП";            icon = QSystemTrayIcon::Critical; break;
    default:               return;
    }
    notify(initial ? "Подключено. " + text : text, icon);
}

void MainWindow::updateSchematicAnimation()
{
    m_schematicModel->setAnimationsEnabled(isVisible() && !isMinimized());
}

void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange)
        updateSchematicAnimation();
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    updateSchematicAnimation();
}

void MainWindow::hideEvent(QHideEvent *event)
{
    QMainWindow::hideEvent(event);
    m_schematicModel->setAnimationsEnabled(false);
}

void MainWindow::setupAutostartAction()
{
    QAction *action = ui->actionAutostartAtLogin;
    if (!Autostart::isSupported()) {
        action->setVisible(false);
        return;
    }

    action->setChecked(Autostart::isEnabled());
    if (Autostart::isSetForAllUsers()) {
        action->setEnabled(false);
        action->setText(action->text() + " (для всех пользователей)");
        return;
    }

    connect(action, &QAction::toggled, this, [this, action](bool on) {
        if (Autostart::setEnabled(on))
            return;
        QSignalBlocker blocker(action);
        action->setChecked(!on);
        QMessageBox::warning(this, "Автозапуск", "Не удалось изменить автозапуск программы.");
    });
}

void MainWindow::quickConnect(bool automatic)
{
    m_autoConnecting = automatic;
    logWindow->setPaused(false);
    if (m_lastConnectionType == ConnectionType::Com && !m_lastConnectedPort.isEmpty()) {
        m_pendingPort = m_lastConnectedPort;
        upsMonitor->connectToPort(m_lastConnectedPort);
    } else if (m_lastConnectionType == ConnectionType::Snmp && !m_lastSnmpParams.ip.isEmpty()) {
        m_pendingSnmpParams = m_lastSnmpParams;
        snmpMonitor->connectToHost(m_lastSnmpParams);
    } else {
        connectionSettingsWindow->show();
        connectionSettingsWindow->raise();
        connectionSettingsWindow->activateWindow();
    }
}

void MainWindow::onConnectFailed(const QString &reason)
{
    const bool automatic = m_autoConnecting;
    m_autoConnecting = false;

    logWindow->appendLine(reason);
    statusBar()->showMessage(reason, 15000);

    if (automatic || !isVisible() || isMinimized()) {
        notify(reason, QSystemTrayIcon::Warning);
        return;
    }

    QWidget *parent = connectionSettingsWindow->isVisible() ? static_cast<QWidget *>(connectionSettingsWindow) : this;
    auto *box = new QMessageBox(QMessageBox::Warning, "Подключение не удалось", reason, QMessageBox::Ok, parent);
    box->setAttribute(Qt::WA_DeleteOnClose);
    box->open();
}