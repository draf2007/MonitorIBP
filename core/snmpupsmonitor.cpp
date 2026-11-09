#include "snmpupsmonitor.h"
#include "snmpcardregistry.h"
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <QSocketNotifier>
#include <QDebug>
#include <QStandardPaths>
#include <cstring>

SnmpUpsMonitor::SnmpUpsMonitor(QObject *parent)
    : QObject(parent)
    , m_pollTimer(new QTimer(this))
    , m_snmpTimeoutTimer(new QTimer(this))
{
    SOCK_STARTUP;
    snmp_set_do_debugging(0);
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_DONT_READ_CONFIGS, 1);
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_DONT_PERSIST_STATE, 1);
    const QString snmpDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/snmp";
    netsnmp_ds_set_string(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_PERSISTENT_DIR, snmpDir.toUtf8().constData());
    qputenv("MIBS", ENV_SEPARATOR);
    init_snmp("MonitorIBP");

    QObject::connect(m_pollTimer, &QTimer::timeout, this, &SnmpUpsMonitor::onPollTimerElapsed);

    m_snmpTimeoutTimer->setInterval(100);
    QObject::connect(m_snmpTimeoutTimer, &QTimer::timeout, this, &SnmpUpsMonitor::onSnmpTimeoutTick);
}

SnmpUpsMonitor::~SnmpUpsMonitor(){
    disconnectFromHost();
    SOCK_CLEANUP;
}

