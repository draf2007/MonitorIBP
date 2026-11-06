#include "connecttocomport.h"
#include <QDebug>
#include <QObject>

connectToComPort::connectToComPort(const QString &portName,QObject *parent)
    : QObject(parent)
    ,m_port(new QSerialPort(this))
    ,m_portName(portName)
    ,m_isConnected(false)
    ,m_reopenTimer(new QTimer(this))
{
    m_port->setPortName(m_portName);
    m_port->setBaudRate(QSerialPort::Baud2400);
    m_port->setDataBits(QSerialPort::Data8);
    m_port->setParity(QSerialPort::NoParity);
    m_port->setStopBits(QSerialPort::OneStop);
    m_port->setFlowControl(QSerialPort::NoFlowControl);

    connect(m_port, &QSerialPort::readyRead, this, &connectToComPort::onDataReady);

    connect(m_port, &QSerialPort::errorOccurred, this, &connectToComPort::onError);

    m_reopenTimer->setInterval(ReopenIntervalMs);
    connect(m_reopenTimer, &QTimer::timeout, this, &connectToComPort::onReopenTimer);

}

connectToComPort::~connectToComPort(){closePort();}

static QString portOpenErrorText(QSerialPort::SerialPortError error, const QString &systemText)
{
    switch (error) {
    case QSerialPort::PermissionError:
#ifdef Q_OS_LINUX
        return "нет доступа (порт занят другой программой или пользователь не в группе dialout)";
#else
        return "нет доступа (порт занят другой программой)";
#endif
    case QSerialPort::DeviceNotFoundError:
        return "порт не найден (устройство отключено?)";
    default:
        return systemText;
    }
}

bool connectToComPort::openPort(){
    if (m_port->isOpen()) {
        return true;
    }

    if (!m_port->open(QIODevice::ReadWrite)) {
        const QString reason = QString("Не удалось открыть порт %1: %2")
                                   .arg(m_portName, portOpenErrorText(m_port->error(), m_port->errorString()));
        qDebug() << reason;
        m_isConnected = false;
        emit connectionStatusChanged(false);
        emit openFailed(reason);
        return false;
    }
    m_isConnected = true;
    qDebug()<<"Порт " << m_portName << " успешно открыт";
    emit logMessage("Порт " + m_portName + " успешно открыт");
    emit connectionStatusChanged(true);
    return true;
}

void connectToComPort::closePort(){
    m_reopenTimer->stop();
    if (m_isConnected) {
        if (m_port->isOpen()) {
            m_port->close();
        }
        m_isConnected = false;
        qDebug() << "Порт закрыт";
        emit logMessage("Порт закрыт");
        emit connectionStatusChanged(false);
    }
}

bool connectToComPort::isConnected() const {
    return m_isConnected;
}

void connectToComPort::sendCommand(const QString &command){
    if(!m_isConnected || !m_port->isOpen()){
        qDebug() << "Порт не открыт! Комманда не может быть отправлена!";
        return;
    }
    QByteArray data = command.toUtf8();
    qint64 bytesWritten = m_port->write(data);

    if (bytesWritten == -1) {
        qDebug() << "Ошибка записи в порт"<<m_port->errorString();
        emit errorOccurred("Ошибка записи в порт " + m_port->errorString());
    }else{
        qDebug() << "Отправлено в порт: " << command;
    }
}

void connectToComPort::onDataReady(){
    QByteArray data = m_port->readAll();
    m_receiveBuffer.append(data);
    if (data.isEmpty()) {
        return;
    }
    qDebug() << "Полученны данные" << data;
    while (m_receiveBuffer.contains('\r')) {
        int endIndex = m_receiveBuffer.indexOf('\r');
        QByteArray completeMessage = m_receiveBuffer.left(endIndex+1);
        m_receiveBuffer = m_receiveBuffer.mid(endIndex+1);
        qDebug() << "Полное сообщение: " << completeMessage;
        emit dataReceived(completeMessage);
    }
}

void connectToComPort::onError(QSerialPort::SerialPortError error){
    if (error == QSerialPort::NoError) {
        return;
    }
    if (m_reopenTimer->isActive()) {
        qDebug() << "Порт ещё недоступен:" << m_port->errorString();
        return;
    }

    if (m_isConnected && m_port->isOpen()) {
        qDebug() << "Ошибка на открытом порту:" << error << m_port->errorString();
        emit logMessage(QString("Порт %1 недоступен (отключено устройство?), ждём его").arg(m_portName));
        m_reopenTimer->start();
        m_port->close();
        m_receiveBuffer.clear();
        return;
    }

    QString errorMsg = QString("Ошибка порта %1").arg(m_port->errorString());
    emit logMessage(errorMsg);
    qDebug() << errorMsg;
}

void connectToComPort::onReopenTimer(){
    if (m_port->open(QIODevice::ReadWrite)) {
        m_reopenTimer->stop();
        emit logMessage(QString("Порт %1 снова доступен").arg(m_portName));
    }
}

void connectToComPort::connectToPort(const QString &nameComPort){
    if (nameComPort.isEmpty()) {
        return;
    }
    closePort();

    m_portName = nameComPort;
    m_port->setPortName(m_portName);
    openPort();
}

void connectToComPort::disconnectToPort(){
    closePort();
}
