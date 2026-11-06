#ifndef UPSMIBALARMS_H
#define UPSMIBALARMS_H

#include <QString>

enum Alarm {
    AlarmBatteryBad = 1, AlarmOnBattery = 2, AlarmLowBattery = 3, AlarmDepletedBattery = 4,
    AlarmTempBad = 5, AlarmInputBad = 6, AlarmOutputBad = 7, AlarmOutputOverload = 8,
    AlarmOnBypass = 9, AlarmBypassBad = 10, AlarmOutputOffAsRequested = 11,
    AlarmUpsOffAsRequested = 12, AlarmChargerFailed = 13, AlarmUpsOutputOff = 14,
    AlarmUpsSystemOff = 15, AlarmFanFailure = 16, AlarmFuseFailure = 17,
    AlarmGeneralFault = 18, AlarmDiagnosticTestFailed = 19, AlarmCommunicationsLost = 20,
    AlarmAwaitingPower = 21, AlarmShutdownPending = 22, AlarmShutdownImminent = 23,
    AlarmTestInProgress = 24
};

QString upsMibAlarmName(int alarm);
bool upsMibAlarmIsFault(int alarm);

int upsMibAlarmFromOid(const QString &oid);

#endif
