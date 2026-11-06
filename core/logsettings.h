#ifndef LOGSETTINGS_H
#define LOGSETTINGS_H

#include <QSettings>

struct LogSettings
{
    bool eventsAuto = true;
    bool dataAuto = false;
    int comIntervalSec = 1;
    int snmpIntervalSec = 6;
    int keepDays = 30;

    void load(const QSettings &settings)
    {
        eventsAuto      = settings.value("log/eventsAuto", eventsAuto).toBool();
        dataAuto        = settings.value("log/dataAuto", dataAuto).toBool();
        comIntervalSec  = settings.value("log/comIntervalSec", comIntervalSec).toInt();
        snmpIntervalSec = settings.value("log/snmpIntervalSec", snmpIntervalSec).toInt();
        keepDays        = settings.value("log/keepDays", keepDays).toInt();
    }

    void save(QSettings &settings) const
    {
        settings.setValue("log/eventsAuto", eventsAuto);
        settings.setValue("log/dataAuto", dataAuto);
        settings.setValue("log/comIntervalSec", comIntervalSec);
        settings.setValue("log/snmpIntervalSec", snmpIntervalSec);
        settings.setValue("log/keepDays", keepDays);
    }
};

#endif