#include "megatecupsmonitor.h"
#include "megatecparser.h"
#include <QDebug>

MegatecUpsMonitor::MegatecUpsMonitor(QObject *parent)
    : QObject(parent)
    ,m_port(new connectToComPort("",this))
    ,m_responseTimeoutTimer(new QTimer(this))
    ,m_pollDelayTimer(new QTimer(this))

{
    m_responseTimeoutTimer->setSingleShot(true);
    m_pollDelayTimer->setSingleShot(true);

    QObject::connect(m_port, &connectToComPort::connectionStatusChanged, this, &MegatecUpsMonitor::onPortConnectionChanged);
    QObject::connect(m_port, &connectToComPort::dataReceived, this, &MegatecUpsMonitor::onDataReceived);
    QObject::connect(m_port, &connectToComPort::errorOccurred, this, &MegatecUpsMonitor::errorOccurred);
    QObject::connect(m_port, &connectToComPort::logMessage, this, &MegatecUpsMonitor::logMessage);
    QObject::connect(m_port, &connectToComPort::openFailed, this, &MegatecUpsMonitor::connectFailed);

    QObject::connect(m_responseTimeoutTimer, &QTimer::timeout, this, &MegatecUpsMonitor::onResponseTimeout);
    QObject::connect(m_pollDelayTimer, &QTimer::timeout, this, &MegatecUpsMonitor::onPollDelayElapsed);

}

MegatecUpsMonitor::~MegatecUpsMonitor() = default;

void MegatecUpsMonitor::sendIdentificationRequest(){
    m_state = RequestState::WaitingForIdentification;
    m_port->sendCommand("I\r");
    m_responseTimeoutTimer->start(ResponseTimeoutMs);
}

void MegatecUpsMonitor::sendRatingsRequest(){
    m_state = RequestState::WaitingForRatings;
    m_port->sendCommand("F\r");
    m_responseTimeoutTimer->start(ResponseTimeoutMs);
}

void MegatecUpsMonitor::sendModeRequest(){
    m_state = RequestState::WaitingForMode;
    m_port->sendCommand("GMOD\r");
    m_responseTimeoutTimer->start(ResponseTimeoutMs);
}

void MegatecUpsMonitor::sendFirmwareRequest(){
    m_state = RequestState::WaitingForFirmware;
    m_port->sendCommand("SVFW\r");
    m_responseTimeoutTimer->start(ResponseTimeoutMs);
}

void MegatecUpsMonitor::sendBatteryInfoRequest(){
    m_state = RequestState::WaitingForBatteryInfo;
    m_port->sendCommand("GBAT\r");
    m_responseTimeoutTimer->start(ResponseTimeoutMs);
}

void MegatecUpsMonitor::finishPtrtInfo(){
    emit ptrtInfoReceived(m_ptrtInfo);
    m_currentAttempt = 0;
    sendStatusRequest();
}

static const char *const PtrtPollCommands[] = {"GMOD", "GOP", "GTMP", "GWS", "GBAT"};
static constexpr int PtrtPollCount = 5;

void MegatecUpsMonitor::sendPtrtPollRequest(){
    m_state = RequestState::WaitingForPtrtPoll;
    m_port->sendCommand(QByteArray(PtrtPollCommands[m_ptrtPollIndex]) + "\r");
    m_responseTimeoutTimer->start(ResponseTimeoutMs);
}

void MegatecUpsMonitor::applyPtrtStatus(UpsStatus &status){
    if (!m_extended) {
        return;
    }
    if (!m_ptrtStatus.mode.isNull()) {
        status.mode = MegatecParser::ptrtModeName(m_ptrtStatus.mode);
    }
    if (m_ptrtStatus.faultCode != 0) {
        status.upsFailed = true;
    }
    for (const QString &text : MegatecParser::ptrtAlarmTexts(m_ptrtStatus)) {
        if (!status.alarms.contains(text)) {
            status.alarms << text;
        }
    }
}

