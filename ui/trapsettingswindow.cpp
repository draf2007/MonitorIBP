#include "trapsettingswindow.h"
#include "ui_trapsettingswindow.h"
#include <QHostAddress>
#include <QMessageBox>

TrapSettingsWindow::TrapSettingsWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::TrapSettingsWindow)
{
    ui->setupUi(this);
}

TrapSettingsWindow::~TrapSettingsWindow()
{
    delete ui;
}

void TrapSettingsWindow::setSettings(const SnmpTrapSettings &settings){
    ui->trapPortSpin->setValue(settings.port);
    ui->trapCommunityEdit->setText(settings.community);
    ui->trapAllowedIpEdit->setText(settings.allowedIp);
    ui->trapAutoStartCheck->setChecked(settings.autoStart);
}

SnmpTrapSettings TrapSettingsWindow::settings() const {
    SnmpTrapSettings settings;
    settings.port = static_cast<quint16>(ui->trapPortSpin->value());
    settings.community = ui->trapCommunityEdit->text().trimmed();
    settings.allowedIp = ui->trapAllowedIpEdit->text().trimmed();
    settings.autoStart = ui->trapAutoStartCheck->isChecked();
    return settings;
}

void TrapSettingsWindow::accept(){
    QString ipText = ui->trapAllowedIpEdit->text().trimmed();
    if (!ipText.isEmpty() && QHostAddress(ipText).isNull()) {
        QMessageBox::warning(this,"Некорректный ip адрес", QString("\"%1\" не является корректным IP-адресом").arg(ipText));
        return;
    }
    QDialog::accept();
}
