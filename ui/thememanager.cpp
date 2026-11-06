#include "thememanager.h"

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QStyleHints>
#include <QPainter>
#include <QSvgRenderer>

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent)
{
    m_light = {
        {"bg",         QColor(0xeef0ee)},
        {"win",        QColor(0xf7f8f6)},
        {"fg",         QColor(0x1d2320)},
        {"muted",      QColor(0x68716c)},
        {"line",       QColor(0xc4cbc6)},
        {"accent",     QColor(0x1f6f5c)},
        {"accentTint", QColor(0xe3efe9)},
        {"good",       QColor(0x2f9e44)},
        {"warn",       QColor(0xa86a00)},
        {"warnTint",   QColor(0xfbf0dc)},
        {"alarm",      QColor(0xb3261e)},
        {"alarmTint",  QColor(0xfbe6e4)},
        {"off",        QColor(0x9aa39e)},
        {"offBg",      QColor(0xeceeec)},
        {"btn",        QColor(0xfdfdfc)},
        {"btnline",    QColor(0xa9b2ac)},
    };

    m_dark = {
        {"bg",         QColor(0x171b19)},
        {"win",        QColor(0x202522)},
        {"fg",         QColor(0xe4e9e6)},
        {"muted",      QColor(0x9aa49e)},
        {"line",       QColor(0x3a423e)},
        {"accent",     QColor(0x6cc4a8)},
        {"accentTint", QColor(0x24352f)},
        {"good",       QColor(0x5fd068)},
        {"warn",       QColor(0xe3a640)},
        {"warnTint",   QColor(0x3a3020)},
        {"alarm",      QColor(0xff8a80)},
        {"alarmTint",  QColor(0x43211f)},
        {"off",        QColor(0x5f6863)},
        {"offBg",      QColor(0x1c201e)},
        {"btn",        QColor(0x2a302d)},
        {"btnline",    QColor(0x4d5651)},
               };

    QFile file(":/resources/style/style.qss");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        m_styleTemplate = QString::fromUtf8(file.readAll());
    else
        qDebug() << "Не удалось открыть style.qss";

    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
            this, &ThemeManager::apply);
}

void ThemeManager::setMode(Mode mode)
{
    m_mode = mode;

    Qt::ColorScheme scheme = Qt::ColorScheme::Unknown;
    if (mode == Mode::Light)
        scheme = Qt::ColorScheme::Light;
    else if (mode == Mode::Dark)
        scheme = Qt::ColorScheme::Dark;

    QGuiApplication::styleHints()->setColorScheme(scheme);
    apply();
}

bool ThemeManager::isDark() const
{
    if (m_mode == Mode::Light)
        return false;
    if (m_mode == Mode::Dark)
        return true;
    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
}

QColor ThemeManager::color(const QString &name) const
{
    return isDark() ? m_dark.value(name) : m_light.value(name);
}

void ThemeManager::apply()
{
    QApplication::setPalette(buildPalette());
    qApp->setStyleSheet(buildStyleSheet());
    emit themeChanged();
}

QPalette ThemeManager::buildPalette() const
{
    QPalette p(color("btn"), color("bg"));

    p.setColor(QPalette::Window,          color("bg"));
    p.setColor(QPalette::WindowText,      color("fg"));
    p.setColor(QPalette::Base,            color("win"));
    p.setColor(QPalette::AlternateBase,   color("bg"));
    p.setColor(QPalette::Text,            color("fg"));
    p.setColor(QPalette::Button,          color("btn"));
    p.setColor(QPalette::ButtonText,      color("fg"));
    p.setColor(QPalette::Highlight,       color("accent"));
    p.setColor(QPalette::HighlightedText, color("win"));
    p.setColor(QPalette::ToolTipBase,     color("win"));
    p.setColor(QPalette::ToolTipText,     color("fg"));
    p.setColor(QPalette::PlaceholderText, color("muted"));
    p.setColor(QPalette::Link,            color("accent"));
    p.setColor(QPalette::Accent,          color("accent"));

    p.setColor(QPalette::Disabled, QPalette::WindowText, color("off"));
    p.setColor(QPalette::Disabled, QPalette::Text,       color("off"));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, color("off"));

    return p;
}

QString ThemeManager::buildStyleSheet() const
{
    QString qss = m_styleTemplate;
    const QHash<QString, QColor> &colors = isDark() ? m_dark : m_light;
    for (auto it = colors.cbegin(); it != colors.cend(); ++it)
        qss.replace("@" + it.key() + "@", it.value().name());
    return qss;
}

QIcon ThemeManager::icon(const QString &name) const
{
    QFile file(":/resources/icons/" + name + ".svg");
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Нет значка" << name;
        return QIcon();
    }

    QByteArray svg = file.readAll();
    svg.replace("currentColor", color("accent").name().toLatin1());

    const int size = 24;
    const qreal ratio = 2.0;

    QPixmap pixmap(QSize(size, size) * ratio);
    pixmap.setDevicePixelRatio(ratio);
    pixmap.fill(Qt::transparent);

    QSvgRenderer renderer(svg);
    QPainter painter(&pixmap);
    renderer.render(&painter, QRectF(0, 0, size, size));
    painter.end();

    return QIcon(pixmap);
}

QVariantMap ThemeManager::colors() const
{
    QVariantMap map;
    const QHash<QString, QColor> &table = isDark() ? m_dark : m_light;
    for (auto it = table.cbegin(); it != table.cend(); ++it)
        map.insert(it.key(), it.value());
    return map;
}

void ThemeManager::setIconSet(const QString &name)
{
    if (name == m_iconSet)
        return;
    m_iconSet = name;
    emit iconSetChanged();
}