void SnmpUpsMonitor::connectToHost(const SnmpConnectionParams &params){
    if (m_connected) {
        disconnectFromHost();
    }
    const SnmpCardInfo &card = findSnmpCard(m_cardId);
    m_driver = card.create();
    m_hasLastStatus = false;
    m_failedPolls = 0;
    m_beeperChangePending = false;
    m_version = params.version;
    m_hostText = params.ip;
    m_lastSendErrno = 0;

    struct snmp_session sessionTemplate;
    snmp_sess_init(&sessionTemplate);

    QByteArray peerAddress = QString("%1:%2").arg(params.ip).arg(params.port).toLatin1();
    sessionTemplate.peername = peerAddress.data();

    sessionTemplate.retries = 2;
    sessionTemplate.timeout = 1000000;

    QByteArray communityBytes;
    QByteArray usernameBytes;
    QByteArray authPassphraseBytes;
    QByteArray privPassphraseBytes;

    if (params.version == SnmpVersion::V3) {
        sessionTemplate.version = SNMP_VERSION_3;
        sessionTemplate.securityModel = SNMP_SEC_MODEL_USM;

        usernameBytes = params.username.toLatin1();
        sessionTemplate.securityName = usernameBytes.data();
        sessionTemplate.securityNameLen = usernameBytes.length();

        switch (params.securityLevel) {
        case SnmpSecurityLevel::NoAuthNoPriv:
            sessionTemplate.securityLevel = SNMP_SEC_LEVEL_NOAUTH;
            break;
        case SnmpSecurityLevel::AuthNoPriv:
            sessionTemplate.securityLevel = SNMP_SEC_LEVEL_AUTHNOPRIV;
            break;
        case SnmpSecurityLevel::AuthPriv:
            sessionTemplate.securityLevel = SNMP_SEC_LEVEL_AUTHPRIV;
            break;
        }

        if (params.securityLevel == SnmpSecurityLevel::AuthNoPriv
            || params.securityLevel == SnmpSecurityLevel::AuthPriv) {

            if (params.authProtocol == SnmpAuthProtocol::MD5) {
                sessionTemplate.securityAuthProto = (oid *)usmHMACMD5AuthProtocol;
                sessionTemplate.securityAuthProtoLen = USM_AUTH_PROTO_MD5_LEN;
            } else {
                sessionTemplate.securityAuthProto = (oid *)usmHMACSHA1AuthProtocol;
                sessionTemplate.securityAuthProtoLen = USM_AUTH_PROTO_SHA_LEN;
            }

            authPassphraseBytes = params.authPassword.toLatin1();
            sessionTemplate.securityAuthKeyLen = USM_AUTH_KU_LEN;

            if (generate_Ku(sessionTemplate.securityAuthProto,
                            sessionTemplate.securityAuthProtoLen,
                            (u_char *)authPassphraseBytes.data(),
                            authPassphraseBytes.length(),
                            sessionTemplate.securityAuthKey,
                            &sessionTemplate.securityAuthKeyLen) != SNMPERR_SUCCESS) {
                emit connectionStatusChanged(false);
                emit connectFailed("Не удалось вычислить ключ аутентификации SNMPv3");
                return;
            }
        }

        if (params.securityLevel == SnmpSecurityLevel::AuthPriv) {
            if (params.privProtocol == SnmpPrivProtocol::DES) {
                sessionTemplate.securityPrivProto = (oid *)usmDESPrivProtocol;
                sessionTemplate.securityPrivProtoLen = USM_PRIV_PROTO_DES_LEN;
            } else {
                sessionTemplate.securityPrivProto = (oid *)usmAESPrivProtocol;
                sessionTemplate.securityPrivProtoLen = USM_PRIV_PROTO_AES_LEN;
            }

            privPassphraseBytes = params.privPassword.toLatin1();
            sessionTemplate.securityPrivKeyLen = USM_PRIV_KU_LEN;

            if (generate_Ku(sessionTemplate.securityAuthProto,
                            sessionTemplate.securityAuthProtoLen,
                            (u_char *)privPassphraseBytes.data(),
                            privPassphraseBytes.length(),
                            sessionTemplate.securityPrivKey,
                            &sessionTemplate.securityPrivKeyLen) != SNMPERR_SUCCESS) {
                emit errorOccurred("Не удалось вычислить ключ шифрования SNMPv3");
                emit connectionStatusChanged(false);
                return;
            }
        }
    } else {
        sessionTemplate.version = (params.version == SnmpVersion::V2c) ? SNMP_VERSION_2c : SNMP_VERSION_1;

        communityBytes = params.community.toLatin1();
        sessionTemplate.community = (u_char *)communityBytes.data();
        sessionTemplate.community_len = communityBytes.length();
    }
    m_writeCommunity = (params.version == SnmpVersion::V3) ? QByteArray()
                                                           : params.effectiveWriteCommunity().toLatin1();

    m_snmpSession = snmp_open(&sessionTemplate);

    if (m_snmpSession == nullptr) {
        int liberr = 0, syserr = 0;
        char *errstring = nullptr;
        snmp_error(&sessionTemplate, &liberr, &syserr, &errstring);

        const QString reason = QString("Не удалось открыть SNMP-сессию с %1: %2")
                                   .arg(params.ip, errstring ? errstring : "причина неизвестна");
        if (errstring != nullptr) {
            free(errstring);
        }

        emit connectionStatusChanged(false);
        emit connectFailed(reason);
        return;

        emit errorOccurred(QString("Не удалось открыть SNMP-сессию: %1")
                               .arg(errstring ? errstring : "причина неизвестна"));

        if (errstring != nullptr) {
            free(errstring);
        }

        emit connectionStatusChanged(false);
        return;
    }

    emit logMessage(QString("Открыта SNMP-сессия с %1:%2").arg(params.ip).arg(params.port));

    emit logMessage(QString("Тип карты: %1").arg(card.name));

    m_socket = snmp_sess_transport(snmp_sess_pointer(m_snmpSession))->sock;
    m_socketNotifier = new QSocketNotifier(m_socket, QSocketNotifier::Read, this);
    QObject::connect(m_socketNotifier, &QSocketNotifier::activated, this, &SnmpUpsMonitor::onSocketActivated);

    checkConnection(true);
}

void SnmpUpsMonitor::disconnectFromHost(){
    closeSession();

    if (m_connected) {
        m_connected = false;
        emit connectionStatusChanged(false);
    }
}

void SnmpUpsMonitor::closeSession(){
    m_pollTimer->stop();
    m_snmpTimeoutTimer->stop();
    m_pollInFlight = false;
    ++m_sessionGeneration;

    delete m_socketNotifier;
    m_socketNotifier = nullptr;

    if (m_snmpSession != nullptr) {
        snmp_close(m_snmpSession);
        m_snmpSession = nullptr;
    }
    qDeleteAll(m_pending);
    m_pending.clear();
}

bool SnmpUpsMonitor::isConnected() const{
    return m_connected;
}

void SnmpUpsMonitor::setCardType(const QString &cardId){
    m_cardId = cardId;
}

