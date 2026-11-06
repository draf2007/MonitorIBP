#include "snmptraplistener.h"
#include "upsmibalarms.h"
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <QSocketNotifier>
#include <QRegularExpression>
#include <QDebug>

static QString oidToString(const oid *name, size_t lenght){
    QStringList parts;
    for (size_t i = 0; i < lenght; ++i) {
        parts << QString::number(name[i]);
    }
    return parts.join('.');
}

static QString variableValueToString(const struct variable_list *vars){
    switch (vars->type) {
    case ASN_OCTET_STR:
        return QString::fromLatin1((char*)vars->val.string, vars->val_len);
    case ASN_INTEGER:
    case ASN_COUNTER:
    case ASN_GAUGE:
    case ASN_TIMETICKS:
        return QString::number(*vars->val.integer);
    case ASN_OBJECT_ID:
        return oidToString(vars->val.objid, vars->val_len / sizeof(oid));
    case ASN_IPADDRESS:
        return QString("%1.%2.%3.%4")
            .arg((quint8)vars->val.string[0])
            .arg((quint8)vars->val.string[1])
            .arg((quint8)vars->val.string[2])
            .arg((quint8)vars->val.string[3]);
    default:
        return QString("<тип 0x%1>").arg(vars->type, 0, 16);
    }
}

static QString v1TrapOid(const struct snmp_pdu *pdu){
    static const char *genericNames[] = {"coldStart", "warmStart", "linkDown", "linkUp",
                                         "authenticationFailure", "egpNeighborLoss"};
    if (pdu->trap_type >= 0 && pdu->trap_type <= 5) {
        return genericNames[pdu->trap_type];
    }
    return QString("%1.0.%2").arg(oidToString(pdu->enterprise, pdu->enterprise_length))
        .arg(pdu->specific_type);
}

SnmpTrapListener::SnmpTrapListener(QObject *parent)
    : QObject(parent)
{

}

SnmpTrapListener::~SnmpTrapListener(){
    stop();
}

bool SnmpTrapListener::start(const SnmpTrapSettings &settings){
    if (m_session != nullptr) {
        stop();
    }
    m_settings = settings;

    QByteArray spec = QString("udp:%1").arg(settings.port).toLatin1();
    struct netsnmp_transport_s *transport = netsnmp_transport_open_server("MonitorIBP_trap", spec.constData());

    if (transport == nullptr) {
        emit errorOccurred(QString("Не удалось открыть порт %1 для приёма SNMP-трапов").arg(settings.port));
        return false;
    }

    struct snmp_session sessionTemplate;
    snmp_sess_init(&sessionTemplate);
    sessionTemplate.callback = &SnmpTrapListener::handleIncomingPdu;
    sessionTemplate.callback_magic = this;
    m_session = snmp_add(&sessionTemplate, transport, nullptr, nullptr);

    if (m_session == nullptr) {
        emit errorOccurred("Не удалось зарегестрировать сессию приёма SNMP-трапов");
        return false;
    }

    m_transport = transport;

    QSocketNotifier *notifier = new QSocketNotifier(m_transport->sock, QSocketNotifier::Read, this);
    QObject::connect(notifier, &QSocketNotifier::activated, this, &SnmpTrapListener::onSocketActivated);
    m_notifier = notifier;

    emit logMessage(QString("Приём SNMP-трапов запущен на порту %1").arg(settings.port));
    return true;
}

void SnmpTrapListener::stop(){
    if (m_notifier !=nullptr) {
        m_notifier->setEnabled(false);
        delete m_notifier;
        m_notifier = nullptr;
    }

    if (m_session != nullptr) {
        snmp_close(m_session);
        m_session = nullptr;
        emit logMessage("Приём SNMP-трапов остановлен");
    }

    m_transport = nullptr;

}

void SnmpTrapListener::onSocketActivated(){
    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(m_transport->sock, &fdset);
    snmp_read(&fdset);
}

QString SnmpTrapListener::sourceIpOf(struct snmp_pdu *pdu) const{
    if (m_transport == nullptr || m_transport->f_fmtaddr == nullptr || pdu->transport_data == nullptr) {
        return "?";
    }

    char *addr = m_transport->f_fmtaddr(m_transport, pdu->transport_data, pdu->transport_data_length);
    QString text = QString::fromLatin1(addr);
    free(addr);

    QRegularExpressionMatch match = QRegularExpression("\\[([^\\]]+)\\]").match(text);
    return match.hasMatch() ? match.captured(1) : text;
}

