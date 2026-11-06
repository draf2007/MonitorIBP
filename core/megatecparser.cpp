#include "megatecparser.h"
#include <QString>
#include <QStringList>
#include <QDebug>

MegatecParser::MegatecParser() {}
MegatecParser::~MegatecParser() {}

static bool firstFieldsAreNumbers(const QStringList &parts, int count)
{
    if (parts.size() < count) {
        return false;
    }
    for (int i = 0; i < count; ++i) {
        bool ok = false;
        parts[i].toDouble(&ok);
        if (!ok) {
            qDebug() << "Megatec: поле" << i << "не число:" << parts[i];
            return false;
        }
    }
    return true;
}

std::optional<UpsStatus> MegatecParser::parseStatus_Q1(const QByteArray &raw){
    if (raw.isEmpty()) {
        qDebug() << "Пришла пустая строка!";
        return std::nullopt;
    }

    QString messageQ1 = QString::fromLatin1(raw).trimmed();

    if (messageQ1.startsWith('(')) {
        messageQ1.remove(0,1);
    }

    QStringList messageQ1Split = messageQ1.split(" ",Qt::SkipEmptyParts);
    if (messageQ1Split.size()!=8) { return std::nullopt; }

    QString statusBit = messageQ1Split[7];
    if (statusBit.size() != 8) { return std::nullopt; }

    for (const QChar &c : statusBit) {
        if (c != '0' && c != '1') {
            qDebug() << "Q1: недопустимый символ в битах статуса:" << statusBit;
            return std::nullopt;
        }
    }

    if (!firstFieldsAreNumbers(messageQ1Split, 6)) { return std::nullopt; }

    UpsStatus status_Q1;
    status_Q1.inVoltage = messageQ1Split[0].toDouble();
    status_Q1.inVoltageFault = messageQ1Split[1].toDouble();
    status_Q1.outVoltage = messageQ1Split[2].toDouble();
    status_Q1.outLoadPercent = messageQ1Split[3].toInt();
    status_Q1.freq = messageQ1Split[4].toDouble();
    status_Q1.batteryVoltage = messageQ1Split[5].toDouble();
    status_Q1.batteryVoltagePerCell = (status_Q1.batteryVoltage > 0 && status_Q1.batteryVoltage < 3.0);
    status_Q1.temperature = messageQ1Split[6].toDouble();

    status_Q1.immediateFault = statusBit[0].digitValue();
    status_Q1.lowBatteryVoltage = statusBit[1].digitValue();
    status_Q1.bypassBoost = statusBit[2].digitValue();
    status_Q1.upsFailed = statusBit[3].digitValue();
    status_Q1.isStandbyType = statusBit[4].digitValue();
    status_Q1.testInProgress = statusBit[5].digitValue();
    status_Q1.shutdownActive = statusBit[6].digitValue();
    status_Q1.beeperOn = statusBit[7].digitValue();
    status_Q1.outputOff = status_Q1.bypassBoost;

    if (status_Q1.upsFailed) {
        status_Q1.alarms << "отказ ИБП";
    }
    if (status_Q1.lowBatteryVoltage) {
        status_Q1.alarms << "низкий заряд батареи";
    }

    return status_Q1;
}

std::optional<UpsIdentification> MegatecParser::parseIdentification_I(const QByteArray &raw){
    if (raw.isEmpty()) {
        qDebug() << "Пришла пустая строка!";
        return std::nullopt;
    }

    QString messageI = QString::fromLatin1(raw).trimmed();

    if (messageI.startsWith('#')) {
        messageI.remove(0,1);
    }

    if (messageI.size() != 35) {
        qDebug() << "Длинна строки не соответствует протоколу!";
        return std::nullopt;
    }

    UpsIdentification identification_I;
    identification_I.manufacturer = messageI.mid(0,15);
    identification_I.model = messageI.mid(15,12);
    identification_I.fwVersion = messageI.mid(27);

    return identification_I;
}