int SnmpUpsMonitor::onSnmpResponse(int operation, struct snmp_session *session, int,
                                   struct snmp_pdu *pdu, void *magic){
    if (operation != NETSNMP_CALLBACK_OP_RECEIVED_MESSAGE
        && operation != NETSNMP_CALLBACK_OP_TIMED_OUT
        && operation != NETSNMP_CALLBACK_OP_SEND_FAILED
        && operation != NETSNMP_CALLBACK_OP_SEC_ERROR) {
        return 1;
    }

    PendingRequest *request = static_cast<PendingRequest *>(magic);
    SnmpUpsMonitor *monitor = request->monitor;

    if (operation == NETSNMP_CALLBACK_OP_SEND_FAILED && session != nullptr)
        monitor->m_lastSendErrno = session->s_errno;

    SnmpReply reply;
    reply.values = SnmpValues(request->valueCount);

    if (operation == NETSNMP_CALLBACK_OP_RECEIVED_MESSAGE && pdu != nullptr) {
        reply.timedOut = false;
        reply.errstat = pdu->errstat;

        int index = 0;
        for (struct variable_list *vars = pdu->variables;
             vars != nullptr && index < reply.values.size();
             vars = vars->next_variable, ++index) {
            SnmpValue &value = reply.values[index];

            switch (vars->type) {
            case ASN_INTEGER:
            case ASN_COUNTER:
            case ASN_GAUGE:
            case ASN_TIMETICKS:
                value.type = SnmpValue::Type::Integer;
                value.integer = *vars->val.integer;
                break;
            case ASN_OCTET_STR:
                value.type = SnmpValue::Type::String;
                value.text = QString::fromUtf8(reinterpret_cast<const char *>(vars->val.string), int(vars->val_len));
                break;
            case ASN_OBJECT_ID:
                value.type = SnmpValue::Type::Oid;
                for (size_t i = 0; i < vars->val_len / sizeof(oid); ++i) {
                    value.text += "." + QString::number(vars->val.objid[i]);
                }
                break;
            default:
                break;
            }
        }
    }

    const quint64 generation = request->generation;
    ReplyHandler handler = request->handler;
    monitor->m_pending.removeOne(request);
    delete request;

    QTimer::singleShot(0, monitor, [monitor, generation, handler, reply]() {
        if (generation == monitor->m_sessionGeneration) {
            handler(reply);
        }
    });
    return 1;
}

void SnmpUpsMonitor::sendRequest(struct snmp_pdu *pdu, int valueCount, ReplyHandler handler){
    PendingRequest *request = new PendingRequest{this, m_sessionGeneration, valueCount, handler};

    m_pending.append(request);

    if (m_snmpSession == nullptr || snmp_async_send(m_snmpSession, pdu, onSnmpResponse, request) == 0) {
        snmp_free_pdu(pdu);
        if (m_pending.removeOne(request)) {
            delete request;
            QTimer::singleShot(0, this, [handler]() { handler(SnmpReply()); });
        }
        return;
    }
    m_snmpTimeoutTimer->start();
}

void SnmpUpsMonitor::onSocketActivated(){
    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(m_socket, &fdset);
    snmp_read(&fdset);
}

void SnmpUpsMonitor::onSnmpTimeoutTick(){
    snmp_timeout();
    if (m_pending.isEmpty()) {
        m_snmpTimeoutTimer->stop();
    }
}

void SnmpUpsMonitor::snmpGet(const QStringList &oids, GetHandler done){
    struct snmp_pdu *pdu = snmp_pdu_create(SNMP_MSG_GET);

    for (const QString &oidString : oids) {
        oid anOID[MAX_OID_LEN];
        size_t anOIDLen = MAX_OID_LEN;

        if (!snmp_parse_oid(oidString.toLatin1().constData(), anOID, &anOIDLen)) {
            qDebug() << "Не удалось разобрать OID:" << oidString;
            emit errorOccurred("Ошибка в описании карты: неверный адрес параметра");
            snmp_free_pdu(pdu);
            QTimer::singleShot(0, this, [done]() { done(false, SnmpValues()); });
            return;
        }

        snmp_add_null_var(pdu, anOID, anOIDLen);
    }

    sendRequest(pdu, oids.size(), [this, oids, done](const SnmpReply &reply) {
        if (reply.timedOut) {
            qDebug() << "SNMP: таймаут";
            done(false, SnmpValues());
            return;
        }

        if (reply.errstat == SNMP_ERR_NOSUCHNAME) {
            if (oids.size() == 1) {
                done(true, SnmpValues(1));
            } else {
                snmpGetOneByOne(oids, 0, SnmpValues(oids.size()), done);
            }
            return;
        }

        if (reply.errstat != SNMP_ERR_NOERROR) {
            emit errorOccurred(QString("ИБП вернул ошибку SNMP: %1").arg(snmp_errstring(int(reply.errstat))));
            done(false, SnmpValues());
            return;
        }

        done(true, reply.values);
    });
}