int SnmpTrapListener::handleIncomingPdu(int operation, struct snmp_session *sp, int reqid, struct snmp_pdu *pdu, void *magic){

    Q_UNUSED(reqid);

    if (operation != NETSNMP_CALLBACK_OP_RECEIVED_MESSAGE) {
        return 1;
    }

    auto *self = static_cast<SnmpTrapListener *>(magic);

    SnmpTrap trap;
    trap.receivedAt = QDateTime::currentDateTime();
    trap.sourceIp = self->sourceIpOf(pdu);

    switch (pdu->version) {
    case SNMP_VERSION_1:  trap.version = "v1";  break;
    case SNMP_VERSION_2c: trap.version = "v2c"; break;
    case SNMP_VERSION_3:  trap.version = "v3";  break;
    default:              trap.version = "?";   break;
    }

    if (pdu->community != nullptr) {
        trap.community = QString::fromLatin1((char*)pdu->community, pdu->community_len);
    }

    if (!self->m_settings.allowedIp.isEmpty() && trap.sourceIp != self->m_settings.allowedIp) {
        if (trap.event.isEmpty()) {
            trap.event = trap.trapOid;
        }
        emit self->logMessage(QString("Трап от %1 отклонён: адрес не разрешён в настройках").arg(trap.sourceIp));
        return 1;
    }
    if (!self->m_settings.community.isEmpty() && trap.community != self->m_settings.community) {
        if (trap.event.isEmpty()) {
            trap.event = trap.trapOid;
        }
        emit self->logMessage(QString("Трап от %1 отклонён: неверный community \"%2\"").arg(trap.sourceIp, trap.community));
        return 1;
    }

    if (pdu->command == SNMP_MSG_INFORM) {
        struct snmp_pdu *reply = snmp_clone_pdu(pdu);
        if (reply != nullptr) {
            reply->command = SNMP_MSG_RESPONSE;
            reply->errstat = 0;
            reply->errindex = 0;
            if (snmp_send(sp, reply) == 0) {
                snmp_free_pdu(reply);
            }
        }
    }

    if (pdu->command == SNMP_MSG_TRAP) {
        trap.trapOid = v1TrapOid(pdu);
    }

    static const oid snmpTrapOidOID[] = {1, 3, 6, 1, 6, 3, 1, 1, 4, 1, 0};

    QString fallbackTrapOid;

    for (struct variable_list *vars = pdu->variables; vars != nullptr; vars = vars->next_variable) {
        QString valueString = variableValueToString(vars);

        bool isTrapOid = (vars->name_length == OID_LENGTH(snmpTrapOidOID))
                         && memcmp(vars->name, snmpTrapOidOID, sizeof(snmpTrapOidOID)) == 0;

        if (isTrapOid) {
            trap.trapOid = valueString;
        } else {
            trap.variables << QString("%1 = %2").arg(oidToString(vars->name, vars->name_length), valueString);
            if (vars->type == ASN_OBJECT_ID) {
                const int alarm = upsMibAlarmFromOid(valueString);
                if (alarm > 0) {
                    trap.detail = upsMibAlarmName(alarm);
                }
            }
            if (fallbackTrapOid.isEmpty() && vars->type == ASN_OBJECT_ID) {
                fallbackTrapOid = valueString;
            }
            if (trap.event.isEmpty() && pdu->command == SNMP_MSG_TRAP
                && vars->type == ASN_OCTET_STR && !valueString.isEmpty()) {
                trap.event = valueString;
            }
            if (trap.trapOid.isEmpty()) {
                trap.trapOid = fallbackTrapOid;
            }
            if (trap.event.isEmpty() && pdu->command == SNMP_MSG_TRAP && pdu->trap_type >= 0 && pdu->trap_type <= 5) {
                trap.event = trap.trapOid;
            }
        }
    }

    if (trap.event.isEmpty()) {
        trap.event = trap.trapOid;
    }

    qDebug() << "Трап" << trap.trapOid << trap.variables;
    emit self->logMessage(QString("Трап от %1: %2").arg(trap.sourceIp, trap.detail.isEmpty() ? trap.event : trap.detail));
    emit self->trapReceived(trap);

    return 1;
}

bool SnmpTrapListener::isRunning() const{
    return m_session != nullptr;
}