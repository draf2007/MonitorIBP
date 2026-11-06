#ifndef MEASUREMENTLOG_H
#define MEASUREMENTLOG_H

#include <QObject>
#include <QTimer>
#include "core/dailylogfile.h"
#include "core/upsdatastruct.h"

class MeasurementLog : public QObject
{
    Q_OBJECT

public:
    explicit MeasurementLog(QObject *parent = nullptr);
    void setRecording(bool on);
    void startSession(const QString &link, int intervalSec);
    void stopSession();
    bool inSession() const { return m_inSession; }
    QString link() const { return m_link; }
    void setIntervalSec(int intervalSec);
    void setLinkAlive(bool alive);
    void setStatus(const UpsStatus &status, int batteryCells);
    static QStringList columnNames();

signals:
    void rowWritten(const QStringList &cells);

public slots:
    void writeNow();

private:
    void updateTimer();
    QStringList formatRow() const;

    DailyLogFile m_file;
    QTimer m_timer;

    UpsStatus m_status{};
    bool m_hasStatus = false;
    int m_batteryCells = 0;

    QString m_link;
    bool m_recording = false;
    bool m_inSession = false;
    bool m_linkAlive = true;
};

#endif