#include "measurementlog.h"

#include <QLocale>
#include <QTime>

QStringList MeasurementLog::columnNames()
{
    return {"Время", "Связь", "Вход, В", "Выход, В", "Нагрузка, %", "Частота, Гц",
            "Батарея, В", "Температура, °C", "Режим", "Аварии"};
}

static QByteArray csvHeader()
{
    return QByteArray("\xEF\xBB\xBF") + MeasurementLog::columnNames().join(';').toUtf8();
}

MeasurementLog::MeasurementLog(QObject *parent)
    : QObject(parent)
    , m_file("data", "csv", csvHeader())
{
    QObject::connect(&m_timer, &QTimer::timeout, this, &MeasurementLog::writeNow);
}

void MeasurementLog::setRecording(bool on)
{
    m_recording = on;
    updateTimer();
}

void MeasurementLog::startSession(const QString &link, int intervalSec)
{
    m_link = link;
    m_inSession = true;
    m_linkAlive = true;
    m_hasStatus = false;
    m_timer.setInterval(intervalSec * 1000);
    updateTimer();
}

void MeasurementLog::stopSession()
{
    m_inSession = false;
    m_hasStatus = false;
    updateTimer();
}

void MeasurementLog::setIntervalSec(int intervalSec)
{
    m_timer.setInterval(intervalSec * 1000);
}

void MeasurementLog::setLinkAlive(bool alive)
{
    m_linkAlive = alive;
    if (!alive)
        m_hasStatus = false;
}

void MeasurementLog::setStatus(const UpsStatus &status, int batteryCells)
{
    m_status = status;
    m_batteryCells = batteryCells;
    m_hasStatus = true;
}

void MeasurementLog::writeNow()
{
    if (!m_recording || !m_inSession || !m_linkAlive || !m_hasStatus)
        return;
    const QStringList cells = formatRow();
    m_file.write(cells.join(';'));
    emit rowWritten(cells);
}

void MeasurementLog::updateTimer()
{
    if (m_recording && m_inSession)
        m_timer.start();
    else
        m_timer.stop();
}

QStringList MeasurementLog::formatRow() const
{
    static const QLocale ru(QLocale::Russian);
    auto number = [](double value) { return ru.toString(value, 'f', 1); };

    const UpsStatus &s = m_status;
    QString battery;
    if (!s.batteryVoltagePerCell)
        battery = number(s.batteryVoltage);
    else if (m_batteryCells > 0)
        battery = number(s.batteryVoltage * m_batteryCells);

    QStringList alarms = s.alarms;
    alarms.replaceInStrings(";", ",");

    return {
        QTime::currentTime().toString("HH:mm:ss"),
        m_link,
        number(s.inVoltage),
        number(s.outVoltage),
        QString::number(s.outLoadPercent),
        s.freq > 0 ? number(s.freq) : QString(),
        battery,
        s.temperature > 0 ? number(s.temperature) : QString(),
        upsModeText(s),
        alarms.join(", "),
        };

}