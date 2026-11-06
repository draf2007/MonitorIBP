#include "snmpcardv1.h"
#include "upsmibalarms.h"

namespace {

const QString Ups = ".1.3.6.1.2.1.33.1.";

enum IdentIndex {
    SysName, SysLocation,
    IdentManufacturer, IdentModel, IdentUpsSoftware, IdentAgentSoftware,
    ConfigInputVoltage, ConfigInputFreq, ConfigOutputVoltage, ConfigOutputFreq, ConfigOutputPower
};

enum StatusIndex {
    OutputSource, BatteryStatus, SecondsOnBattery, ChargeRemaining, BatteryVoltage,
    InputLineBads, InputFrequency, InputVoltage, OutputVoltage, OutputLoad,
    AlarmsPresent, TestId, TestResult, ShutdownAfter, StartupAfter, AudibleStatus,
    BatteryTemperature, MinutesRemaining
};

const int MaxAlarmRows = 16;

QString cleanText(const SnmpValue &value)
{
    QString text = value.text.trimmed();
    if (text.size() > 1 && text.count(text.at(0)) == text.size()) {
        return QString();
    }
    return text;
}

double tenths(const SnmpValue &value)
{
    return value.isMissing() ? -1 : value.toInt() / 10.0;
}

int countdown(const SnmpValue &value)
{
    const long seconds = value.toInt();
    return seconds > 0 ? int(seconds) : -1;
}

UpsTestKind testKindFromOid(const QString &oid)
{
    if (oid == Ups + "7.7.4") return UpsTestKind::Quick;
    if (oid == Ups + "7.7.5") return UpsTestKind::Deep;
    if (oid == Ups + "7.7.3") return UpsTestKind::Other;
    return UpsTestKind::Unknown;
}

}

QStringList SnmpCardV1::identificationOids() const
{
    return {
        ".1.3.6.1.2.1.1.5.0",
        ".1.3.6.1.2.1.1.6.0",
        Ups + "1.1.0",
        Ups + "1.2.0",
        Ups + "1.3.0",
        Ups + "1.4.0",
        Ups + "9.1.0",
        Ups + "9.2.0",
        Ups + "9.3.0",
        Ups + "9.4.0",
        Ups + "9.6.0",
    };
}

void SnmpCardV1::parseIdentification(const SnmpValues &values,
                                     UpsIdentification &identification,
                                     UpsNominalSpecifications &ratings) const
{
    identification.sysName = cleanText(values[SysName]);
    identification.sysLocation = cleanText(values[SysLocation]);
    identification.manufacturer = cleanText(values[IdentManufacturer]);
    identification.model = cleanText(values[IdentModel]);
    identification.fwVersion = cleanText(values[IdentUpsSoftware]);
    identification.agentFwVersion = cleanText(values[IdentAgentSoftware]);

    ratings.nominalVoltage = values[ConfigInputVoltage].isMissing()
                                 ? values[ConfigOutputVoltage].toInt()
                                 : values[ConfigInputVoltage].toInt();
    ratings.nominalFreq = values[ConfigInputFreq].isMissing()
                              ? tenths(values[ConfigOutputFreq])
                              : tenths(values[ConfigInputFreq]);
    ratings.nominalPowerW = values[ConfigOutputPower].toInt();
    ratings.nominalCurrent = -1;
    ratings.nominalBatteryVoltage = -1;
}

QStringList SnmpCardV1::statusOids() const
{
    return {
        Ups + "4.1.0",
        Ups + "2.1.0",
        Ups + "2.2.0",
        Ups + "2.4.0",
        Ups + "2.5.0",
        Ups + "3.1.0",
        Ups + "3.3.1.2.1",
        Ups + "3.3.1.3.1",
        Ups + "4.4.1.2.1",
        Ups + "4.4.1.5.1",
        Ups + "6.1.0",
        Ups + "7.1.0",
        Ups + "7.3.0",
        Ups + "8.2.0",
        Ups + "8.3.0",
        Ups + "9.8.0",
        Ups + "2.7.0",
        Ups + "2.3.0",
    };
}

QStringList SnmpCardV1::extraStatusOids(const SnmpValues &statusValues) const
{
    const int count = qBound(0, int(statusValues[AlarmsPresent].toInt(0)), MaxAlarmRows);
    QStringList oids;
    for (int row = 1; row <= count; ++row) {
        oids << Ups + QString("6.2.1.2.%1").arg(row);
    }
    return oids;
}

