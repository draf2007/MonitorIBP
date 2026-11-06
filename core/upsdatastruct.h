#ifndef UPSDATASTRUCT_H
#define UPSDATASTRUCT_H
#include <QString>
#include <QStringList>
#include <QDate>

enum class UpsTestResult {
    Unknown     = 0,
    Passed      = 1,
    Warning     = 2,
    Error       = 3,
    Aborted     = 4,
    InProgress  = 5,
    NoTest      = 6
};

enum class UpsTestKind {
    Unknown,
    Quick,
    Deep,
    Other
};

struct UpsStatus
{
    double inVoltage;
    double inVoltageFault;
    double outVoltage;
    int outLoadPercent;
    double freq;
    double batteryVoltage;
    double temperature;

    bool immediateFault;
    bool lowBatteryVoltage;
    bool bypassBoost;
    bool upsFailed;
    bool isStandbyType;
    bool testInProgress;
    bool shutdownActive;
    bool beeperOn;

    bool outputOff = false;
    bool mainsLost = false;
    int inputLineBads = -1;
    bool batteryVoltagePerCell = false;
    int secondsOnBattery = -1;
    int shutdownCountdownSec = -1;
    int startupCountdownSec = -1;
    UpsTestResult testResult = UpsTestResult::Unknown;
    UpsTestKind testKind = UpsTestKind::Unknown;
    QString mode;
    QStringList alarms;
};

inline QString upsModeText(const UpsStatus &status)
{
    if (!status.mode.isEmpty()) return status.mode;
    if (status.testInProgress) return "Тест";
    if (status.outputOff || status.bypassBoost) return "Байпас";
    if (status.immediateFault) return "Батарея";
    return "Сеть";
}

struct UpsIdentification
{
    QString manufacturer;
    QString model;
    QString fwVersion;

    QString agentFwVersion;
    QString sysName;
    QString sysLocation;
};

struct UpsNominalSpecifications
{
    double nominalVoltage;
    int nominalCurrent;
    double nominalBatteryVoltage;
    double nominalFreq;
    double nominalPowerW = -1;
};

struct UpsBatteryLevel
{
    QString batteryLevel;
};

struct UpsBatteryConfig {
    int cellsPerBattery = 6;
    int batteryCount = 0;
    bool countFromUps = false;

    int totalCells() const { return cellsPerBattery * batteryCount; }
    double nominalVoltage() const { return totalCells() * 2.0; }
};

struct UpsPtrtInfo
{
    QString fwVersion;
    QDate fwDate;
    int batteryCount = -1;
    double batteryEndVoltage = -1;
    double batteryAlarmVoltage = -1;
};

struct UpsPtrtStatus
{
    QChar mode;
    double outPowerW = -1;
    double outPowerVA = -1;
    double outCurrent = -1;
    int outLoadPercent = -1;
    double tempPfc = -1;
    double tempInverter = -1;
    double tempAmbient = -1;
    double batteryCurrent = -1;
    int faultCode = 0;
    quint16 alarms1 = 0;
    quint16 alarms2 = 0;
};

#endif
