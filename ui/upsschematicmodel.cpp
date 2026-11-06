#include "upsschematicmodel.h"

#include <QtMath>

UpsSchematicModel::UpsSchematicModel(QObject *parent)
    : QObject(parent)
{
    rebuild();
}

void UpsSchematicModel::setStatus(const UpsStatus &status)
{
    m_status = status;
    m_hasStatus = true;
    m_lastUpdate = QDateTime::currentDateTime();
    rebuild();
}

void UpsSchematicModel::setBatteryLevel(const UpsBatteryLevel &level)
{
    bool ok = false;
    const int value = level.batteryLevel.toInt(&ok);
    m_batteryLevel = ok ? value : -1;
    rebuild();
}

void UpsSchematicModel::setBatteryConfig(const UpsBatteryConfig &config)
{
    m_batteryConfig = config;
    rebuild();
}

void UpsSchematicModel::setLinkLost(bool lost)
{
    m_linkLost = lost;
    rebuild();
}

void UpsSchematicModel::setConnected(bool connected)
{
    m_connected = connected;
    if (!connected) {
        m_hasStatus = false;
        m_batteryLevel = -1;
        m_linkLost = false;
    }
    rebuild();
}

UpsSchematicModel::Mode UpsSchematicModel::detectMode() const
{
    const UpsStatus &s = m_status;

    if (s.mode == "Авария")
        return Mode::Fault;
    if (s.mode == "Байпас" || s.mode == "ECO")
        return Mode::Bypass;
    if (s.mode == "Батарея")
        return s.lowBatteryVoltage ? Mode::LowBattery : Mode::Battery;
    if (s.mode == "Тест батареи" || s.mode == "Тест")
        return Mode::Test;
    if (s.mode == "Выключен" || s.mode == "Ожидание")
        return Mode::OutputOff;
    if (!s.mode.isEmpty())
        return Mode::Mains;

    if (s.testInProgress)
        return Mode::Test;
    if (s.mainsLost || s.immediateFault)
        return s.lowBatteryVoltage ? Mode::LowBattery : Mode::Battery;
    if (s.bypassBoost)
        return Mode::Bypass;
    if (s.outputOff)
        return Mode::OutputOff;
    return Mode::Mains;
}

void UpsSchematicModel::updateMode(){
    const bool hasMode = m_connected && m_hasStatus && !m_linkLost;
    const Mode mode = hasMode ? detectMode() : Mode::Mains;
    if (hasMode == m_hasMode && mode == m_mode)
        return;
    m_hasMode = hasMode;
    m_mode = mode;
    emit modeChanged();
}

