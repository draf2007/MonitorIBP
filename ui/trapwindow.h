#ifndef TRAPWINDOW_H
#define TRAPWINDOW_H

#include <QDialog>
#include <QVector>
#include <QTemporaryFile>
#include "snmptrapdata.h"

namespace Ui {
class TrapWindow;
}

class TrapWindow : public QDialog
{
    Q_OBJECT

public:
    explicit TrapWindow(QWidget *parent = nullptr);
    ~TrapWindow();

    void setListening(bool listening, quint16 port);
    bool confirmClose();

public slots:
    void addTrap(const SnmpTrap &trap);

signals:
    void startRequested();
    void stopRequested();

private:
    enum Column{ColTime, ColSource, ColEvent, ColVersion, ColCommunity, ColTrapOid, ColumnCount};

    static constexpr int MaxRows = 1000;

    Ui::TrapWindow *ui;
    QVector<SnmpTrap> m_traps;
    QTemporaryFile *m_file = nullptr;
    bool m_unsaved = false;

    void writeToFile(const SnmpTrap &trap);
    bool saveToFile();

private slots:
    void onCurrentRowChanged();
    void onClearClicked();
    void onSaveClicked();
};

#endif