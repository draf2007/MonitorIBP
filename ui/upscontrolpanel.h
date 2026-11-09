#ifndef UPSCONTROLPANEL_H
#define UPSCONTROLPANEL_H

#include <QWidget>
#include <QTimer>
#include "core/upsdatastruct.h"
#include "core/upscommand.h"
#include "core/upsshutdownscenario.h"

namespace Ui {
class UpsControlPanel;
}

class UpsControlPanel : public QWidget
{
    Q_OBJECT

public:
    explicit UpsControlPanel(QWidget *parent = nullptr);
    ~UpsControlPanel() override;

    void setScenario(UpsShutdownScenario *scenario);
    void setConnected(bool connected);
    void setStatus(const UpsStatus &status);

    void setDelays(int shutdownDelaySec, int restartDelaySec);

    void setTimedTestAvailable(bool available);
    void setBeeperLogEnabled(bool enabled);

signals:
    void commandRequested(UpsCommand command, const UpsShutdownParams &params);
    void delaysChanged(int shutdownDelaySec, int restartDelaySec);
    void beeperLogToggled(bool enabled);

private:
    Ui::UpsControlPanel *ui;
    UpsShutdownScenario *m_scenario = nullptr;
    QTimer *m_refreshTimer;
    bool m_connected = false;
    UpsStatus m_status;
    bool m_hasStatus = false;
    bool m_timedTestAvailable = false;

    bool confirm(const QString &question);
    void sendSimpleCommand(UpsCommand command);
    void onShutdownClicked();
    void onShutdownNowClicked();
    void onCancelShutdownClicked();
    void onTurnOnClicked();
    void onHelpClicked();
    void onTimedTestClicked();
    void updateControls();
};

#endif