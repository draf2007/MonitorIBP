#ifndef UPSPARAMSWINDOW_H
#define UPSPARAMSWINDOW_H

#include <QDialog>
#include "core/upsdatastruct.h"

namespace Ui {
class UpsParamsWindow;
}

class UpsParamsWindow : public QDialog
{
    Q_OBJECT

public:
    explicit UpsParamsWindow(QWidget *parent = nullptr);
    ~UpsParamsWindow();
    void setBatteryConfig(const UpsBatteryConfig &config);
    UpsBatteryConfig batteryConfig() const;

private:
    Ui::UpsParamsWindow *ui;
    void updateSummary();
};

#endif
