#include "ui/comidentwidget.h"
#include "ui_comidentwidget.h"

ComIdentWidget::ComIdentWidget(QWidget *parent)
    : UpsIdentWidget(parent)
    , ui(new Ui::ComIdentWidget)
{
    ui->setupUi(this);
    ui->titleProtocol->setVisible(false);
    ui->labelProtocol->setVisible(false);
    ui->titlePtrtFirmware->setVisible(false);
    ui->labelPtrtFirmware->setVisible(false);
}

ComIdentWidget::~ComIdentWidget()
{
    delete ui;
}

void ComIdentWidget::setIdentification(const UpsIdentification &info)
{
    setRow(ui->titleManufacturer, ui->labelManufacturer, info.manufacturer);
    setRow(ui->titleModel, ui->labelModel, info.model);
    setRow(ui->titleFwVersion, ui->labelFwVersion, info.fwVersion);
}

void ComIdentWidget::setPortName(const QString &port)
{
    ui->labelPort->setText(QString("%1, 2400 бод").arg(port));
}

void ComIdentWidget::setProtocol(bool extended)
{
    setRow(ui->titleProtocol, ui->labelProtocol, extended ? QString("Расширенный") : QString("Базовый"));
}

void ComIdentWidget::setPtrtInfo(const UpsPtrtInfo &info)
{
    QString text = info.fwVersion;
    if (info.fwDate.isValid()) {
        text += QString(" от %1").arg(info.fwDate.toString("dd.MM.yyyy"));
    }
    setRow(ui->titlePtrtFirmware, ui->labelPtrtFirmware, text);
}