void SnmpUpsMonitor::snmpGetOneByOne(const QStringList &oids, int index, SnmpValues values, GetHandler done){
    if (index >= oids.size()) {
        done(true, values);
        return;
    }
    snmpGet({oids[index]}, [this, oids, index, values, done](bool ok, const SnmpValues &one) mutable {
        if (!ok) {
            done(false, SnmpValues());
            return;
        }
        values[index] = one[0];
        snmpGetOneByOne(oids, index + 1, values, done);
    });
}

void SnmpUpsMonitor::sendControlCommand(UpsCommand command, const UpsShutdownParams &params){
    const QString name = upsCommandName(command);

    if (!m_connected || !m_driver) {
        emit errorOccurred(QString("Команда «%1» не отправлена: нет подключения по SNMP").arg(name));
        return;
    }

    const SnmpSetRequests requests = m_driver->commandRequests(command, params, m_lastStatus);
    if (requests.isEmpty()) {
        emit errorOccurred(QString("Команда «%1» не поддерживается этой картой").arg(name));
        return;
    }

    snmpSet(requests, [this, command, name](bool ok) {
        if (!ok) {
            emit errorOccurred(QString("Команда «%1» не выполнена").arg(name));
            return;
        }

        emit logMessage(QString("Команда «%1» отправлена").arg(name));

        if (command == UpsCommand::ToggleBeeper) {
            m_beeperHoldValue = !m_lastStatus.beeperOn;
            m_beeperHoldTimer.start();
        }
        if (command == UpsCommand::CancelShutdown) {
            m_shutdownCancelRequested = true;
        } else if (command == UpsCommand::Shutdown) {
            m_shutdownCancelRequested = false;
        }

        fetchStatus();
    });
}

void SnmpUpsMonitor::snmpSet(const SnmpSetRequests &requests, std::function<void(bool ok)> done){
    struct snmp_pdu *pdu = snmp_pdu_create(SNMP_MSG_SET);

    for (const SnmpSetRequest &request : requests) {
        oid anOID[MAX_OID_LEN];
        size_t anOIDLen = MAX_OID_LEN;

        if (!snmp_parse_oid(request.oid.toLatin1().constData(), anOID, &anOIDLen)) {
            qDebug() << "Не удалось разобрать OID:" << request.oid;
            emit errorOccurred("Ошибка в описании карты: неверный адрес параметра");
            snmp_free_pdu(pdu);
            QTimer::singleShot(0, this, [done]() { done(false); });
            return;
        }

        const char type = (request.value.type == SnmpValue::Type::Oid) ? 'o' : 'i';
        const QByteArray text = (type == 'o') ? request.value.text.toLatin1()
                                              : QByteArray::number(qlonglong(request.value.integer));

        if (snmp_add_var(pdu, anOID, anOIDLen, type, text.constData()) != 0) {
            qDebug() << "Не удалось подготовить запись" << request.oid << "=" << text;
            emit errorOccurred("Не удалось подготовить команду для карты");
            snmp_free_pdu(pdu);
            QTimer::singleShot(0, this, [done]() { done(false); });
            return;
        }

        qDebug() << "SNMP SET" << request.oid << "=" << text;
    }

    if (!m_writeCommunity.isEmpty()) {
        pdu->community = static_cast<u_char *>(malloc(size_t(m_writeCommunity.size())));
        memcpy(pdu->community, m_writeCommunity.constData(), size_t(m_writeCommunity.size()));
        pdu->community_len = size_t(m_writeCommunity.size());
    }

    sendRequest(pdu, requests.size(), [this, done](const SnmpReply &reply) {
        if (reply.timedOut) {
            emit errorOccurred("Нет ответа на запись (проверьте community на запись)");
            done(false);
            return;
        }

        if (reply.errstat != SNMP_ERR_NOERROR) {
            const long errstat = reply.errstat;
            const bool accessDenied = errstat == SNMP_ERR_NOACCESS || errstat == SNMP_ERR_NOSUCHNAME
                                      || errstat == SNMP_ERR_NOTWRITABLE || errstat == SNMP_ERR_READONLY
                                      || errstat == SNMP_ERR_AUTHORIZATIONERROR;
            emit errorOccurred(QString("Карта отклонила запись: %1%2")
                                   .arg(snmp_errstring(int(errstat)))
                                   .arg(accessDenied ? " — проверьте community на запись" : ""));
            done(false);
            return;
        }
        done(true);
    });
}

