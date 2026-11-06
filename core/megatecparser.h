#ifndef MEGATECPARSER_H
#define MEGATECPARSER_H

#include <optional>
#include <QByteArray>
#include <QStringList>
#include "upsdatastruct.h"

class MegatecParser
{
public:
    MegatecParser();
    ~MegatecParser();

    static std::optional<UpsStatus> parseStatus_Q1(const QByteArray &raw);
    static std::optional<UpsIdentification> parseIdentification_I(const QByteArray &raw);
    static std::optional<UpsNominalSpecifications> parseNominalSpecifications_F(const QByteArray &raw);
    static std::optional<UpsBatteryLevel> parseBatteryLevel_BL(const QByteArray &raw);
    static std::optional<QChar> parseMode_GMOD(const QByteArray &raw);
    static bool parseFirmware_SVFW(const QByteArray &raw, UpsPtrtInfo &info);
    static bool parseBattery_GBAT(const QByteArray &raw, UpsPtrtInfo &info);
    static bool parseBattery_GBAT(const QByteArray &raw, UpsPtrtStatus &status);
    static bool parseOutput_GOP(const QByteArray &raw, UpsPtrtStatus &status);
    static bool parseTemperature_GTMP(const QByteArray &raw, UpsPtrtStatus &status);
    static bool parseWarnings_GWS(const QByteArray &raw, UpsPtrtStatus &status);
    static QString ptrtModeName(QChar mode);
    static QStringList ptrtAlarmTexts(const UpsPtrtStatus &status);
};

#endif
