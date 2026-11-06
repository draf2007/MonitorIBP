#ifndef COMPORTAVAILABLES_H
#define COMPORTAVAILABLES_H

#include <QObject>
#include <QSerialPortInfo>

class comPortAvailables : public QObject
{
    Q_OBJECT
public:
    explicit comPortAvailables(QObject *parent = nullptr);

public slots:
    void availablesPort();

signals:
    void portListSend(const QList<QSerialPortInfo> &portList);

private:

};

#endif
