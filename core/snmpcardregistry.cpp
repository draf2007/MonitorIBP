#include "snmpcardregistry.h"
#include "snmpcardv1.h"

const QVector<SnmpCardInfo> &snmpCards()
{
    static const QVector<SnmpCardInfo> cards = {
                                                 { "snmp-card-v1", "SNMP-card-v1",
                                                  "Однофазные ИБП. Стандартный UPS-MIB (RFC 1628), ветка .1.3.6.1.2.1.33.\n"
                                                  "Мониторинг: вход, выход, батарея, тревоги, тест, отсчёт выключения.\n"
                                                  "Управление: тесты, выключение/включение, звук.",
                                                  []() { return std::make_unique<SnmpCardV1>(); } },
                                                 };
    return cards;
}

const SnmpCardInfo &findSnmpCard(const QString &id)
{
    for (const SnmpCardInfo &card : snmpCards()) {
        if (card.id == id) {
            return card;
        }
    }
    return snmpCards().first();
}