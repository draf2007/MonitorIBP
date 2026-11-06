#ifndef DAILYLOGFILE_H
#define DAILYLOGFILE_H

#include <QByteArray>
#include <QDate>
#include <QFile>
#include <QString>

class DailyLogFile
{
public:
    DailyLogFile(const QString &prefix, const QString &extension, const QByteArray &header = QByteArray());

    bool write(const QString &line);
    void close();

    static QString logsDir();
    static void removeOlderThan(int days);

private:
    bool openFor(const QDate &date);

    QString m_prefix;
    QString m_extension;
    QByteArray m_header;
    QFile m_file;
    QDate m_date;
};

#endif