void MegatecUpsMonitor::sendStatusRequest(){
    m_state = RequestState::WaitingForStatus;
    m_port->sendCommand("Q1\r");
    m_responseTimeoutTimer->start(ResponseTimeoutMs);
}

void MegatecUpsMonitor::sendBatteryLevelRequest(){
    m_state = RequestState::WaitingForBatteryLevel;
    m_port->sendCommand("BL\r");
    m_responseTimeoutTimer->start(ResponseTimeoutMs);
}

void MegatecUpsMonitor::sendNextCommand(){
    m_currentCommand = m_commandQueue.dequeue();
    m_state = RequestState::WaitingForCommandAck;
    m_port->sendCommand(m_currentCommand.text + "\r");
    m_responseTimeoutTimer->start(ResponseTimeoutMs);
}

void MegatecUpsMonitor::sendControlCommand(UpsCommand command, const UpsShutdownParams &params){
    const QString name = upsCommandName(command);

    if (!isConnected()) {
        emit errorOccurred(QString("Команда «%1» не отправлена: нет подключения по COM").arg(name));
        return;
    }

    PendingCommand pending{command, QString(), QString()};
    switch (command) {
    case UpsCommand::QuickTest:      pending.text = "T";    break;
    case UpsCommand::DeepTest:       pending.text = "TL";   break;
    case UpsCommand::CancelTest:     pending.text = "CT";   break;
    case UpsCommand::ToggleBeeper:   pending.text = "Q";    break;
    case UpsCommand::Shutdown:       pending.text = "SOFF"; pending.fallback = "S.2R9999"; break;
    case UpsCommand::TurnOn:         pending.text = "SON";  pending.fallback = "C"; break;
    case UpsCommand::CancelShutdown: pending.text = "C";    break;
    case UpsCommand::TimedTest:      pending.text = QString("T%1").arg(qBound(1, params.testMinutes, 99), 2, 10, QChar('0')); break;
    }
    m_commandQueue.enqueue(pending);

    if (m_pollDelayTimer->isActive()) {
        m_pollDelayTimer->stop();
        sendNextCommand();
    }
}

void MegatecUpsMonitor::onResponseTimeout(){
    if (m_state == RequestState::Idle) {
        return;
    }

    if (m_state == RequestState::WaitingForCommandAck) {
        emit logMessage(QString("Команда «%1» отправлена, ИБП не ответил")
                            .arg(upsCommandName(m_currentCommand.command)));
        m_pollDelayTimer->start(PollDelayMs);
        return;
    }

    if (m_state == RequestState::WaitingForIdentification) {
        m_currentAttempt++;
        qDebug() << "I: нет ответа, попытка" << m_currentAttempt << "/" << MaxAttempts;
        if (m_currentAttempt < MaxAttempts) {
            sendIdentificationRequest();
        } else if (!m_gotAnyReply) {
            m_currentAttempt = 0;
            disconnectFromPort();
            emit connectFailed(QString("ИБП не отвечает на порту %1. Проверьте порт, "
                                       "кабель, питание ИБП").arg(m_portName));
        } else {
            m_currentAttempt = 0;
            sendRatingsRequest();
        }
        return;
    }

    if (m_state == RequestState::WaitingForRatings) {
        m_currentAttempt++;
        qDebug() << "F: нет ответа, попытка" << m_currentAttempt << "/" << MaxAttempts;
        if (m_currentAttempt < MaxAttempts) {
            sendRatingsRequest();
        } else {
            m_currentAttempt = 0;
            sendModeRequest();
        }
        return;
    }

    if (m_state == RequestState::WaitingForMode) {
        m_extended = false;
        emit logMessage("Протокол: базовый");
        emit protocolDetected(false);
        m_currentAttempt = 0;
        sendStatusRequest();
        return;
    }

    if (m_state == RequestState::WaitingForFirmware) {
        qDebug() << "SVFW: нет ответа";
        sendBatteryInfoRequest();
        return;
    }

    if (m_state == RequestState::WaitingForBatteryInfo) {
        qDebug() << "GBAT: нет ответа";
        finishPtrtInfo();
        return;
    }

    if (m_state == RequestState::WaitingForPtrtPoll) {
        qDebug() << PtrtPollCommands[m_ptrtPollIndex] << ": нет ответа";
        m_ptrtPollIndex = (m_ptrtPollIndex + 1) % PtrtPollCount;
        m_pollDelayTimer->start(PollDelayMs);
        return;
    }

    if (m_state == RequestState::WaitingForBatteryLevel) {
        m_currentAttempt++;
        qDebug() << "BL: нет ответа, попытка" << m_currentAttempt << "/" << MaxAttempts;
        if (m_currentAttempt < MaxAttempts) {
            sendBatteryLevelRequest();
        } else {
            m_currentAttempt = 0;
            sendStatusRequest();
        }
        return;
    }

    ++m_failedPolls;
    qDebug() << "Q1: нет ответа, подряд" << m_failedPolls;
    if (m_failedPolls == LinkLostAfter) {
        emit logMessage("Связь с ИБП потеряна");
        emit linkStatusChanged(false);
    }
    sendStatusRequest();

}

