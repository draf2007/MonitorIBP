#include "logsettingswindow.h"
#include "ui_logsettingswindow.h"
#include "core/dailylogfile.h"

#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDir>
#include <QUrl>

LogSettingsWindow::LogSettingsWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LogSettingsWindow)
{
    ui->setupUi(this);

    ui->logsDirLabel->setText(QDir::toNativeSeparators(DailyLogFile::logsDir()));

    QObject::connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    QObject::connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    QObject::connect(ui->openLogsDirBtn, &QPushButton::clicked, this, [](){
        QDir().mkpath(DailyLogFile::logsDir());
        QDesktopServices::openUrl(QUrl::fromLocalFile(DailyLogFile::logsDir()));
    });
}

LogSettingsWindow::~LogSettingsWindow()
{
    delete ui;
}

void LogSettingsWindow::setSettings(const LogSettings &settings){
    ui->eventsAutoCheck->setChecked(settings.eventsAuto);
    ui->dataAutoCheck->setChecked(settings.dataAuto);
    ui->comIntervalSpin->setValue(settings.comIntervalSec);
    ui->snmpIntervalSpin->setValue(settings.snmpIntervalSec);
    ui->keepDaysSpin->setValue(settings.keepDays);
}

LogSettings LogSettingsWindow::settings() const{
    LogSettings settings;
    settings.eventsAuto      = ui->eventsAutoCheck->isChecked();
    settings.dataAuto        = ui->dataAutoCheck->isChecked();
    settings.comIntervalSec  = ui->comIntervalSpin->value();
    settings.snmpIntervalSec = ui->snmpIntervalSpin->value();
    settings.keepDays        = ui->keepDaysSpin->value();
    return settings;
}