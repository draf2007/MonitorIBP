#ifndef SNMPUPSMONITOR_H
#define SNMPUPSMONITOR_H

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QString>
#include <QStringList>
#include <QList>
#include <memory>
#include <functional>
#include "upsdatastruct.h"
#include "snmpconnectionparams.h"
#include "snmpcarddriver.h"
#include "upscommand.h"

class SnmpUpsMonitor : public QObject
{
    Q_OBJECT
public:
    explicit SnmpUpsMonitor(QObject *parent = nullptr);
    ~SnmpUpsMonitor() override;

    void connectToHost(const SnmpConnectionParams &params);
    void disconnectFromHost();
    bool isConnected() const;

    void setCardType(const QString &cardId);

    void sendControlCommand(UpsCommand command, const UpsShutdownParams &params = {});

signals:
    void connectionStatusChanged (bool connected);
    void identificationReceived(const UpsIdentification &info);
    void ratingsReceived(const UpsNominalSpecifications &ratings);
    void statusUpdated(const UpsStatus &status);
    void batteryLevelUpdated(const UpsBatteryLevel &level);
    void errorOccurred(const QString &message);
    void logMessage(const QString &line);
    void linkStatusChanged(bool alive);
     void connectFailed(const QString &reason);

private slots:
    void onPollTimerElapsed();
    void onSocketActivated();
    void onSnmpTimeoutTick();
private:

    struct SnmpReply {
        bool timedOut = true;
        long errstat = 0;
        SnmpValues values;
    };
    using ReplyHandler = std::function<void(const SnmpReply &reply)>;
    using GetHandler = std::function<void(bool ok, const SnmpValues &values)>;

    struct PendingRequest {
        SnmpUpsMonitor *monitor;
        quint64 generation;
        int valueCount;
        ReplyHandler handler;
    };

    static constexpr int PollIntervalMs = 5000;
    static constexpr int BeeperHoldMs = 15000;

    struct snmp_session *m_snmpSession = nullptr;
    QTimer *m_pollTimer;
    QTimer *m_snmpTimeoutTimer;
    class QSocketNotifier *m_socketNotifier = nullptr;
    int m_socket = -1;
    QList<PendingRequest *> m_pending;
    quint64 m_sessionGeneration = 0;
    bool m_pollInFlight = false;
    bool m_connected = false;
    static constexpr int LinkLostAfter = 3;
    int m_failedPolls = 0;
    SnmpVersion m_version = SnmpVersion::V1;
    QString m_hostText;
    int m_lastSendErrno = 0;
    QByteArray m_writeCommunity;

    QString m_cardId;
    std::unique_ptr<SnmpCardDriver> m_driver;

    UpsStatus m_lastStatus;
    QStringList m_lastAlarms;
    bool m_hasLastStatus = false;
    int m_mainsLossCount = 0;

    QElapsedTimer m_beeperHoldTimer;
    bool m_beeperHoldValue = false;

    bool m_shutdownCancelRequested = false;

    static int onSnmpResponse(int operation, struct snmp_session *session, int reqid,
                              struct snmp_pdu *pdu, void *magic);
    void sendRequest(struct snmp_pdu *pdu, int valueCount, ReplyHandler handler);
    void snmpGet(const QStringList &oids, GetHandler done);
    void snmpGetOneByOne(const QStringList &oids, int index, SnmpValues values, GetHandler done);
    void snmpSet(const SnmpSetRequests &requests, std::function<void(bool ok)> done);
    void closeSession();

    void checkConnection(bool firstAttempt);
    void fetchIdentification();
    void fetchStatus();
    void applyStatus(const SnmpValues &statusValues, const SnmpValues &extraValues);
    void logStatusChanges(const UpsStatus &status, const QStringList &alarms);
    void registerPollFailure();
    void registerPollSuccess();
};

#endif
