#ifndef SNMPTRAPEVENTS_H
#define SNMPTRAPEVENTS_H

#include "snmptrapdata.h"
#include <QString>

QString trapEventText(const QString &event);
QString trapEventText(const SnmpTrap &trap);

#endif