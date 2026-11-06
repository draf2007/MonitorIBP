#include "dailylogfile.h"

#include <QDebug>
#include <QDir>
#include <QRegularExpression>
#include <QStandardPaths>

DailyLogFile::DailyLogFile(const QString &prefix, const QString &extension, const QByteArray &header)
    : m_prefix(prefix)
    , m_extension(extension)
    , m_header(header)
{
}

bool DailyLogFile::write(const QString &line)
{
    const QDate today = QDate::currentDate();
    if (!m_file.isOpen() || today != m_date) {
        if (!openFor(today))
            return false;
    }

    m_file.write(line.toUtf8() + "\n");
    m_file.flush();
    return true;
}

void DailyLogFile::close()
{
    m_file.close();
}

bool DailyLogFile::openFor(const QDate &date)
{
    m_file.close();

    const QString dir = logsDir();
    QDir().mkpath(dir);

    m_file.setFileName(QString("%1/%2_%3.%4")
                           .arg(dir, m_prefix, date.toString("yyyy-MM-dd"), m_extension));
    const bool isNew = !m_file.exists() || m_file.size() == 0;

    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        qDebug() << "Не удалось открыть журнал" << m_file.fileName() << m_file.errorString();
        return false;
    }
    if (isNew && !m_header.isEmpty())
        m_file.write(m_header + "\n");

    m_date = date;
    return true;
}

QString DailyLogFile::logsDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/logs";
}

void DailyLogFile::removeOlderThan(int days)
{
    const QDate oldest = QDate::currentDate().addDays(-days);
    static const QRegularExpression dateInName("_(\\d{4}-\\d{2}-\\d{2})\\.");

    QDir dir(logsDir());
    const QStringList files = dir.entryList(QDir::Files);
    for (const QString &name : files) {
        const QRegularExpressionMatch match = dateInName.match(name);
        if (!match.hasMatch())
            continue;
        const QDate date = QDate::fromString(match.captured(1), "yyyy-MM-dd");
        if (date.isValid() && date < oldest)
            dir.remove(name);
    }
}