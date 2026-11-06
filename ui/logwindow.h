#ifndef LOGWINDOW_H
#define LOGWINDOW_H

#include <QDialog>
#include "core/dailylogfile.h"

namespace Ui {
class LogWindow;
}

class LogWindow : public QDialog
{
    Q_OBJECT

public:
    explicit LogWindow(QWidget *parent = nullptr);
    ~LogWindow();

    void appendLine(const QString &line);
    void appendTrapLine(const QString &line);
    void setPaused(bool paused);
    void setLoggingEnabled(bool enabled);
    void setMeasurementLoggingEnabled(bool enabled);
    void setMeasurementColumns(const QStringList &names);
    void appendMeasurementRow(const QStringList &cells);

signals:
    void measurementLoggingToggled(bool on);

private:
    static constexpr int MaxLines = 1000;
    static constexpr int MaxDataRows = 500;

    Ui::LogWindow *ui;
    bool m_loggingEnable = false;
    bool m_paused = false;
    DailyLogFile m_eventsFile{"events", "log"};

    void writeLine(const QString &line);
    void growColumnsFor(const QStringList &texts);

private slots:
    void onOpenLogsDirClicked();
    void onClearClicked();
    void onToggleLoggingClicked(bool checked);
    void onToggleDataClicked(bool checked);
};

#endif