SnmpPollResult SnmpCardV1::parseStatus(const SnmpValues &statusValues,
                                       const SnmpValues &extraValues) const
{
    QList<int> alarms;
    const QString alarmPrefix = Ups + "6.3.";
    for (const SnmpValue &value : extraValues) {
        if (value.type == SnmpValue::Type::Oid && value.text.startsWith(alarmPrefix)) {
            alarms << value.text.mid(alarmPrefix.size()).toInt();
        }
    }
    auto hasAlarm = [&alarms](std::initializer_list<int> list) {
        for (int alarm : list) {
            if (alarms.contains(alarm)) return true;
        }
        return false;
    };

    const long outputSource = statusValues[OutputSource].toInt();
    const long batteryStatus = statusValues[BatteryStatus].toInt();
    const long testCode = statusValues[TestResult].toInt();

    SnmpPollResult result;
    UpsStatus &status = result.status;

    status.inVoltage = statusValues[InputVoltage].toInt();
    status.inVoltageFault = 0;
    status.outVoltage = statusValues[OutputVoltage].toInt();
    status.outLoadPercent = statusValues[OutputLoad].toInt();
    status.freq = tenths(statusValues[InputFrequency]);
    status.batteryVoltage = tenths(statusValues[BatteryVoltage]);
    status.batteryVoltagePerCell = false;
    status.temperature = tenths(statusValues[BatteryTemperature]);
    status.mainsLost = (statusValues[InputLineBads].toInt() == 1);
    status.immediateFault = (outputSource == 5) || hasAlarm({AlarmOnBattery}) || status.mainsLost;
    status.bypassBoost = (outputSource == 4 || outputSource == 6 || outputSource == 7);
    status.outputOff = (outputSource == 2 || outputSource == 4) || hasAlarm({AlarmOnBypass, AlarmOutputOffAsRequested, AlarmUpsOutputOff, AlarmUpsSystemOff});
    status.lowBatteryVoltage = (batteryStatus == 3 || batteryStatus == 4) || hasAlarm({AlarmLowBattery, AlarmDepletedBattery});
    status.testInProgress = (testCode == 5) || hasAlarm({AlarmTestInProgress});
    status.shutdownActive = (statusValues[ShutdownAfter].toInt() > 0) || hasAlarm({AlarmUpsOffAsRequested, AlarmShutdownPending, AlarmShutdownImminent});
    status.beeperOn = (statusValues[AudibleStatus].toInt() == 2);
    status.upsFailed = hasAlarm({AlarmBatteryBad, AlarmTempBad, AlarmOutputBad, AlarmOutputOverload,
                                 AlarmBypassBad, AlarmChargerFailed, AlarmFanFailure, AlarmFuseFailure,
                                 AlarmGeneralFault, AlarmDiagnosticTestFailed, AlarmCommunicationsLost})
                       || testCode == 3;
    status.isStandbyType = false;

    status.inputLineBads = -1;
    status.secondsOnBattery = statusValues[SecondsOnBattery].toInt();
    status.shutdownCountdownSec = countdown(statusValues[ShutdownAfter]);
    status.startupCountdownSec = countdown(statusValues[StartupAfter]);
    status.testResult = (testCode >= 1 && testCode <= 6) ? static_cast<UpsTestResult>(testCode)
                                                         : UpsTestResult::Unknown;
    status.testKind = testKindFromOid(statusValues[TestId].text);

    result.batteryLevel.batteryLevel = QString::number(statusValues[ChargeRemaining].toInt());

    for (int alarm : alarms) {
        result.activeAlarms << upsMibAlarmName(alarm);
        if (upsMibAlarmIsFault(alarm)) {
            status.alarms << upsMibAlarmName(alarm);
        }
    }
    return result;
}

SnmpSetRequests SnmpCardV1::commandRequests(UpsCommand command,
                                            const UpsShutdownParams &params,
                                            const UpsStatus &currentStatus) const
{
    const QString testId = Ups + "7.1.0";
    const QString shutdownAfter = Ups + "8.2.0";
    const QString startupAfter = Ups + "8.3.0";
    const QString audibleStatus = Ups + "9.8.0";

    switch (command) {
    case UpsCommand::QuickTest:
        return {setOid(testId, Ups + "7.7.4")};
    case UpsCommand::DeepTest:
        return {setOid(testId, Ups + "7.7.5")};
    case UpsCommand::CancelTest:
        return {setOid(testId, Ups + "7.7.2")};
    case UpsCommand::Shutdown:
        return {setInteger(shutdownAfter, params.shutdownDelaySec)};
    case UpsCommand::CancelShutdown:
        return {setInteger(shutdownAfter, -1), setInteger(startupAfter, -1)};
    case UpsCommand::TurnOn:
        return {setInteger(startupAfter, 0)};
    case UpsCommand::ToggleBeeper:
        return {setInteger(audibleStatus, currentStatus.beeperOn ? 3 : 2)};
    case UpsCommand::TimedTest:
        return {};
    }
    return {};
}
