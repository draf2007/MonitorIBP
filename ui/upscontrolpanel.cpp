#include "ui/upscontrolpanel.h"
#include "ui_upscontrolpanel.h"
#include <QMessageBox>

static QString testResultText(UpsTestResult result)
{
    switch (result) {
    case UpsTestResult::Passed:     return "пройден";
    case UpsTestResult::Warning:    return "пройден с предупреждением";
    case UpsTestResult::Error:      return "ошибка";
    case UpsTestResult::Aborted:    return "прерван";
    case UpsTestResult::InProgress: return "идёт";
    case UpsTestResult::NoTest:     return "тестов не было";
    default:                        return QString();
    }
}

UpsControlPanel::UpsControlPanel(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::UpsControlPanel)
    , m_refreshTimer(new QTimer(this))
{
    ui->setupUi(this);

    QObject::connect(ui->quickTestButton, &QPushButton::clicked, this, [this]() { sendSimpleCommand(UpsCommand::QuickTest); });
    QObject::connect(ui->deepTestButton, &QPushButton::clicked, this, [this]() { sendSimpleCommand(UpsCommand::DeepTest); });
    QObject::connect(ui->cancelTestButton, &QPushButton::clicked, this, [this]() { sendSimpleCommand(UpsCommand::CancelTest); });
    QObject::connect(ui->beeperButton, &QPushButton::clicked, this, [this]() { sendSimpleCommand(UpsCommand::ToggleBeeper); });

    QObject::connect(ui->shutdownButton, &QPushButton::clicked, this, &UpsControlPanel::onShutdownClicked);
    QObject::connect(ui->shutdownNowButton, &QPushButton::clicked, this, &UpsControlPanel::onShutdownNowClicked);
    QObject::connect(ui->cancelShutdownButton, &QPushButton::clicked, this, &UpsControlPanel::onCancelShutdownClicked);
    QObject::connect(ui->turnOnButton, &QPushButton::clicked, this, &UpsControlPanel::onTurnOnClicked);
    QObject::connect(ui->helpButton, &QPushButton::clicked, this, &UpsControlPanel::onHelpClicked);

    auto emitDelays = [this]() { emit delaysChanged(ui->shutdownDelaySpin->value(), ui->restartDelaySpin->value()); };
    QObject::connect(ui->shutdownDelaySpin, qOverload<int>(&QSpinBox::valueChanged), this, emitDelays);
    QObject::connect(ui->restartDelaySpin, qOverload<int>(&QSpinBox::valueChanged), this, emitDelays);

    m_refreshTimer->setInterval(1000);
    QObject::connect(m_refreshTimer, &QTimer::timeout, this, &UpsControlPanel::updateControls);
    QObject::connect(ui->timedTestButton, &QPushButton::clicked, this, &UpsControlPanel::onTimedTestClicked);

    updateControls();
}

UpsControlPanel::~UpsControlPanel()
{
    delete ui;
}

void UpsControlPanel::setScenario(UpsShutdownScenario *scenario)
{
    m_scenario = scenario;
    QObject::connect(m_scenario, &UpsShutdownScenario::stateChanged, this, [this](UpsShutdownScenario::State state) {
        const bool counting = (state == UpsShutdownScenario::State::WaitingShutdown
                               || state == UpsShutdownScenario::State::WaitingStartup);
        if (counting) {
            m_refreshTimer->start();
        } else {
            m_refreshTimer->stop();
        }
        updateControls();
    });
    updateControls();
}

void UpsControlPanel::setConnected(bool connected)
{
    m_connected = connected;
    if (!connected) {
        m_hasStatus = false;
    }
    updateControls();
}

void UpsControlPanel::setStatus(const UpsStatus &status)
{
    m_status = status;
    m_hasStatus = true;
    updateControls();
}

void UpsControlPanel::setDelays(int shutdownDelaySec, int restartDelaySec)
{
    ui->shutdownDelaySpin->setValue(shutdownDelaySec);
    ui->restartDelaySpin->setValue(restartDelaySec);
}

void UpsControlPanel::setTimedTestAvailable(bool available)
{
    m_timedTestAvailable = available;
    updateControls();
}

void UpsControlPanel::onTimedTestClicked()
{
    const int minutes = ui->testMinutesSpin->value();
    if (!confirm(QString("Запустить тест батареи на %1 мин?\n\n"
                         "ИБП будет работать от батареи. Если она разрядится раньше, "
                         "ИБП сам вернётся на сеть.").arg(minutes))) {
        return;
    }
    UpsShutdownParams params;
    params.testMinutes = minutes;
    emit commandRequested(UpsCommand::TimedTest, params);
}

bool UpsControlPanel::confirm(const QString &question)
{
    return QMessageBox::question(this, "Управление ИБП", question) == QMessageBox::Yes;
}

