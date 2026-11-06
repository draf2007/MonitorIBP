#ifndef SNMPIDENTWIDGET_H
#define SNMPIDENTWIDGET_H

#include "ui/upsidentwidget.h"

namespace Ui {
class SnmpIdentWidget;
}

class SnmpIdentWidget : public UpsIdentWidget
{
    Q_OBJECT
public:
    explicit SnmpIdentWidget(QWidget *parent = nullptr);
    ~SnmpIdentWidget() override;

    void setIdentification(const UpsIdentification &info) override;
    void setCardName(const QString &name);
private:
    Ui::SnmpIdentWidget *ui;
};

#endif