std::optional<UpsNominalSpecifications> MegatecParser::parseNominalSpecifications_F(const QByteArray &raw){
    if (raw.isEmpty()) {
        qDebug() << "Пришла пустая строка!";
        return std::nullopt;
    }

    QString messageF = QString::fromLatin1(raw).trimmed();

    if (messageF.startsWith('#')) {
        messageF.remove(0,1);
    }
    QStringList messageF_Split = messageF.split(" ",Qt::SkipEmptyParts);
    if (messageF_Split.size()!=4) { return std::nullopt; }

    if (!firstFieldsAreNumbers(messageF_Split, 4)) { return std::nullopt; }

    UpsNominalSpecifications nominal_F;
    nominal_F.nominalVoltage = messageF_Split[0].toDouble();
    nominal_F.nominalCurrent = messageF_Split[1].toInt();
    nominal_F.nominalBatteryVoltage = messageF_Split[2].toDouble();
    nominal_F.nominalFreq = messageF_Split[3].toDouble();
    return nominal_F;
}

std::optional<UpsBatteryLevel> MegatecParser::parseBatteryLevel_BL(const QByteArray &raw){
    if (raw.isEmpty()) {
        qDebug() << "Пришла пустая строка!";
        return std::nullopt;
    }

    QString messageBL = QString::fromLatin1(raw).trimmed();

    if (messageBL.startsWith('B')) {
        messageBL.remove(0,2);
        bool ok = false;
        messageBL.toInt(&ok);
        if (!ok) {
            qDebug() << "BL: не удалось распознать уровень заряда";
            return std::nullopt;
        }
        UpsBatteryLevel batteryLevel_BL;
        batteryLevel_BL.batteryLevel=messageBL;
        return batteryLevel_BL;
    }
    return std::nullopt;
}

std::optional<QChar> MegatecParser::parseMode_GMOD(const QByteArray &raw){
    if (raw.isEmpty()) {
        qDebug() << "Пришла пустая строка!";
        return std::nullopt;
    }

    const QString message = QString::fromLatin1(raw).trimmed();
    static const QString knownModes = "PSYLBTFECDX";
    if (message.size() != 2 || message[0] != '(' || !knownModes.contains(message[1])) {
        return std::nullopt;
    }
    return message[1];
}

bool MegatecParser::parseFirmware_SVFW(const QByteArray &raw, UpsPtrtInfo &info){
    if (raw.isEmpty()) {
        qDebug() << "Пришла пустая строка!";
        return false;
    }

    QString message = QString::fromLatin1(raw).trimmed();
    message.remove('(');
    const QStringList parts = message.split(' ', Qt::SkipEmptyParts);
    if (parts.size() != 2) {
        return false;
    }
    info.fwVersion = parts[0];
    info.fwDate = QDate::fromString(parts[1], "yyyyMMdd");
    return true;
}

bool MegatecParser::parseBattery_GBAT(const QByteArray &raw, UpsPtrtInfo &info){
    if (raw.isEmpty()) {
        qDebug() << "Пришла пустая строка!";
        return false;
    }

    QString message = QString::fromLatin1(raw).trimmed();
    if (!message.startsWith('(')) {
        return false;
    }
    message.remove(0, 1);
    const QStringList parts = message.split(' ', Qt::SkipEmptyParts);
    if (parts.size() < 3) {
        return false;
    }
    bool ok = false;
    const int count = parts[2].toInt(&ok);
    if (!ok || count <= 0) {
        return false;
    }
    info.batteryCount = count;
    if (parts.size() >= 5) {
        info.batteryEndVoltage = parts[3].toDouble();
        info.batteryAlarmVoltage = parts[4].toDouble();
    }
    return true;
}

static QStringList ptrtFields(const QByteArray &raw)
{
    QString message = QString::fromLatin1(raw).trimmed();
    if (!message.startsWith('(')) {
        return QStringList();
    }
    message.remove(0, 1);
    return message.split(' ', Qt::SkipEmptyParts);
}

bool MegatecParser::parseBattery_GBAT(const QByteArray &raw, UpsPtrtStatus &status){
    const QStringList parts = ptrtFields(raw);
    if (!firstFieldsAreNumbers(parts, 2)) { return false; }
    status.batteryCurrent = parts[1].toDouble();
    return true;
}

bool MegatecParser::parseOutput_GOP(const QByteArray &raw, UpsPtrtStatus &status){
    const QStringList parts = ptrtFields(raw);
    if (!firstFieldsAreNumbers(parts, 6)) { return false; }
    status.outCurrent = parts[2].toDouble();
    status.outPowerW = parts[3].toDouble();
    status.outPowerVA = parts[4].toDouble();
    status.outLoadPercent = parts[5].toInt();
    return true;
}