void UpsControlPanel::sendSimpleCommand(UpsCommand command)
{
    if (!confirm(QString("Выполнить команду «%1»?").arg(upsCommandName(command)))) {
        return;
    }
    emit commandRequested(command, UpsShutdownParams());
}

void UpsControlPanel::onShutdownClicked()
{
    const int x = ui->shutdownDelaySpin->value();
    const int y = ui->restartDelaySpin->value();

    const QString when = (x == 0) ? QString("сейчас") : QString("через %1 с").arg(x);
    const QString restart = (y == 0) ? QString("Включать обратно не будем.")
                                     : QString("Через %1 с после выключения ИБП включится.").arg(y);
    if (!confirm(QString("Выключить ИБП %1?\n%2\n\nВыход ИБП перейдёт на байпас.").arg(when, restart))) {
        return;
    }

    m_scenario->start(UpsShutdownParams{x, (y == 0) ? -1 : y});
}

void UpsControlPanel::onShutdownNowClicked()
{
    if (!confirm("Выключить ИБП сейчас?\nВключать обратно не будем — включите кнопкой «Включить сейчас».\n\n"
                 "Выход ИБП перейдёт на байпас.")) {
        return;
    }

    m_scenario->start(UpsShutdownParams{0, -1});
}

void UpsControlPanel::onCancelShutdownClicked()
{
    const bool startup = (m_scenario->state() == UpsShutdownScenario::State::WaitingStartup);
    if (!confirm(startup ? QString("Отменить включение ИБП по таймеру?")
                         : QString("Отменить выключение ИБП?"))) {
        return;
    }
    m_scenario->cancel(startup ? QString("включение отменено") : QString());
}

void UpsControlPanel::onTurnOnClicked()
{
    if (!confirm("Включить ИБП сейчас?")) {
        return;
    }
    m_scenario->turnOnNow();
}

void UpsControlPanel::onHelpClicked()
{
    QMessageBox::information(this, "Выключение и включение",
                             "Таймеры ведёт программа, а ИБП получает только команды «выключить сейчас» и «включить сейчас».\n\n"
                             "• «Выключить» — по таймерам: через X, затем включить через Y.\n"
                             "• «Выключить сейчас» — сразу и без автоматического включения.\n"
                             "• «Выключить через» 0 — выключить сразу.\n"
                             "• «Включить через» 0 — после выключения не включать.\n"
                             "• После выключения выход ИБП идёт через байпас: нагрузка питается от сети без защиты.\n"
                             "• Программа должна работать до конца отсчёта. При потере связи сценарий отменяется, "
                             "и ИБП нужно включить кнопкой «Включить сейчас».");
}

void UpsControlPanel::updateControls()
{
    using State = UpsShutdownScenario::State;
    const State state = m_scenario ? m_scenario->state() : State::Idle;
    const bool known = m_connected && m_hasStatus;
    const bool outputOff = known && m_status.outputOff;
    const bool testing = known && m_status.testInProgress;

    ui->quickTestButton->setEnabled(known && !testing && !outputOff && state == State::Idle);
    ui->deepTestButton->setEnabled(known && !testing && !outputOff && state == State::Idle);
    ui->cancelTestButton->setEnabled(known && testing);
    ui->timedTestButton->setVisible(m_timedTestAvailable);
    ui->testMinutesSpin->setVisible(m_timedTestAvailable);
    ui->timedTestButton->setEnabled(known && !testing && !outputOff && state == State::Idle);

    const QString result = known ? testResultText(m_status.testResult) : QString();
    ui->testResultLabel->setVisible(!result.isEmpty());
    ui->testResultLabel->setText(QString("Последний тест: %1").arg(result));

    ui->shutdownDelaySpin->setEnabled(state == State::Idle);
    ui->restartDelaySpin->setEnabled(state == State::Idle);
    ui->shutdownButton->setEnabled(known && !outputOff && state == State::Idle);
    ui->shutdownNowButton->setEnabled(known && !outputOff && state == State::Idle);
    ui->cancelShutdownButton->setEnabled(m_connected && (state == State::WaitingShutdown || state == State::WaitingStartup));
    ui->cancelShutdownButton->setText(state == State::WaitingStartup ? "Отменить включение"
                                                                     : "Отменить выключение");
    ui->turnOnButton->setEnabled(outputOff && state != State::ShuttingDown && state != State::StartingUp);

    ui->beeperButton->setEnabled(known);
    const bool beeperOn = known && m_status.beeperOn;
    ui->beeperButton->setText(beeperOn ? "Выключить динамик" : "Включить динамик");
    ui->beeperStateLabel->setText(!known ? QString("—") : beeperOn ? QString("● Динамик включен") : QString("○ Динамик выключен"));
}