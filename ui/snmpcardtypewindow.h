#ifndef SNMPCARDTYPEWINDOW_H
#define SNMPCARDTYPEWINDOW_H

#include <QDialog>
#include <QString>

namespace Ui {
class SnmpCardTypeWindow;
}

class SnmpCardTypeWindow : public QDialog
{
    Q_OBJECT

public:
    explicit SnmpCardTypeWindow(QWidget *parent = nullptr);
    ~SnmpCardTypeWindow();
    QString cardType() const;
    void setCardType(const QString &cardId);

private:
    Ui::SnmpCardTypeWindow *ui;
    void onCardChanged(int index);
};

#endif
