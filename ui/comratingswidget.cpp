#include "ui/comratingswidget.h"
#include "ui_comratingswidget.h"

ComRatingsWidget::ComRatingsWidget(QWidget *parent)
    : UpsRatingsWidget(parent)
    , ui(new Ui::ComRatingsWidget)
{
    ui->setupUi(this);
}

ComRatingsWidget::~ComRatingsWidget()
{
    delete ui;
}

void ComRatingsWidget::setRatings(const UpsNominalSpecifications &ratings, const UpsBatteryConfig &battery)
{
    setRow(ui->titleVoltage, ui->labelVoltage, ratings.nominalVoltage, "В");
    setRow(ui->titleCurrent, ui->labelCurrent, ratings.nominalCurrent, "А");
    setRow(ui->titleFreq, ui->labelFreq, ratings.nominalFreq, "Гц");
    setBatteryRow(ui->titleBattery, ui->labelBattery, ratings, battery);

    const double power = (ratings.nominalVoltage > 0 && ratings.nominalCurrent > 0)
                             ? ratings.nominalVoltage * ratings.nominalCurrent : -1;
    setRow(ui->titlePower, ui->labelPower, power, "ВА (оценка U × I)");
}