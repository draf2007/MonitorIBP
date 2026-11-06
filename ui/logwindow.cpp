#include "logwindow.h"
#include "ui_logwindow.h"
#include <QTime>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QHeaderView>
#include <QScrollBar>
#include <QTableWidgetItem>
#include <QDebug>

LogWindow::LogWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LogWindow)
{
    ui->setupUi(this);
    ui->logTextEdit->setMaximumBlockCount(MaxLines);
    ui->logTextEdit->setUndoRedoEnabled(false);
    QObject::connect(ui->openLogsDir, &QPushButton::clicked, this, &LogWindow::onOpenLogsDirClicked);
    QObject::connect(ui->clearLog, &QPushButton::clicked, this, &LogWindow::onClearClicked);
    QObject::connect(ui->toggleLoggingBtn, &QPushButton::toggled, this, &LogWindow::onToggleLoggingClicked);
    QObject::connect(ui->toggleDataBtn, &QPushButton::toggled, this, &LogWindow::onToggleDataClicked);
    QObject::connect(ui->clearDataBtn, &QPushButton::clicked, this, [this](){
        ui->dataTable->setRowCount(0);
    });

    ui->dataTable->verticalHeader()->setVisible(false);
    QHeaderView *header = ui->dataTable->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Interactive);
    header->setStretchLastSection(true);
}

LogWindow::~LogWindow()
{
    delete ui;
}

void LogWindow::appendLine(const QString &line){
    if (m_paused) {
        return;
    }
    writeLine(line);
}

void LogWindow::appendTrapLine(const QString &line){
    writeLine(line);
}

void LogWindow::writeLine(const QString &line){
    if (!m_loggingEnable) {
        return;
    }

    QString timestamp = QTime::currentTime().toString("HH:mm:ss.zzz");
    QString text = QString("[%1] %2").arg(timestamp, line);
    ui->logTextEdit->appendPlainText(text);
    m_eventsFile.write(text);
}

void LogWindow::setPaused(bool paused){
    m_paused = paused;
}

void LogWindow::setLoggingEnabled(bool enabled){
    ui->toggleLoggingBtn->setChecked(enabled);
}

void LogWindow::onToggleLoggingClicked(bool checked){
    m_loggingEnable = checked;
    ui->toggleLoggingBtn->setText(checked ? "Остановить запись в лог" : "Запустить запись в лог");
}

void LogWindow::onClearClicked(){
    ui->logTextEdit->clear();
}

void LogWindow::onOpenLogsDirClicked(){
    QDir().mkpath(DailyLogFile::logsDir());
    QDesktopServices::openUrl(QUrl::fromLocalFile(DailyLogFile::logsDir()));
}

void LogWindow::setMeasurementLoggingEnabled(bool enabled){
    ui->toggleDataBtn->setChecked(enabled);
}

void LogWindow::onToggleDataClicked(bool checked){
    ui->toggleDataBtn->setText(checked ? "Остановить запись измерений" : "Записывать измерения");
    emit measurementLoggingToggled(checked);
}

void LogWindow::setMeasurementColumns(const QStringList &names){
    ui->dataTable->setColumnCount(names.size());
    ui->dataTable->setHorizontalHeaderLabels(names);
    growColumnsFor(names);
}

void LogWindow::appendMeasurementRow(const QStringList &cells){
    qDebug() << "row:" << cells;
    QTableWidget *table = ui->dataTable;
    QScrollBar *bar = table->verticalScrollBar();
    const bool atBottom = bar->value() == bar->maximum();

    const int row = table->rowCount();
    table->insertRow(row);
    for (int column = 0; column < cells.size() && column < table->columnCount(); ++column) {
        auto *item = new QTableWidgetItem(cells.at(column));
        if (column >= 2 && column <= 7)
            item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        table->setItem(row, column, item);
    }
    growColumnsFor(cells);

    while (table->rowCount() > MaxDataRows)
        table->removeRow(0);

    if (atBottom)
        table->scrollToBottom();
}

void LogWindow::growColumnsFor(const QStringList &texts){
    QTableWidget *table = ui->dataTable;
    const QFontMetrics fm(table->horizontalHeader()->font());
    const int padding = 24;
    const int last = table->columnCount() - 1;
    for (int column = 0; column < texts.size() && column < last; ++column) {
        const int need = fm.horizontalAdvance(texts.at(column)) + padding;
        if (need > table->columnWidth(column))
            table->setColumnWidth(column, need);
    }
}