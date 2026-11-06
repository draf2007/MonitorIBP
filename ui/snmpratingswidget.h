#ifndef SNMPRATINGSWIDGET_H
#define SNMPRATINGSWIDGET_H

#include "ui/upsratingswidget.h"

namespace Ui {
class SnmpRatingsWidget;
}

class SnmpRatingsWidget : public UpsRatingsWidget
{
    Q_OBJECT

public:
    explicit SnmpRatingsWidget(QWidget *parent = nullptr);
    ~SnmpRatingsWidget() override;

    void setRatings(const UpsNominalSpecifications &ratings, const UpsBatteryConfig &battery) override;

private:
    Ui::SnmpRatingsWidget *ui;
};

#endif