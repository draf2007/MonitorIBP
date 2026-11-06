#ifndef MEGATECUPSMONITOR_H
#define MEGATECUPSMONITOR_H

#include <QObject>
#include <QTimer>
#include <QQueue>
#include "connecttocomport.h"
#include "upsdatastruct.h"
#include "upscommand.h"

class MegatecUpsMonitor : public QObject
{
    Q_OBJECT
public:
    explicit MegatecUpsMonitor(QObject *parent = nullptr);
    ~MegatecUpsMonitor() override;
    void connectToPort(const QString &portName);
    void disconnectFromPort();
    bool isConnected() const;
    void sendControlCommand(UpsCommand command, const UpsShutdownParams &params = UpsShutdownParams());

signals:
    void connectionStatusChanged(bool connected);
    void identificationReceived(const UpsIdentification &info);
    void ratingsReceived(const UpsNominalSpecifications &ratings);
    void statusUpdated(const UpsStatus &status);
    void batteryLevelUpdated(const UpsBatteryLevel &status);
    void protocolDetected(bool extended);
    void ptrtInfoReceived(const UpsPtrtInfo &info);
    void ptrtStatusUpdated(const UpsPtrtStatus &status);
    void errorOccurred(const QString &message);
    void logMessage(const QString &line);
    void linkStatusChanged(bool alive);
    void connectFailed(const QString &reason);

private slots:
    void onPortConnectionChanged(bool connected);
    void onDataReceived(const QByteArray  &data);
    void onResponseTimeout();
    void onPollDelayElapsed();

private:
    enum class RequestState{
        Idle,
        WaitingForIdentification,
        WaitingForRatings,
        WaitingForMode,
        WaitingForFirmware,
        WaitingForBatteryInfo,
        WaitingForPtrtPoll,
        WaitingForStatus,
        WaitingForBatteryLevel,
        WaitingForCommandAck
    };

    struct PendingCommand {
        UpsCommand command;
        QString text;
        QString fallback;
    };

    static constexpr int ResponseTimeoutMs = 1500;
    static constexpr int PollDelayMs = 200;
    static constexpr int MaxAttempts = 3;
    static constexpr int LinkLostAfter = 3;
    int m_failedPolls = 0;
    bool m_gotAnyReply = false;
    QString m_portName;

    void sendIdentificationRequest();
    void sendRatingsRequest();
    void sendModeRequest();
    void sendFirmwareRequest();
    void sendBatteryInfoRequest();
    void finishPtrtInfo();
    void sendPtrtPollRequest();
    void applyPtrtStatus(UpsStatus &status);
    void logStatusChanges(const UpsStatus &status);
    void sendStatusRequest();
    void sendBatteryLevelRequest();
    void sendNextCommand();

    connectToComPort *m_port;
    QTimer *m_responseTimeoutTimer;
    QTimer *m_pollDelayTimer;

    RequestState m_state = RequestState::Idle;
    int m_currentAttempt = 0;

    QQueue<PendingCommand> m_commandQueue;
    PendingCommand m_currentCommand;
    bool m_extended = false;
    UpsPtrtInfo m_ptrtInfo;
    UpsPtrtStatus m_ptrtStatus;
    int m_ptrtPollIndex = 0;
    UpsStatus m_loggedStatus;
    bool m_hasLoggedStatus = false;

};

#endif
