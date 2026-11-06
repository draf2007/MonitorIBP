#ifndef UPSSCHEMATICMODEL_H
#define UPSSCHEMATICMODEL_H

#include <QObject>
#include <QDateTime>
#include <QVariantMap>
#include "core/upsdatastruct.h"

class UpsSchematicModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap banner  READ banner  NOTIFY changed)
    Q_PROPERTY(QVariantMap mains   READ mains   NOTIFY changed)
    Q_PROPERTY(QVariantMap ups     READ ups     NOTIFY changed)
    Q_PROPERTY(QVariantMap load    READ load    NOTIFY changed)
    Q_PROPERTY(QVariantMap battery READ battery NOTIFY changed)
    Q_PROPERTY(QVariantMap lines   READ lines   NOTIFY changed)
    Q_PROPERTY(bool animationsEnabled READ animationsEnabled NOTIFY animationsEnabledChanged)

public:
    explicit UpsSchematicModel(QObject *parent = nullptr);

    enum class Mode { Mains, Battery, LowBattery, Bypass, Test, OutputOff, Fault };

    bool hasMode() const { return m_hasMode; }
    Mode mode() const    { return m_mode; }
    QVariantMap banner() const  { return m_banner; }
    QVariantMap mains() const   { return m_mains; }
    QVariantMap ups() const     { return m_ups; }
    QVariantMap load() const    { return m_load; }
    QVariantMap battery() const { return m_battery; }
    QVariantMap lines() const   { return m_lines; }
    bool animationsEnabled() const { return m_animationsEnabled; }

public slots:
    void setStatus(const UpsStatus &status);
    void setBatteryLevel(const UpsBatteryLevel &level);
    void setBatteryConfig(const UpsBatteryConfig &config);
    void setLinkLost(bool lost);
    void setConnected(bool connected);
    void setAnimationsEnabled(bool on);

signals:
    void changed();
    void modeChanged();
    void animationsEnabledChanged();

private:

    Mode detectMode() const;
    void rebuild();
    void buildEmpty(const QString &bannerText);
    void markAllStale();
    QString batteryDetailText() const;
    void updateMode();

    bool m_hasMode = false;
    Mode m_mode = Mode::Mains;
    UpsStatus m_status;
    bool m_hasStatus = false;
    int m_batteryLevel = -1;
    UpsBatteryConfig m_batteryConfig;
    bool m_connected = false;
    bool m_linkLost = false;
    bool m_animationsEnabled = true;
    QDateTime m_lastUpdate;

    QVariantMap m_banner;
    QVariantMap m_mains;
    QVariantMap m_ups;
    QVariantMap m_load;
    QVariantMap m_battery;
    QVariantMap m_lines;
};

#endif
