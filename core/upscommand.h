#ifndef UPSCOMMAND_H
#define UPSCOMMAND_H

#include <QString>

enum class UpsCommand {
    QuickTest,
    DeepTest,
    CancelTest,
    Shutdown,
    CancelShutdown,
    TurnOn,
    ToggleBeeper,
    TimedTest
};

struct UpsShutdownParams {
    int shutdownDelaySec = 60;
    int restartDelaySec = -1;
    int testMinutes = 1;
};

inline QString upsCommandName(UpsCommand command)
{
    switch (command) {
    case UpsCommand::QuickTest:      return "быстрый тест";
    case UpsCommand::DeepTest:       return "тест до разряда батарей";
    case UpsCommand::CancelTest:     return "отмена теста";
    case UpsCommand::Shutdown:       return "выключение";
    case UpsCommand::CancelShutdown: return "отмена выключения";
    case UpsCommand::TurnOn:         return "включение";
    case UpsCommand::ToggleBeeper:   return "пищалка вкл/выкл";
    case UpsCommand::TimedTest:      return "тест на время";
    }
    return QString();
}

#endif