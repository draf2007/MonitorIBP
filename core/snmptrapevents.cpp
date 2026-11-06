#include "snmptrapevents.h"
#include <QHash>

static const QHash<QString, QString> &knownEvents(){
    static const QHash<QString, QString> events{
        {"upsInputFault",         "Пропало входное питание"},
        {"upsInputFaultCancel",   "Входное питание восстановлено"},
        {"upsTest",               "Начат самотест"},
        {"upsTestCancel",         "Самотест окончен"},
        {"upsBypassOutput",       "ИБП перешёл на байпас"},
        {"upsBypassOutputCancel", "ИБП вернулся с байпаса"},
        {"coldStart",             "Устройство включилось"},
        {"warmStart",             "Устройство перезапустилось"},
        {"linkDown",              "Сетевой интерфейс отключён"},
        {"linkUp",                "Сетевой интерфейс подключён"},
        {"authenticationFailure", "Запрос с неверным community"},
        {"1.3.6.1.2.1.33.2.1",    "ИБП перешёл на батарею"},
        {"1.3.6.1.2.1.33.2.2",    "Тест завершён"},
        {"1.3.6.1.2.1.33.2.3",    "Появилась тревога"},
        {"1.3.6.1.2.1.33.2.4",    "Тревога снята"},
        };
    return events;
}

QString trapEventText(const QString &event){
    return knownEvents().value(event, event);
}

QString trapEventText(const SnmpTrap &trap){
    const QString text = trapEventText(trap.event);
    return trap.detail.isEmpty() ? text : QString("%1: %2").arg(text, trap.detail);
}