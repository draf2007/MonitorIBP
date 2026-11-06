#include "comportavailables.h"
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QString>
#include <QDebug>
#include <QList>
#include <QVariant>

comPortAvailables::comPortAvailables(QObject *parent)
    : QObject(parent)
{

}

void comPortAvailables::availablesPort(){
    const auto portList = QSerialPortInfo::availablePorts();
    for (const auto &portInfo : portList)
        qDebug() << portInfo.portName() << portInfo.description();

    emit portListSend(portList);
}
