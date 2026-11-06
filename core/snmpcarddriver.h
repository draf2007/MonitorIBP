#ifndef SNMPCARDDRIVER_H
#define SNMPCARDDRIVER_H

#include <QString>
#include <QStringList>
#include <QVector>
#include "upsdatastruct.h"
#include "upscommand.h"

struct SnmpValue{
    enum class Type {
        Missing, Integer, String, Oid
    };

    Type type = Type::Missing;
    long integer = 0;
    QString text;

    bool isMissing() const {return type == Type::Missing;}
    long toInt(long fallback = -1) const {return type == Type::Integer ? integer : fallback;}
};

using SnmpValues = QVector<SnmpValue>;

struct SnmpPollResult {
    UpsStatus status;
    UpsBatteryLevel batteryLevel;
    QStringList activeAlarms;
};

struct SnmpSetRequest
{
    QString oid;
    SnmpValue value;
};

using SnmpSetRequests = QVector<SnmpSetRequest>;

class SnmpCardDriver {
public:
    virtual ~SnmpCardDriver() = default;
    virtual int phaseCount() const { return 1; }
    virtual QStringList identificationOids() const = 0;
    virtual void parseIdentification (const SnmpValues &values, UpsIdentification &identification, UpsNominalSpecifications &ratings) const = 0;
    virtual QStringList statusOids() const = 0;
    virtual QStringList extraStatusOids(const SnmpValues &statusValues) const {
        Q_UNUSED(statusValues);
        return {};
    }
    virtual SnmpPollResult parseStatus(const SnmpValues &statusValues, const SnmpValues &extraValues) const = 0;
    virtual SnmpSetRequests commandRequests(UpsCommand command,
                                            const UpsShutdownParams &params,
                                            const UpsStatus &currentStatus) const
    {
        Q_UNUSED(command);
        Q_UNUSED(params);
        Q_UNUSED(currentStatus);
        return {};
    }

protected:
    static SnmpSetRequest setInteger(const QString &oid, long number)
    {
        SnmpSetRequest request;
        request.oid = oid;
        request.value.type = SnmpValue::Type::Integer;
        request.value.integer = number;
        return request;
    }
    static SnmpSetRequest setOid(const QString &oid, const QString &oidValue)
    {
        SnmpSetRequest request;
        request.oid = oid;
        request.value.type = SnmpValue::Type::Oid;
        request.value.text = oidValue;
        return request;
    }
};

#endif
