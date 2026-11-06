#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QObject>
#include <QColor>
#include <QHash>
#include <QPalette>
#include <QString>
#include <QIcon>
#include <QVariantMap>

class ThemeManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap colors READ colors NOTIFY themeChanged)
    Q_PROPERTY(bool dark READ isDark NOTIFY themeChanged)
    Q_PROPERTY(QString iconSet READ iconSet NOTIFY iconSetChanged)

public:
    enum class Mode { System, Light, Dark };

    explicit ThemeManager(QObject *parent = nullptr);

    void setMode(Mode mode);
    Mode mode() const { return m_mode; }

    bool isDark() const;
    QColor color(const QString &name) const;
    QIcon icon(const QString &name) const;

    QVariantMap colors() const;

    QString iconSet() const { return m_iconSet; }
    void setIconSet(const QString &name);

signals:
    void themeChanged();
    void iconSetChanged();

private:
    void apply();
    QPalette buildPalette() const;
    QString buildStyleSheet() const;

    Mode m_mode = Mode::System;
    QString m_styleTemplate;
    QHash<QString, QColor> m_light;
    QHash<QString, QColor> m_dark;
        QString m_iconSet = "scheme";
};

#endif
