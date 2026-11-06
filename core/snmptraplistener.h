#ifndef SNMPTRAPLISTENER_H
#define SNMPTRAPLISTENER_H

#include "snmptrapdata.h"
#include <QObject>
#include <QString>

class QSocketNotifier;

class SnmpTrapListener : public QObject
{
    Q_OBJECT
public:
    explicit SnmpTrapListener(QObject *parent = nullptr);
    ~SnmpTrapListener() override;

    bool start(const SnmpTrapSettings &settings);
    void stop();
    bool isRunning() const;

signals:
    void trapReceived(const SnmpTrap &trap);
    void logMessage(const QString &line);
    void errorOccurred(const QString &message);

private:
    struct netsnmp_transport_s *m_transport = nullptr;
    struct snmp_session *m_session = nullptr;
    QSocketNotifier *m_notifier = nullptr;

    SnmpTrapSettings m_settings;
    QString sourceIpOf(struct snmp_pdu *pdu) const;

    static int handleIncomingPdu(int operation, struct snmp_session *sp, int reqid, struct snmp_pdu *pdu, void *magic);

private slots:
    void onSocketActivated();
};

#endif
