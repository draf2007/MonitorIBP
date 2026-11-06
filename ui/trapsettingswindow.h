#ifndef TRAPSETTINGSWINDOW_H
#define TRAPSETTINGSWINDOW_H

#include <QDialog>
#include "snmptrapdata.h"

namespace Ui {
class TrapSettingsWindow;
}

class TrapSettingsWindow : public QDialog
{
    Q_OBJECT

public:
    explicit TrapSettingsWindow(QWidget *parent = nullptr);
    ~TrapSettingsWindow();
    void setSettings (const SnmpTrapSettings &settings);
    SnmpTrapSettings settings() const;

public slots:
    void accept() override;

private:
    Ui::TrapSettingsWindow *ui;
};

#endif
