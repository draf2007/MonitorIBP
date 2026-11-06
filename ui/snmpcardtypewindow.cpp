#include "snmpcardtypewindow.h"
#include "ui_snmpcardtypewindow.h"
#include "snmpcardregistry.h"

SnmpCardTypeWindow::SnmpCardTypeWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SnmpCardTypeWindow)
{
    ui->setupUi(this);

    for (const SnmpCardInfo &card : snmpCards()) {
        ui->cardTypeCombo->addItem(card.name, card.id);
    }

    QObject::connect(ui->cardTypeCombo, &QComboBox::currentIndexChanged, this, &SnmpCardTypeWindow::onCardChanged);
    onCardChanged(ui->cardTypeCombo->currentIndex());
}

SnmpCardTypeWindow::~SnmpCardTypeWindow()
{
    delete ui;
}

void SnmpCardTypeWindow::setCardType(const QString &cardId){
    int index = ui->cardTypeCombo->findData(findSnmpCard(cardId).id);
    ui->cardTypeCombo->setCurrentIndex(index);
}

QString SnmpCardTypeWindow::cardType() const {
    return ui->cardTypeCombo->currentData().toString();
}

void SnmpCardTypeWindow::onCardChanged(int index){
    Q_UNUSED(index);
    ui->descriptionLabel->setText(findSnmpCard(cardType()).description);
}