bool MegatecParser::parseTemperature_GTMP(const QByteArray &raw, UpsPtrtStatus &status){
    const QStringList parts = ptrtFields(raw);
    if (!firstFieldsAreNumbers(parts, 3)) { return false; }
    status.tempPfc = parts[0].toDouble();
    status.tempInverter = parts[1].toDouble();
    status.tempAmbient = parts[2].toDouble();
    return true;
}

bool MegatecParser::parseWarnings_GWS(const QByteArray &raw, UpsPtrtStatus &status){
    const QStringList parts = ptrtFields(raw);
    if (parts.size() < 3 || parts[1].size() != 16 || parts[2].size() != 16) {
        return false;
    }
    bool ok1 = false, ok2 = false, ok3 = false;
    const int code = parts[0].toInt(&ok1);
    const quint16 word1 = parts[1].toUShort(&ok2, 2);
    const quint16 word2 = parts[2].toUShort(&ok3, 2);
    if (!ok1 || !ok2 || !ok3) {
        return false;
    }
    status.faultCode = code;
    status.alarms1 = word1;
    status.alarms2 = word2;
    return true;
}

QString MegatecParser::ptrtModeName(QChar mode){
    switch (mode.toLatin1()) {
    case 'P': return "Включение";
    case 'S': return "Ожидание";
    case 'Y': return "Байпас";
    case 'L': return "Сеть";
    case 'B': return "Батарея";
    case 'T': return "Тест батареи";
    case 'F': return "Авария";
    case 'E': return "ECO";
    case 'C': return "Стабилизатор частоты";
    case 'D': return "Выключен";
    case 'X': return "Тест";
    }
    return QString("Режим %1").arg(mode);
}

static QString ptrtFaultText(int code)
{
    switch (code) {
    case 1:  return "сбой плавного пуска шины";
    case 2:  return "перенапряжение шины";
    case 3:  return "низкое напряжение шины";
    case 7:  return "перегрев";
    case 8:  return "КЗ реле батареи";
    case 9:  return "медленный подъём шины";
    case 10: return "КЗ шины";
    case 17: return "сбой пуска инвертора";
    case 18: return "перенапряжение инвертора";
    case 19: return "низкое напряжение инвертора";
    case 20: return "КЗ инвертора";
    case 26: return "защита от обратной мощности";
    case 33: return "обрыв реле инвертора";
    case 34: return "КЗ реле инвертора";
    case 35: return "обрыв реле байпаса";
    case 36: return "КЗ реле байпаса";
    case 37: return "перепутаны вход и выход";
    case 39: return "КЗ зарядного";
    case 66: return "перегрузка";
    case 67: return "перезаряд или переполюсовка батареи";
    case 68: return "неизвестная модель";
    case 72: return "перегрузка зарядного по току";
    case 73: return "нет загрузчика";
    case 81: return "неизвестное количество АКБ";
    case 82: return "количество АКБ не совпадает";
    }
    return QString("неизвестная авария");
}

QStringList MegatecParser::ptrtAlarmTexts(const UpsPtrtStatus &status){
    QStringList texts;
    if (status.faultCode != 0) {
        texts << QString("%1: %2").arg(status.faultCode).arg(ptrtFaultText(status.faultCode));
    }
    static const QList<QPair<int, QString>> word1 = {
                                                      {13, "сбой EEPROM"}, {12, "сгорел сетевой предохранитель"}, {11, "отказ вентилятора"},
                                                      {9, "перегрев"}, {8, "отказ зарядного"}, {7, "высокое напряжение батареи"},
                                                      {1, "батарея не подключена"}, {0, "низкое напряжение батареи"}};
    static const QList<QPair<int, QString>> word2 = {
                                                      {7, "включён сервисный байпас"}, {6, "аварийное отключение (EPO)"},
                                                      {5, "байпас заблокирован после 3 перегрузок"}, {4, "перегрузка"}};
    for (const auto &bit : word1) {
        if (status.alarms1 & (1u << bit.first)) {
            texts << bit.second;
        }
    }
    for (const auto &bit : word2) {
        if (status.alarms2 & (1u << bit.first)) {
            texts << bit.second;
        }
    }
    return texts;
}