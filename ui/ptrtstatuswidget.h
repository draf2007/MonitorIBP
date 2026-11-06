#ifndef PTRTSTATUSWIDGET_H
#define PTRTSTATUSWIDGET_H

#include <QWidget>
#include "core/upsdatastruct.h"

namespace Ui {
class PtrtStatusWidget;
}

class PtrtStatusWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PtrtStatusWidget(QWidget *parent = nullptr);
    ~PtrtStatusWidget();

    void setStatus(const UpsPtrtStatus &status);
    void clear();

private:
    Ui::PtrtStatusWidget *ui;
};

#endif