void SnmpUpsMonitor::checkConnection(bool firstAttempt){
    snmpGet({".1.3.6.1.2.1.1.3.0"}, [this, firstAttempt](bool ok, const SnmpValues &values) {
        if (ok && !values[0].isMissing()) {
            fetchIdentification();
            return;
        }

        if (firstAttempt && m_version == SnmpVersion::V3) {
            qDebug() << "SNMPv3: первая попытка — обнаружение engineID, повторяем запрос";
            if (usm_create_user_from_session(m_snmpSession) == SNMPERR_SUCCESS) {
                checkConnection(false);
                return;
            }
            emit errorOccurred("Не удалось зарегистрировать пользователя SNMPv3 в локальной таблице USM");
        }

        QString reason;
        if (m_lastSendErrno != 0)
            reason = QString("Адрес %1 недоступен: %2")
                         .arg(m_hostText, QString::fromLocal8Bit(strerror(m_lastSendErrno)));
        else if (m_version == SnmpVersion::V3)
            reason = QString("Карта %1 не отвечает. Проверьте адрес, сеть, имя пользователя "
                             "и пароли SNMPv3").arg(m_hostText);
        else
            reason = QString("Карта %1 не отвечает. Проверьте адрес, сеть и community").arg(m_hostText);

        closeSession();
        emit connectionStatusChanged(false);
        emit connectFailed(reason);
    });
}

void SnmpUpsMonitor::fetchIdentification(){
    snmpGet(m_driver->identificationOids(), [this](bool ok, const SnmpValues &values) {
        if (!ok) {
            emit errorOccurred("ИБП не отвечает по SNMP");
            closeSession();
            emit connectionStatusChanged(false);
            return;
        }

        UpsIdentification identification;
        UpsNominalSpecifications ratings;
        m_driver->parseIdentification(values, identification, ratings);

        emit logMessage(QString("Идентификация: производитель \"%1\", модель \"%2\", прошивка ИБП \"%3\", прошивка карты \"%4\"")
                            .arg(identification.manufacturer, identification.model,
                                 identification.fwVersion, identification.agentFwVersion));
        emit logMessage(QString("Имя \"%1\", место \"%2\"").arg(identification.sysName, identification.sysLocation));
        emit logMessage(QString("Номиналы: %1 В, %2 Гц, %3 Вт")
                            .arg(ratings.nominalVoltage).arg(ratings.nominalFreq).arg(ratings.nominalPowerW));

        m_connected = true;
        emit connectionStatusChanged(true);
        emit identificationReceived(identification);
        emit ratingsReceived(ratings);

        fetchStatus();
        m_pollTimer->start(PollIntervalMs);
    });
}

void SnmpUpsMonitor::fetchStatus(){
    if (m_pollInFlight) {
        return;
    }
    m_pollInFlight = true;

    snmpGet(m_driver->statusOids(), [this](bool ok, const SnmpValues &statusValues) {
        if (!ok) {
            m_pollInFlight = false;
            registerPollFailure();
            return;
        }

        const QStringList extraOids = m_driver->extraStatusOids(statusValues);
        if (extraOids.isEmpty()) {
            m_pollInFlight = false;
            applyStatus(statusValues, SnmpValues());
            return;
        }

        snmpGet(extraOids, [this, statusValues](bool extraOk, const SnmpValues &extraValues) {
            m_pollInFlight = false;
            applyStatus(statusValues, extraOk ? extraValues : SnmpValues());
        });
    });
}

