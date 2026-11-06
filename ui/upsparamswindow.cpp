#include "upsparamswindow.h"
#include "ui_upsparamswindow.h"

UpsParamsWindow::UpsParamsWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::UpsParamsWindow)
{
    ui->setupUi(this);
    QObject::connect(ui->cellsPerBatterySpin, qOverload<int>(&QSpinBox::valueChanged),
                     this, &UpsParamsWindow::updateSummary);
    QObject::connect(ui->batteryCountSpin, qOverload<int>(&QSpinBox::valueChanged),
                     this, &UpsParamsWindow::updateSummary);
    updateSummary();
}

UpsParamsWindow::~UpsParamsWindow()
{
    delete ui;
}

void UpsParamsWindow::setBatteryConfig(const UpsBatteryConfig &config){
    ui->cellsPerBatterySpin->setValue(config.cellsPerBattery);
    ui->batteryCountSpin->setValue(config.batteryCount);
}

UpsBatteryConfig UpsParamsWindow::batteryConfig() const{
    UpsBatteryConfig config;
    config.cellsPerBattery = ui->cellsPerBatterySpin->value();
    config.batteryCount = ui->batteryCountSpin->value();
    return config;
}

void UpsParamsWindow::updateSummary(){
    const UpsBatteryConfig config = batteryConfig();
    if (config.batteryCount == 0) {
        ui->summaryLabel->setText("Количество АКБ не задано — пересчёт напряжения выключен");
        return;
    }
    ui->summaryLabel->setText(QString("Номинальное напряжение батареи: %1 В (%2 элементов)")
                                  .arg(config.nominalVoltage())
                                  .arg(config.totalCells()));
}
