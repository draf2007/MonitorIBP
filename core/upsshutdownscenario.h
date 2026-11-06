#ifndef UPSSHUTDOWNSCENARIO_H
#define UPSSHUTDOWNSCENARIO_H

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include "upsdatastruct.h"
#include "upscommand.h"

class UpsShutdownScenario : public QObject
{
    Q_OBJECT
public:

    enum class State{
        Idle,
        WaitingShutdown,
        ShuttingDown,
        WaitingStartup,
        StartingUp
    };

    explicit UpsShutdownScenario(QObject *parent = nullptr);

    void start(const UpsShutdownParams &params);
    void cancel(const QString &reason = QString());
    void turnOnNow();

    State state() const {return m_state;}
    bool isActive() const {return m_state != State::Idle;}
    int secondsLeft() const;

public slots:
    void onStatusUpdated(const UpsStatus &status);

signals:
    void commandRequested(UpsCommand command, const UpsShutdownParams &params);
    void logMessage(const QString &line);
    void stateChanged(UpsShutdownScenario::State state);

private:
    static constexpr int ConfirmTimeoutSec = 60;

    State m_state = State::Idle;
    UpsShutdownParams m_params;
    QTimer *m_timer;
    QElapsedTimer m_elapsed;
    int m_durationSec = 0;
    bool m_outputOff = false;
    bool m_hasStatus = false;

    void startTimer(State state, int seconds);
    void finish(const QString &message);
    void sendShutdown();
    void sendStartup();
    void onTimeout();
};

#endif
