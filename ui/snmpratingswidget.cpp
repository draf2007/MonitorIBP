#include "ui/snmpratingswidget.h"
#include "ui_snmpratingswidget.h"

SnmpRatingsWidget::SnmpRatingsWidget(QWidget *parent)
    : UpsRatingsWidget(parent)
    , ui(new Ui::SnmpRatingsWidget)
{
    ui->setupUi(this);
}

SnmpRatingsWidget::~SnmpRatingsWidget()
{
    delete ui;
}

void SnmpRatingsWidget::setRatings(const UpsNominalSpecifications &ratings, const UpsBatteryConfig &battery)
{
    setRow(ui->titleVoltage, ui->labelVoltage, ratings.nominalVoltage, "В");
    setRow(ui->titleFreq, ui->labelFreq, ratings.nominalFreq, "Гц");
    setRow(ui->titlePower, ui->labelPower, ratings.nominalPowerW, "Вт");
    setBatteryRow(ui->titleBattery, ui->labelBattery, ratings, battery);
}