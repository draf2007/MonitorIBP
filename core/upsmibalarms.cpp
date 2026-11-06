#include "upsmibalarms.h"

QString upsMibAlarmName(int alarm)
{
    switch (alarm) {
    case AlarmBatteryBad:           return "батарею нужно заменить";
    case AlarmOnBattery:            return "работа от батареи";
    case AlarmLowBattery:           return "низкий заряд батареи";
    case AlarmDepletedBattery:      return "батарея разряжена";
    case AlarmTempBad:              return "перегрев";
    case AlarmInputBad:             return "сеть вне допуска";
    case AlarmOutputBad:            return "выход вне допуска";
    case AlarmOutputOverload:       return "перегрузка";
    case AlarmOnBypass:             return "байпас";
    case AlarmBypassBad:            return "байпас вне допуска";
    case AlarmOutputOffAsRequested: return "выход выключен по команде";
    case AlarmUpsOffAsRequested:    return "выключение по команде";
    case AlarmChargerFailed:        return "неисправность зарядного";
    case AlarmUpsOutputOff:         return "выход выключен";
    case AlarmUpsSystemOff:         return "ИБП выключен";
    case AlarmFanFailure:           return "неисправность вентилятора";
    case AlarmFuseFailure:          return "неисправность предохранителя";
    case AlarmGeneralFault:         return "общая неисправность";
    case AlarmDiagnosticTestFailed: return "тест не пройден";
    case AlarmCommunicationsLost:   return "нет связи с ИБП";
    case AlarmAwaitingPower:        return "ожидание сети";
    case AlarmShutdownPending:      return "скоро выключение";
    case AlarmShutdownImminent:     return "выключение немедленно";
    case AlarmTestInProgress:       return "идёт тест";
    }
    return QString("тревога %1").arg(alarm);
}

bool upsMibAlarmIsFault(int alarm)
{
    switch (alarm) {
    case AlarmBatteryBad:
    case AlarmLowBattery:
    case AlarmDepletedBattery:
    case AlarmTempBad:
    case AlarmOutputBad:
    case AlarmOutputOverload:
    case AlarmBypassBad:
    case AlarmChargerFailed:
    case AlarmFanFailure:
    case AlarmFuseFailure:
    case AlarmGeneralFault:
    case AlarmDiagnosticTestFailed:
    case AlarmCommunicationsLost:
    case AlarmShutdownImminent:
        return true;
    default:
        return false;
    }
}

int upsMibAlarmFromOid(const QString &oid)
{
    static const QString prefix = "1.3.6.1.2.1.33.1.6.3.";
    QString text = oid.startsWith('.') ? oid.mid(1) : oid;
    if (!text.startsWith(prefix)) {
        return 0;
    }
    return text.mid(prefix.size()).toInt();
}