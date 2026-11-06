#include "ptrtstatuswidget.h"
#include "ui_ptrtstatuswidget.h"

PtrtStatusWidget::PtrtStatusWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PtrtStatusWidget)
{
    ui->setupUi(this);
    clear();
}

PtrtStatusWidget::~PtrtStatusWidget()
{
    delete ui;
}

void PtrtStatusWidget::setStatus(const UpsPtrtStatus &status)
{
    if (status.outPowerW >= 0) {
        ui->labelOutput->setText(QString("%1 Вт / %2 ВА, %3 А, %4 %")
                                     .arg(status.outPowerW)
                                     .arg(status.outPowerVA)
                                     .arg(status.outCurrent, 0, 'f', 1)
                                     .arg(status.outLoadPercent));
    }
    if (status.batteryCurrent >= 0) {
        ui->labelBatteryCurrent->setText(QString("%1 А").arg(status.batteryCurrent, 0, 'f', 2));
    }
    if (status.tempPfc >= 0) {
        ui->labelTemperature->setText(QString("PFC %1 °C, инв. %2 °C, среда %3 °C")
                                          .arg(status.tempPfc)
                                          .arg(status.tempInverter)
                                          .arg(status.tempAmbient));
    }
}

void PtrtStatusWidget::clear()
{
    ui->labelOutput->setText("—");
    ui->labelBatteryCurrent->setText("—");
    ui->labelTemperature->setText("—");
}
