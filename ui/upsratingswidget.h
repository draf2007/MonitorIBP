#ifndef UPSRATINGSWIDGET_H
#define UPSRATINGSWIDGET_H

#include <QWidget>
#include <QLabel>
#include "core/upsdatastruct.h"

class UpsRatingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit UpsRatingsWidget(QWidget *parent = nullptr) : QWidget(parent) {}

    virtual void setRatings(const UpsNominalSpecifications &ratings, const UpsBatteryConfig &battery) = 0;

protected:
    static void setRow(QLabel *title, QLabel *value, double number, const QString &unit)
    {
        const bool visible = (number >= 0);
        title->setVisible(visible);
        value->setVisible(visible);
        value->setText(QString("%1 %2").arg(number).arg(unit));
    }

    static void setBatteryRow(QLabel *title, QLabel *value,
                              const UpsNominalSpecifications &ratings, const UpsBatteryConfig &battery)
    {
        const bool fromUps = (ratings.nominalBatteryVoltage >= 0);
        const bool hasConfig = (battery.batteryCount > 0);
        title->setVisible(fromUps || hasConfig);
        value->setVisible(fromUps || hasConfig);

        if (!hasConfig) {
            value->setText(QString("%1 В").arg(ratings.nominalBatteryVoltage));
            return;
        }
        const QString config = QString("%1 АКБ × %2 эл.").arg(battery.batteryCount).arg(battery.cellsPerBattery);
        if (battery.countFromUps) {
            value->setText(QString("%1 В (%2, по данным ИБП)").arg(fromUps ? ratings.nominalBatteryVoltage : battery.nominalVoltage()).arg(config));
        } else if (fromUps) {
            value->setText(QString("%1 В (%2)").arg(ratings.nominalBatteryVoltage).arg(config));
        } else {
            value->setText(QString("%1 В (%2, из параметров ИБП)").arg(battery.nominalVoltage()).arg(config));
        }
    }
};

#endif