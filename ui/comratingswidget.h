#ifndef COMRATINGSWIDGET_H
#define COMRATINGSWIDGET_H

#include "ui/upsratingswidget.h"

namespace Ui {
class ComRatingsWidget;
}

class ComRatingsWidget : public UpsRatingsWidget
{
    Q_OBJECT

public:
    explicit ComRatingsWidget(QWidget *parent = nullptr);
    ~ComRatingsWidget() override;

    void setRatings(const UpsNominalSpecifications &ratings, const UpsBatteryConfig &battery) override;

private:
    Ui::ComRatingsWidget *ui;
};

#endif