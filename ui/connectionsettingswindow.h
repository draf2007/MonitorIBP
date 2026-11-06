#ifndef CONNECTIONSETTINGSWINDOW_H
#define CONNECTIONSETTINGSWINDOW_H

#include <QDialog>
#include <QSerialPortInfo>

#include "snmpconnectionparams.h"

namespace Ui {
class ConnectionSettingsWindow;
}

class ConnectionSettingsWindow : public QDialog
{
    Q_OBJECT

public:
    explicit ConnectionSettingsWindow(QWidget *parent = nullptr);
    ~ConnectionSettingsWindow();
    void setSnmpParams(const SnmpConnectionParams &params);
    void setLastComPortName(const QString &portName);

public slots:
    void updatePortList(const QList<QSerialPortInfo> &ports);
    void onConnectionStatusChanged(bool connected);

signals:
    void refreshRequested();
    void connectRequested(const QString &portName);
    void snmpConnectRequested(const SnmpConnectionParams &params);

private:
    Ui::ConnectionSettingsWindow *ui;
    QString m_lastComPortName;
};

#endif
