#ifndef COMIDENTWIDGET_H
#define COMIDENTWIDGET_H

#include "ui/upsidentwidget.h"

namespace Ui {
class ComIdentWidget;
}

class ComIdentWidget : public UpsIdentWidget
{
    Q_OBJECT

public:
    explicit ComIdentWidget(QWidget *parent = nullptr);
    ~ComIdentWidget() override;

    void setIdentification(const UpsIdentification &info) override;
    void setPortName(const QString &port);

    void setProtocol(bool extended);
    void setPtrtInfo(const UpsPtrtInfo &info);

private:
    Ui::ComIdentWidget *ui;
};

#endif