void MegatecUpsMonitor::connectToPort(const QString &portName){
    m_portName = portName;
    m_port->connectToPort(portName);
}

void MegatecUpsMonitor::onPortConnectionChanged(bool connected){
    if (!connected) {
        m_responseTimeoutTimer->stop();
        m_pollDelayTimer->stop();
        m_state = RequestState::Idle;
        m_currentAttempt = 0;
        m_commandQueue.clear();
        emit connectionStatusChanged(false);
        return;
    }

    emit connectionStatusChanged(true);
    m_currentAttempt = 0;
    m_extended = false;
    m_ptrtInfo = UpsPtrtInfo();
    m_ptrtStatus = UpsPtrtStatus();
    m_ptrtPollIndex = 0;
    m_hasLoggedStatus = false;
    m_failedPolls = 0;
    m_gotAnyReply = false;
    sendIdentificationRequest();
}

void MegatecUpsMonitor::onDataReceived(const QByteArray  &data){
    if (data.trimmed().isEmpty()) {
        qDebug() << "Пустая строка от ИБП — пропускаем";
        return;
    }
    m_gotAnyReply = true;

    switch (m_state) {
    case RequestState::WaitingForIdentification: {
        auto parseIDtata = MegatecParser::parseIdentification_I(data);
        if (parseIDtata != std::nullopt) {
            m_responseTimeoutTimer->stop();
            emit identificationReceived(*parseIDtata);
            emit logMessage(QString("Идентификация: производитель \"%1\", модель \"%2\", прошивка ИБП \"%3\"")
                                .arg(parseIDtata->manufacturer.trimmed(), parseIDtata->model.trimmed(), parseIDtata->fwVersion.trimmed()));
            m_currentAttempt = 0;
            sendRatingsRequest();
        }
        break;
    }
    case RequestState::WaitingForRatings: {
        auto parseFData = MegatecParser::parseNominalSpecifications_F(data);
        if (parseFData != std::nullopt) {
            m_responseTimeoutTimer->stop();
            emit ratingsReceived(*parseFData);
            emit logMessage(QString("Номиналы: %1 В, %2 Гц, %3 А, батарея %4 В")
                                .arg(parseFData->nominalVoltage)
                                .arg(parseFData->nominalFreq)
                                .arg(parseFData->nominalCurrent)
                                .arg(parseFData->nominalBatteryVoltage));
            m_currentAttempt = 0;
            sendModeRequest();
        }
        break;
    }

    case RequestState::WaitingForMode: {
        m_responseTimeoutTimer->stop();
        const auto mode = MegatecParser::parseMode_GMOD(data);
        m_extended = mode.has_value();
        if (m_extended) {
            m_ptrtStatus.mode = *mode;
            emit logMessage("Протокол: расширенный");
        } else {
            qDebug() << "GMOD: ответ" << data;
            emit logMessage("Протокол: базовый");
        }
        emit protocolDetected(m_extended);
        m_currentAttempt = 0;
        if (m_extended) {
            sendFirmwareRequest();
        } else {
            sendStatusRequest();
        }
        break;
    }

    case RequestState::WaitingForFirmware: {
        m_responseTimeoutTimer->stop();
        if (MegatecParser::parseFirmware_SVFW(data, m_ptrtInfo)) {
            emit logMessage(QString("Прошивка ИБП: %1 от %2")
                                .arg(m_ptrtInfo.fwVersion, m_ptrtInfo.fwDate.toString("dd.MM.yyyy")));
        } else {
            qDebug() << "SVFW: непонятный ответ" << data;
        }
        sendBatteryInfoRequest();
        break;
    }

    case RequestState::WaitingForBatteryInfo: {
        m_responseTimeoutTimer->stop();
        if (MegatecParser::parseBattery_GBAT(data, m_ptrtInfo)) {
            emit logMessage(QString("Батарея: %1 АКБ, конец разряда %2 В, предупреждение %3 В")
                                .arg(m_ptrtInfo.batteryCount)
                                .arg(m_ptrtInfo.batteryEndVoltage)
                                .arg(m_ptrtInfo.batteryAlarmVoltage));
        } else {
            qDebug() << "GBAT: непонятный ответ" << data;
        }
        finishPtrtInfo();
        break;
    }

    case RequestState::WaitingForStatus: {
        auto parseQ1Data = MegatecParser::parseStatus_Q1(data);
        if (parseQ1Data != std::nullopt) {
            m_responseTimeoutTimer->stop();
            if (m_failedPolls >= LinkLostAfter) {
                emit logMessage("Связь с ИБП восстановлена");
                emit linkStatusChanged(true);
            }
            m_failedPolls = 0;
            applyPtrtStatus(*parseQ1Data);
            emit statusUpdated(*parseQ1Data);
            QString numbers = QString("Uin=%1 Uin_ав=%2 Uout=%3 Нагрузка=%4% F=%5 Uбат=%6 T=%7")
                                  .arg(parseQ1Data->inVoltage)
                                  .arg(parseQ1Data->inVoltageFault)
                                  .arg(parseQ1Data->outVoltage)
                                  .arg(parseQ1Data->outLoadPercent)
                                  .arg(parseQ1Data->freq)
                                  .arg(parseQ1Data->batteryVoltage)
                                  .arg(parseQ1Data->temperature);

            QStringList activeFlags;
            if (parseQ1Data->immediateFault)    activeFlags << "авария сети";
            if (parseQ1Data->lowBatteryVoltage) activeFlags << "разряд батарей";
            if (parseQ1Data->bypassBoost)       activeFlags << "байпас";
            if (parseQ1Data->upsFailed)         activeFlags << "авария ИБП";
            if (parseQ1Data->isStandbyType)     activeFlags << "тип: резервный";
            if (parseQ1Data->testInProgress)    activeFlags << "тест";
            if (parseQ1Data->shutdownActive)    activeFlags << "таймер выключения";
            if (parseQ1Data->beeperOn)          activeFlags << "динамик";
            QString flagsText = activeFlags.isEmpty() ? "норма" : activeFlags.join(", ");
            qDebug().noquote() << QString("Q1: %1 [%2]").arg(numbers, flagsText);
            logStatusChanges(*parseQ1Data);

            m_currentAttempt = 0;
            sendBatteryLevelRequest();
        }
        break;
    }

    case RequestState::WaitingForBatteryLevel: {
        auto parseBLData = MegatecParser::parseBatteryLevel_BL(data);
        if (parseBLData != std::nullopt) {
            m_responseTimeoutTimer->stop();
            emit batteryLevelUpdated(*parseBLData);
            qDebug() << "BL: заряд" << parseBLData->batteryLevel;
            m_currentAttempt = 0;
            if (m_extended && m_commandQueue.isEmpty()) {
                sendPtrtPollRequest();
            } else {
                m_pollDelayTimer->start(PollDelayMs);
            }
        }
        break;
    }

    case RequestState::WaitingForPtrtPoll: {
        m_responseTimeoutTimer->stop();
        const QByteArray command = PtrtPollCommands[m_ptrtPollIndex];
        bool ok = false;
        if (command == "GMOD") {
            const auto mode = MegatecParser::parseMode_GMOD(data);
            ok = mode.has_value();
            if (ok) {
                m_ptrtStatus.mode = *mode;
            }
        } else if (command == "GOP") {
            ok = MegatecParser::parseOutput_GOP(data, m_ptrtStatus);
        } else if (command == "GTMP") {
            ok = MegatecParser::parseTemperature_GTMP(data, m_ptrtStatus);
        } else if (command == "GWS") {
            ok = MegatecParser::parseWarnings_GWS(data, m_ptrtStatus);
        } else if (command == "GBAT") {
            ok = MegatecParser::parseBattery_GBAT(data, m_ptrtStatus);
        }
        if (ok) {
            emit ptrtStatusUpdated(m_ptrtStatus);
        } else {
            qDebug() << command << ": непонятный ответ" << data;
        }
        m_ptrtPollIndex = (m_ptrtPollIndex + 1) % PtrtPollCount;
        m_pollDelayTimer->start(PollDelayMs);
        break;
    }

    case RequestState::WaitingForCommandAck: {
        m_responseTimeoutTimer->stop();
        const QByteArray answer = data.trimmed();
        const QString name = upsCommandName(m_currentCommand.command);

        if (answer == "ACK") {
            emit logMessage(QString("Команда «%1» принята ИБП").arg(name));
        } else if (answer == "NAK" && !m_currentCommand.fallback.isEmpty()) {
            qDebug() << "NAK на" << m_currentCommand.text << "— пробуем" << m_currentCommand.fallback;
            m_commandQueue.prepend(PendingCommand{m_currentCommand.command, m_currentCommand.fallback, QString()});
        } else {
            qDebug() << "Ответ на" << m_currentCommand.text << ":" << answer;
            emit errorOccurred(QString("ИБП отклонил команду «%1»").arg(name));
        }
        m_pollDelayTimer->start(PollDelayMs);
        break;
    }
    case RequestState::Idle:

    default:
        break;
    }
}