void UpsSchematicModel::rebuild(){
    updateMode();

    if (!m_connected) {
        buildEmpty("Не подключено");
        emit changed();
        return;
    }
    if (!m_hasStatus) {
        buildEmpty("Ожидание данных");
        emit changed();
        return;
    }

    const UpsStatus &s = m_status;
    const Mode mode = detectMode();
    const bool onBattery = (mode == Mode::Battery || mode == Mode::LowBattery);

    QString modeText = "От сети";
    QString modeState = "ok";
    switch (mode) {
    case Mode::Mains:      modeText = "От сети";          modeState = "ok";    break;
    case Mode::Battery:    modeText = "От батареи";       modeState = "warn";  break;
    case Mode::LowBattery: modeText = "От батареи";       modeState = "alarm"; break;
    case Mode::Bypass:     modeText = "Байпас";           modeState = "warn";  break;
    case Mode::Test:       modeText = "Тест батареи";     modeState = "ok";    break;
    case Mode::OutputOff:  modeText = "Выход выключен";   modeState = "off";   break;
    case Mode::Fault:      modeText = "Байпас (авария)";  modeState = "alarm"; break;
    }
    m_banner = {
                 {"text",  modeText},
                 {"state", modeState},
                 {"meta",  QString()},
                 {"alarm", s.alarms.join(" · ")},
                 };

    QString mainsNote;
    if (onBattery)
        mainsNote = "Сеть пропала";
    else if (s.inputLineBads >= 0)
        mainsNote = QString("Отключений: %1").arg(s.inputLineBads);
    m_mains = {
                {"state",   onBattery ? "alarm" : "ok"},
                {"voltage", QString("%1 В").arg(s.inVoltage, 0, 'f', 0)},
                {"freq",    onBattery ? QString("— Гц") : QString("%1 Гц").arg(s.freq, 0, 'f', 1)},
                {"note",    mainsNote},
                };

    QString upsTitle = "Инвертор";
    QString upsState = "ok";
    if (mode == Mode::Bypass)    { upsTitle = "Инв. выкл.";  upsState = "warn"; }
    if (mode == Mode::OutputOff) { upsTitle = "Выход выкл."; upsState = "off"; }
    if (mode == Mode::Fault)     { upsTitle = "Авария";      upsState = "alarm"; }
    if (s.upsFailed)               upsState = "alarm";

    QString upsNote;
    if (s.shutdownCountdownSec > 0) {
        upsNote = QString("Выкл. через %1 с").arg(s.shutdownCountdownSec);
        if (upsState == "ok") upsState = "warn";
    } else if (s.startupCountdownSec > 0) {
        upsNote = QString("Вкл. через %1 с").arg(s.startupCountdownSec);
    } else if (s.shutdownActive) {
        upsNote = "Идёт выключение";
        if (upsState == "ok") upsState = "warn";
    } else if (mode == Mode::Test) {
        upsNote = "Тест";
    }
    m_ups = {
              {"state", upsState},
              {"title", upsTitle},
              {"temp",  s.temperature > 0 ? QString("%1 °C").arg(s.temperature, 0, 'f', 0) : QString()},
              {"note",  upsNote},
              };

    const bool loadPowered = (mode != Mode::OutputOff);
    const bool unprotected = (mode == Mode::Bypass || mode == Mode::Fault);
    const bool overload = loadPowered && s.outLoadPercent > 100;
    QString loadState = "ok";
    if (!loadPowered)     loadState = "off";
    else if (overload)    loadState = "alarm";
    else if (unprotected) loadState = "warn";
    m_load = {
               {"state",    loadState},
               {"voltage",  loadPowered ? QString("%1 В").arg(s.outVoltage, 0, 'f', 0) : QString("0 В")},
               {"percent",  loadPowered ? QString("%1 %").arg(s.outLoadPercent) : QString("Нет")},
               {"level",    loadPowered ? qBound(0, s.outLoadPercent, 100) : 0},
               {"overload", overload},
               {"note",     overload ? QString("Перегрузка") : unprotected ? QString("Без защиты") : QString()},
               };

    QString batteryState = "ok";
    QString batteryNote = (m_batteryLevel >= 95) ? "Заряжена" : "Заряд";
    if (mode == Mode::Battery) {
        batteryState = "warn";
        batteryNote = s.secondsOnBattery > 0
                          ? QString("На батарее %1:%2").arg(s.secondsOnBattery / 60)
                                .arg(s.secondsOnBattery % 60, 2, 10, QChar('0'))
                          : QString("Разряд");
    } else if (mode == Mode::LowBattery) {
        batteryState = "alarm";
        batteryNote = "Разряжена";
    } else if (mode == Mode::Test) {
        batteryState = "warn";
        batteryNote = "Идёт разряд";
    }
    double batteryVolts = s.batteryVoltage;
    if (s.batteryVoltagePerCell && m_batteryConfig.totalCells() > 0)
        batteryVolts = s.batteryVoltage * m_batteryConfig.totalCells();
    m_battery = {
                  {"state",   batteryState},
                  {"voltage", QString("%1 В").arg(batteryVolts, 0, 'f', 1)},
                  {"detail",  batteryDetailText()},
                  {"level",   qMax(0, m_batteryLevel)},
                  {"note",    batteryNote},
                  };

    QString lineIn = "flow", lineOut = "flow", lineBattery = "charge", bypass = "idle";
    switch (mode) {
    case Mode::Mains:
        break;
    case Mode::Battery:
        lineIn = "idle"; lineOut = "warn"; lineBattery = "discharge";
        break;
    case Mode::LowBattery:
        lineIn = "idle"; lineOut = "warn"; lineBattery = "alarm";
        break;
    case Mode::Test:
        lineIn = "idle"; lineOut = "warn"; lineBattery = "discharge";
        break;
    case Mode::Bypass:
        lineIn = "idle"; lineOut = "idle"; bypass = "warn";
        break;
    case Mode::OutputOff:
        lineIn = "charge"; lineOut = "idle";
        break;
    case Mode::Fault:
        lineIn = "idle"; lineOut = "idle"; lineBattery = "idle"; bypass = "alarm";
        break;
    }
    m_lines = {
                {"in",      lineIn},
                {"out",     lineOut},
                {"battery", lineBattery},
                {"bypass",  bypass},
                };

    if (m_linkLost)
        markAllStale();

    emit changed();
}

void UpsSchematicModel::buildEmpty(const QString &bannerText)
{
    m_banner  = {{"text", bannerText}, {"state", "off"}, {"meta", QString()}, {"alarm", QString()}};
    m_mains   = {{"state", "off"}, {"voltage", "--- В"}, {"freq", "--- Гц"}, {"note", QString()}};
    m_ups     = {{"state", "off"}, {"title", "---"}, {"temp", QString()}, {"note", QString()}};
    m_load    = {{"state", "off"}, {"voltage", "--- В"}, {"percent", "--- %"}, {"level", 0},
              {"overload", false}, {"note", QString()}};
    m_battery = {{"state", "off"}, {"voltage", "--- В"}, {"detail", "--- %"}, {"level", 0},
                 {"note", QString()}};
    m_lines   = {{"in", "off"}, {"out", "off"}, {"battery", "off"}, {"bypass", "off"}};
}

void UpsSchematicModel::markAllStale()
{
    for (QVariantMap *block : {&m_mains, &m_ups, &m_load, &m_battery})
        (*block)["state"] = "stale";
    for (auto it = m_lines.begin(); it != m_lines.end(); ++it)
        it.value() = "stale";

    m_banner["text"]  = "Нет связи";
    m_banner["state"] = "alarm";
    m_banner["meta"]  = "последний ответ " + m_lastUpdate.toString("hh:mm:ss");
}

QString UpsSchematicModel::batteryDetailText() const
{
    QStringList parts;
    const int cells = m_batteryConfig.totalCells();
    if (m_status.batteryVoltagePerCell)
        parts << QString("%1 В/эл").arg(m_status.batteryVoltage, 0, 'f', 2);
    else if (cells > 0)
        parts << QString("%1 В/эл").arg(m_status.batteryVoltage / cells, 0, 'f', 2);
    if (m_batteryLevel >= 0)
        parts << QString("%1 %").arg(m_batteryLevel);
    return parts.join(" · ");
}

void UpsSchematicModel::setAnimationsEnabled(bool on)
{
    if (m_animationsEnabled == on)
        return;
    m_animationsEnabled = on;
    emit animationsEnabledChanged();
}
