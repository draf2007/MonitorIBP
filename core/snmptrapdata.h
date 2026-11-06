#ifndef SNMPTRAPDATA_H
#define SNMPTRAPDATA_H

#include <QString>
#include <QStringList>
#include <QDateTime>

struct SnmpTrapSettings{
    quint16 port = 162;
    QString community;
    QString allowedIp;
    bool autoStart = false;
};

struct SnmpTrap{
    QDateTime receivedAt;
    QString sourceIp;
    QString version;
    QString community;
    QString trapOid;
    QString event;
    QStringList variables;
    QString detail;

};

#endif
