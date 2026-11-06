#ifndef CONNECTTOCOMPORT_H
#define CONNECTTOCOMPORT_H

#include <QObject>
#include <QString>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>

class connectToComPort : public QObject
{
    Q_OBJECT
public:
    explicit connectToComPort(const QString &portName = "", QObject *parent = nullptr);

    bool isConnected() const;

    void sendCommand(const QString &command);

    ~connectToComPort();

private:

    QSerialPort *m_port;

    QString m_portName;

    bool m_isConnected;

    bool openPort();

    void closePort();

    QTimer *m_reopenTimer;
    static constexpr int ReopenIntervalMs = 2000;

    QByteArray m_receiveBuffer;

private slots:

    void onDataReady();

    void onError(QSerialPort::SerialPortError error);

    void onReopenTimer();

signals:

    void dataReceived(const QByteArray &data);

    void connectionStatusChanged(bool connected);

    void errorOccurred(const QString &errorMessage);

    void logMessage(const QString &line);

    void openFailed(const QString &reason);

public slots:
    void connectToPort(const QString &nameComPort);

    void disconnectToPort();
};

#endif
