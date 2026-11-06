#ifndef SNMPCARDV1_H
#define SNMPCARDV1_H

#include "snmpcarddriver.h"

class SnmpCardV1 : public SnmpCardDriver
{
public:
    QStringList identificationOids() const override;
    void parseIdentification(const SnmpValues &values,
                             UpsIdentification &identification,
                             UpsNominalSpecifications &ratings) const override;

    QStringList statusOids() const override;
    QStringList extraStatusOids(const SnmpValues &statusValues) const override;
    SnmpPollResult parseStatus(const SnmpValues &statusValues,
                               const SnmpValues &extraValues) const override;

    SnmpSetRequests commandRequests(UpsCommand command,
                                    const UpsShutdownParams &params,
                                    const UpsStatus &currentStatus) const override;
};

#endif