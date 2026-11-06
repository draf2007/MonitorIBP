#include "connectionsettingswindow.h"
#include "ui_connectionsettingswindow.h"
#include <QString>
#include <QList>
#include <QHostAddress>
#include <QMessageBox>

ConnectionSettingsWindow::ConnectionSettingsWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ConnectionSettingsWindow)
{
    ui->setupUi(this);
    QObject::connect(ui->refreshComPortList, &QPushButton::clicked, this, &ConnectionSettingsWindow::refreshRequested);
    QObject::connect(ui->connectBtn, &QPushButton::clicked, this, [this](){
            emit connectRequested(ui->viewComPort->currentData().toString());
    });
    QObject::connect(ui->snmpConnectBtn, &QPushButton::clicked, this, [this](){
        QString ipText = ui->snmpIpAddress->text().trimmed();
        QHostAddress address;
        if (!address.setAddress(ipText)) {
            QMessageBox::warning(this, "Некорректный ip адрес", QString("\"%1\" не является корректным IP-адресом").arg(ipText));
            return;
        }

        SnmpConnectionParams params;
        params.ip = ui->snmpIpAddress->text();
        params.port = static_cast<quint16>(ui->snmpPort->text().toUShort());
        params.community = ui->snmpCommunity->text();
        params.writeCommunity = ui->snmpWriteCommunity->text();

        switch (ui->snmpVersionCombo->currentIndex()) {
        case 0:
            params.version = SnmpVersion::V1;
            break;
        case 1:
            params.version = SnmpVersion::V2c;
            break;
        case 2:
            params.version = SnmpVersion::V3;
            break;
        default:
            break;
        }

        params.username = ui->snmpUsername->text();

        switch (ui->snmpSecurityLevelCombo->currentIndex()) {
        case 0:
            params.securityLevel = SnmpSecurityLevel::NoAuthNoPriv;
            break;
        case 1:
            params.securityLevel = SnmpSecurityLevel::AuthNoPriv;
            break;
        case 2:
            params.securityLevel = SnmpSecurityLevel::AuthPriv;
            break;
        default:
            break;
        }

        params.authProtocol = (ui->snmpAuthProtocolCombo->currentIndex() == 0) ? SnmpAuthProtocol::MD5 : SnmpAuthProtocol::SHA;
        params.authPassword = ui->snmpAuthPassword->text();

        params.privProtocol = (ui->snmpPrivProtocolCombo->currentIndex() == 0) ? SnmpPrivProtocol::DES : SnmpPrivProtocol::AES;
        params.privPassword = ui->snmpPrivPassword->text();

        emit snmpConnectRequested(params);
    });
}

ConnectionSettingsWindow::~ConnectionSettingsWindow()
{
    delete ui;
}

void ConnectionSettingsWindow::updatePortList(const QList<QSerialPortInfo> &ports){
    ui->viewComPort->clear();
    for (const QSerialPortInfo &port : ports) {
        const QString text = port.description().isEmpty()
        ? port.portName()
        : port.portName() + " — " + port.description();
        ui->viewComPort->addItem(text, port.portName());
    }

    if (!m_lastComPortName.isEmpty()) {
        const int index = ui->viewComPort->findData(m_lastComPortName);
        if (index != -1)
            ui->viewComPort->setCurrentIndex(index);
    }
}

void ConnectionSettingsWindow::onConnectionStatusChanged(bool connected){
    if (connected) {
        close();
    }
}

void ConnectionSettingsWindow::setSnmpParams(const SnmpConnectionParams &params){
    if (params.ip.isEmpty()) {
        return;
    }

    ui->snmpIpAddress->setText(params.ip);
    ui->snmpPort->setText(QString::number(params.port));
    ui->snmpCommunity->setText(params.community);
    ui->snmpWriteCommunity->setText(params.writeCommunity);

    switch (params.version) {
    case SnmpVersion::V1:  ui->snmpVersionCombo->setCurrentIndex(0); break;
    case SnmpVersion::V2c: ui->snmpVersionCombo->setCurrentIndex(1); break;
    case SnmpVersion::V3:  ui->snmpVersionCombo->setCurrentIndex(2); break;
    }

    ui->snmpUsername->setText(params.username);
    ui->snmpWriteCommunity->setText(params.writeCommunity);

    switch (params.securityLevel){
    case SnmpSecurityLevel::NoAuthNoPriv: ui->snmpSecurityLevelCombo->setCurrentIndex(0); break;
    case SnmpSecurityLevel::AuthNoPriv:   ui->snmpSecurityLevelCombo->setCurrentIndex(1); break;
    case SnmpSecurityLevel::AuthPriv:     ui->snmpSecurityLevelCombo->setCurrentIndex(2); break;
    }

    ui->snmpAuthProtocolCombo->setCurrentIndex(params.authProtocol==SnmpAuthProtocol::SHA ? 1:0);
    ui->snmpAuthPassword->setText(params.authPassword);

    ui->snmpPrivProtocolCombo->setCurrentIndex(params.privProtocol==SnmpPrivProtocol::AES ? 1:0);
    ui->snmpPrivPassword->setText(params.privPassword);
}

void ConnectionSettingsWindow::setLastComPortName(const QString &portName){
    m_lastComPortName = portName;
}
