#ifndef UPSIDENTWIDGET_H
#define UPSIDENTWIDGET_H

#include <QWidget>
#include <QLabel>
#include "core/upsdatastruct.h"

class UpsIdentWidget : public QWidget
{
    Q_OBJECT

public:
    explicit UpsIdentWidget(QWidget *parent = nullptr) : QWidget(parent) {}
    virtual void setIdentification(const UpsIdentification &info) = 0;

protected:
    static bool isPlaceholder(const QString &text)
    {
        const QString trimmed = text.trimmed();
        if (trimmed.isEmpty()) {
            return true;
        }
        return trimmed.size() >= 4 && trimmed.count(trimmed.at(0)) == trimmed.size();
    }

    static void setRow(QLabel *title, QLabel *value, const QString &text)
    {
        const bool visible = !isPlaceholder(text);
        title->setVisible(visible);
        value->setVisible(visible);
        value->setText(text.trimmed());
    }
};

#endif