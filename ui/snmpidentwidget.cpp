#include "snmpidentwidget.h"
#include "ui_snmpidentwidget.h"

SnmpIdentWidget::SnmpIdentWidget(QWidget *parent)
    : UpsIdentWidget(parent)
    , ui(new Ui::SnmpIdentWidget)
{
    ui->setupUi(this);
}

SnmpIdentWidget::~SnmpIdentWidget()
{
    delete ui;
}

void SnmpIdentWidget::setIdentification(const UpsIdentification &info)
{
    setRow(ui->titleSysName, ui->labelSysName, info.sysName);
    setRow(ui->titleSysLocation, ui->labelSysLocation, info.sysLocation);
    setRow(ui->titleFwVersion, ui->labelFwVersion, info.fwVersion);
    setRow(ui->titleAgentFwVersion, ui->labelAgentFwVersion, info.agentFwVersion);
    setRow(ui->titleManufacturer, ui->labelManufacturer, info.manufacturer);
    setRow(ui->titleModel, ui->labelModel, info.model);
}

void SnmpIdentWidget::setCardName(const QString &name)
{
    ui->labelCardType->setText(name);
}