void SnmpUpsMonitor::applyStatus(const SnmpValues &statusValues, const SnmpValues &extraValues){
    registerPollSuccess();

    const SnmpPollResult result = m_driver->parseStatus(statusValues, extraValues);
    UpsStatus status = result.status;

    if (m_hasLastStatus && status.mainsLost && !m_lastStatus.mainsLost) {
        ++m_mainsLossCount;
    }
    status.inputLineBads = m_mainsLossCount;

    const bool beeperHeld = m_beeperHoldTimer.isValid();
    if (beeperHeld) {
        if (status.beeperOn == m_beeperHoldValue || m_beeperHoldTimer.elapsed() > BeeperHoldMs) {
            m_beeperHoldTimer.invalidate();
        } else {
            status.beeperOn = m_beeperHoldValue;
        }
    }

    if (!beeperHeld && m_hasLastStatus && status.beeperOn != m_lastStatus.beeperOn && !m_beeperChangePending) {
        m_beeperChangePending = true;
        status.beeperOn = m_lastStatus.beeperOn;
    } else {
        m_beeperChangePending = false;
    }

    logStatusChanges(status, result.activeAlarms);

    qDebug().noquote() << QString("SNMP: Uin=%1 Uout=%2 Нагрузка=%3% F=%4 Uбат=%5 Заряд=%6%")
                              .arg(status.inVoltage)
                              .arg(status.outVoltage)
                              .arg(status.outLoadPercent)
                              .arg(status.freq)
                              .arg(status.batteryVoltage)
                              .arg(result.batteryLevel.batteryLevel);

    emit statusUpdated(status);
    emit batteryLevelUpdated(result.batteryLevel);
}

static QString testKindText(UpsTestKind kind)
{
    switch (kind) {
    case UpsTestKind::Quick: return " (быстрый)";
    case UpsTestKind::Deep:  return " (до разряда)";
    case UpsTestKind::Other: return " (другой)";
    default:                 return QString();
    }
}

static QString testResultText(UpsTestResult result)
{
    switch (result) {
    case UpsTestResult::Passed:     return "пройден";
    case UpsTestResult::Warning:    return "пройден с предупреждением";
    case UpsTestResult::Error:      return "ошибка";
    case UpsTestResult::Aborted:    return "прерван";
    case UpsTestResult::InProgress: return "идёт";
    case UpsTestResult::NoTest:    return "тестов не было";
    default:                        return "нет данных";
    }
}

void SnmpUpsMonitor::logStatusChanges(const UpsStatus &status, const QStringList &alarms){
    if (!m_hasLastStatus || upsModeText(status) != upsModeText(m_lastStatus)) {
        emit logMessage(QString("Режим ИБП: %1").arg(upsModeText(status)));
    }

    if (!m_hasLastStatus || alarms != m_lastAlarms) {
        if (status.testResult == UpsTestResult::InProgress) {
            emit logMessage(QString("Идёт тест%1").arg(testKindText(status.testKind)));
        } else {
            emit logMessage(QString("Последний тест%1: %2")
                                .arg(testKindText(status.testKind), testResultText(status.testResult)));
        }
    }

    const bool countdown = status.shutdownCountdownSec >= 0;
    const bool lastCountdown = m_hasLastStatus && m_lastStatus.shutdownCountdownSec >= 0;
    if (countdown && !lastCountdown) {
        emit logMessage(QString("Выключение через %1 с").arg(status.shutdownCountdownSec));
    } else if (!countdown && lastCountdown) {
        emit logMessage(m_shutdownCancelRequested ? "Отсчёт выключения отменён"
                                                  : "Отсчёт выключения закончился");
        m_shutdownCancelRequested = false;
    }

    if (m_logBeeper && m_hasLastStatus && status.beeperOn != m_lastStatus.beeperOn) {
        emit logMessage(status.beeperOn ? "Пищалка включена" : "Пищалка выключена");
    }

    if (!m_hasLastStatus ? status.mainsLost : status.mainsLost != m_lastStatus.mainsLost) {
        emit logMessage(status.mainsLost
                            ? QString("Пропала сеть (пропаданий с запуска программы: %1)").arg(status.inputLineBads)
                            : QString("Сеть восстановлена"));
    }

    m_lastStatus = status;
    m_lastAlarms = alarms;
    m_hasLastStatus = true;
}

void SnmpUpsMonitor::onPollTimerElapsed(){
    fetchStatus();
}

void SnmpUpsMonitor::registerPollFailure(){
    ++m_failedPolls;
    qDebug() << "SNMP: опрос не удался, подряд" << m_failedPolls;
    if (m_failedPolls == LinkLostAfter) {
        emit logMessage("Связь с ИБП потеряна");
        emit linkStatusChanged(false);
    }
}

void SnmpUpsMonitor::registerPollSuccess(){
    if (m_failedPolls >= LinkLostAfter) {
        emit logMessage("Связь с ИБП восстановлена");
        emit linkStatusChanged(true);
    }
    m_failedPolls = 0;
}
