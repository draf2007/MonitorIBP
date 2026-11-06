#ifndef SNMPCARDREGISTRY_H
#define SNMPCARDREGISTRY_H

#include <QString>
#include <QVector>
#include <functional>
#include <memory>
#include "snmpcarddriver.h"

struct SnmpCardInfo
{
    QString id;
    QString name;
    QString description;
    std::function<std::unique_ptr<SnmpCardDriver>()> create;
};

const QVector<SnmpCardInfo> &snmpCards();

const SnmpCardInfo &findSnmpCard(const QString &id);

#endif