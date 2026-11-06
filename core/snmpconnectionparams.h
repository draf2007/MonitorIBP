#ifndef SNMPCONNECTIONPARAMS_H
#define SNMPCONNECTIONPARAMS_H

#include <QString>

enum class SnmpVersion{
    V1,
    V2c,
    V3
};

enum class SnmpSecurityLevel{
    NoAuthNoPriv,
    AuthNoPriv,
    AuthPriv
};

enum class SnmpAuthProtocol{
    MD5,
    SHA
};

enum class SnmpPrivProtocol{
    DES,
    AES
};

struct SnmpConnectionParams{
    QString ip;
    quint16 port = 161;
    QString community;
    QString writeCommunity;

    QString effectiveWriteCommunity() const { return writeCommunity.isEmpty() ? community : writeCommunity; }

    SnmpVersion version = SnmpVersion::V1;

    QString username;
    SnmpSecurityLevel securityLevel = SnmpSecurityLevel::NoAuthNoPriv;
    SnmpAuthProtocol authProtocol = SnmpAuthProtocol::MD5;
    QString authPassword;
    SnmpPrivProtocol privProtocol = SnmpPrivProtocol::DES;
    QString privPassword;
};

#endif
