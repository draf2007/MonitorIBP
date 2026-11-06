#ifndef LOGSETTINGSWINDOW_H
#define LOGSETTINGSWINDOW_H

#include <QDialog>
#include "core/logsettings.h"

namespace Ui {
class LogSettingsWindow;
}

class LogSettingsWindow : public QDialog
{
    Q_OBJECT

public:
    explicit LogSettingsWindow(QWidget *parent = nullptr);
    ~LogSettingsWindow();

    void setSettings(const LogSettings &settings);
    LogSettings settings() const;

private:
    Ui::LogSettingsWindow *ui;
};

#endif