void MegatecUpsMonitor::onPollDelayElapsed()
{
    if (!m_commandQueue.isEmpty()) {
        sendNextCommand();
        return;
    }
    sendStatusRequest();
}

void MegatecUpsMonitor::disconnectFromPort(){
    m_port->disconnectToPort();
}

bool MegatecUpsMonitor::isConnected() const{
    return m_port->isConnected();
}

void MegatecUpsMonitor::logStatusChanges(const UpsStatus &status){
    const QString mode = upsModeText(status);
    if (!m_hasLoggedStatus || mode != upsModeText(m_loggedStatus)) {
        emit logMessage(QString("Режим ИБП: %1").arg(mode));
    }
    if (!m_hasLoggedStatus || status.alarms != m_loggedStatus.alarms) {
        if (!status.alarms.isEmpty()) {
            emit logMessage(QString("Аварии: %1").arg(status.alarms.join(", ")));
        } else if (m_hasLoggedStatus) {
            emit logMessage("Аварии сняты");
        }
    }
    if (m_logBeeper && (!m_hasLoggedStatus || status.beeperOn != m_loggedStatus.beeperOn)) {
        emit logMessage(status.beeperOn ? "Динамик включен" : "Динамик выключен");
    }
    m_loggedStatus = status;
    m_hasLoggedStatus = true;
}