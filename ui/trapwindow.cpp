#include "trapwindow.h"
#include "ui_trapwindow.h"
#include "snmptrapevents.h"
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QDateTime>
#include <QDir>

static QString csvField(QString text){
    text.replace("\"", "\"\"");
    return "\"" + text + "\"";
}

TrapWindow::TrapWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::TrapWindow)
{
    ui->setupUi(this);

    ui->trapTable->setColumnCount(ColumnCount);
    ui->trapTable->setHorizontalHeaderLabels({"Время", "Источник", "Событие", "Версия", "Community", "Trap OID"});
    ui->trapTable->horizontalHeader()->setStretchLastSection(true);
    ui->trapTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->trapTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->trapTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QObject::connect(ui->trapTable, &QTableWidget::itemSelectionChanged, this, &TrapWindow::onCurrentRowChanged);
    QObject::connect(ui->clearBtn, &QPushButton::clicked, this, &TrapWindow::onClearClicked);
    QObject::connect(ui->saveBtn, &QPushButton::clicked, this, &TrapWindow::onSaveClicked);
    QObject::connect(ui->startBtn, &QPushButton::clicked, this, &TrapWindow::startRequested);
    QObject::connect(ui->stopBtn, &QPushButton::clicked, this, &TrapWindow::stopRequested);

    setListening(false, 0);
}

TrapWindow::~TrapWindow()
{
    delete ui;
}

void TrapWindow::setListening(bool listening, quint16 port){
    ui->statusLabel->setText(listening ? QString("Приём трапов на UDP-порту %1").arg(port)
                                       : QString("Приём трапов остановлен"));
    ui->startBtn->setEnabled(!listening);
    ui->stopBtn->setEnabled(listening);
}

void TrapWindow::addTrap(const SnmpTrap &trap){
    writeToFile(trap);

    if (m_traps.size() >= MaxRows) {
        m_traps.removeFirst();
        ui->trapTable->removeRow(0);
    }
    m_traps.append(trap);

    int row = ui->trapTable->rowCount();
    ui->trapTable->insertRow(row);
    ui->trapTable->setItem(row, ColTime,      new QTableWidgetItem(trap.receivedAt.toString("dd.MM.yyyy HH:mm:ss")));
    ui->trapTable->setItem(row, ColSource,    new QTableWidgetItem(trap.sourceIp));
    auto *eventItem = new QTableWidgetItem(trapEventText(trap));
    eventItem->setToolTip(trap.event);
    ui->trapTable->setItem(row, ColEvent, eventItem);
    ui->trapTable->setItem(row, ColVersion,   new QTableWidgetItem(trap.version));
    ui->trapTable->setItem(row, ColCommunity, new QTableWidgetItem(trap.community));
    ui->trapTable->setItem(row, ColTrapOid,   new QTableWidgetItem(trap.trapOid));
    ui->trapTable->scrollToBottom();
}

void TrapWindow::onCurrentRowChanged(){
    int row = ui->trapTable->currentRow();
    if (row < 0 || row >= m_traps.size()) {
        ui->trapDetails->clear();
        return;
    }
    ui->trapDetails->setPlainText(m_traps[row].variables.join("\n"));
}

void TrapWindow::onClearClicked(){
    m_traps.clear();
    ui->trapTable->setRowCount(0);
    ui->trapDetails->clear();
    delete m_file;
    m_file = nullptr;
    m_unsaved = false;
}

void TrapWindow::writeToFile(const SnmpTrap &trap){
    const QString separator = ";";

    if (!m_file) {
        m_file = new QTemporaryFile(QDir::tempPath() + "/ibp_traps_XXXXXX.csv", this);
        if (!m_file->open()) {
            delete m_file;
            m_file = nullptr;
            return;
        }

        QString header = QStringList{"Время", "Источник", "Событие", "Версия", "Community", "Trap OID", "Переменные"}.join(separator) + "\r\n";
        m_file->write(QByteArray("\xEF\xBB\xBF") + header.toUtf8());
    }

    QStringList fields{trap.receivedAt.toString("dd.MM.yyyy HH:mm:ss"),
                       trap.sourceIp,
                       trapEventText(trap),
                       trap.version,
                       trap.community,
                       trap.trapOid,
                       trap.variables.join("\n")};
    for (QString &field : fields) {
        field = csvField(field);
    }
    m_file->write((fields.join(separator) + "\r\n").toUtf8());
    m_file->flush();
    m_unsaved = true;
}

void TrapWindow::onSaveClicked(){
    saveToFile();
}

bool TrapWindow::saveToFile(){
    if (!m_file) {
        QMessageBox::information(this, "Сохранение трапов", "Таблица пуста, сохранять нечего");
        return false;
    }

    QString defaultName = QString("traps_%1.csv").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm"));
    QString fileName = QFileDialog::getSaveFileName(this, "Сохранить трапы", defaultName, "CSV для Excel (*.csv)");
    if (fileName.isEmpty()) {
        return false;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "Сохранение трапов", QString("Не удалось открыть файл для записи:\n%1").arg(file.errorString()));
        return false;
    }
    m_file->seek(0);
    file.write(m_file->readAll());
    m_file->seek(m_file->size());
    m_unsaved = false;
    return true;
}

bool TrapWindow::confirmClose(){
    if (!m_unsaved) {
        return true;
    }
    auto answer = QMessageBox::question(this, "Выход", "Трапы не сохранены. Сохранить их перед выходом?",
                                        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    if (answer == QMessageBox::Yes) {
        return saveToFile();
    }
    return answer == QMessageBox::No;
}