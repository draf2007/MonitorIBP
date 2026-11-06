#include "upsshutdownscenario.h"

UpsShutdownScenario::UpsShutdownScenario(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
{
    m_timer->setSingleShot(true);
    QObject::connect(m_timer, &QTimer::timeout, this, &UpsShutdownScenario::onTimeout);
}

void UpsShutdownScenario::start(const UpsShutdownParams &params){
    if (isActive()) {
        cancel("запущен новый");
    }
    m_params = params;

    emit logMessage(params.restartDelaySec >= 0
                        ? QString("Сценарий: выключение через %1 с, включение через %2 с после выключения")
                              .arg(params.shutdownDelaySec).arg(params.restartDelaySec)
                        : QString("Сценарий: выключение через %1 с, без включения")
                              .arg(params.shutdownDelaySec));

    if (params.shutdownDelaySec > 0) {
        startTimer(State::WaitingShutdown, params.shutdownDelaySec);
    } else {
        sendShutdown();
    }
}

void UpsShutdownScenario::cancel(const QString &reason){
    if (!isActive()) {
        return;
    }
    m_timer->stop();
    m_state = State::Idle;
    emit stateChanged(m_state);
    emit logMessage(reason.isEmpty() ? QString("Сценарий выключения отменён")
                                     : QString("Сценарий выключения отменён: %1").arg(reason));
    if (m_outputOff) {
        emit logMessage("ИБП остался выключенным (байпас), включите его командой «Включить»");
    }
}

void UpsShutdownScenario::turnOnNow(){
    m_timer->stop();
    if (m_state == State::Idle) {
        m_params = UpsShutdownParams{};
    }
    sendStartup();
}

int UpsShutdownScenario::secondsLeft() const{
    if (m_state != State::WaitingShutdown && m_state != State::WaitingStartup) {
        return -1;
    }
    const int left = m_durationSec - int(m_elapsed.elapsed() / 1000);
    return left > 0 ? left : 0;
}

void UpsShutdownScenario::onStatusUpdated(const UpsStatus &status){
    m_outputOff = status.outputOff;
    m_hasStatus = true;

    if (m_state == State::ShuttingDown && status.outputOff) {
        emit logMessage("Сценарий: ИБП выключен (байпас)");
        if (m_params.restartDelaySec < 0) {
            finish("Сценарий завершён, включение не задано");
        } else {
            startTimer(State::WaitingStartup, m_params.restartDelaySec);
        }
    } else if (m_state == State::StartingUp && !status.outputOff) {
        finish("Сценарий: ИБП включён");
    }
}

void UpsShutdownScenario::startTimer(State state, int seconds){
    m_state = state;
    m_durationSec = seconds;
    m_elapsed.start();
    m_timer->start(seconds * 1000);
    emit stateChanged(m_state);
}

void UpsShutdownScenario::finish(const QString &message){
    m_timer->stop();
    m_state = State::Idle;
    emit stateChanged(m_state);
    emit logMessage(message);
}

void UpsShutdownScenario::sendShutdown(){
    startTimer(State::ShuttingDown, ConfirmTimeoutSec);
    emit commandRequested(UpsCommand::Shutdown, UpsShutdownParams{0, -1});
}

void UpsShutdownScenario::sendStartup(){
    if (m_hasStatus && !m_outputOff) {
        finish("Сценарий: ИБП уже работает, команда включения не нужна");
        return;
    }
    startTimer(State::StartingUp, ConfirmTimeoutSec);
    emit commandRequested(UpsCommand::TurnOn, UpsShutdownParams{});
}

void UpsShutdownScenario::onTimeout(){
    switch (m_state) {
    case State::WaitingShutdown:
        sendShutdown();
        break;
    case State::ShuttingDown:
        finish(QString("Сценарий: ИБП не выключился за %1 с, сценарий остановлен").arg(ConfirmTimeoutSec));
        break;
    case State::WaitingStartup:
        sendStartup();
        break;
    case State::StartingUp:
        finish(QString("Сценарий: ИБП не включился за %1 с").arg(ConfirmTimeoutSec));
        break;
    case State::Idle:
        break;
    }
}