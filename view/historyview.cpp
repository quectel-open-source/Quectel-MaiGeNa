#include "historyview.h"
#include "appcontext.h"
#include "inspectionrecordrepo.h"
#include "ui_historyview.h"
#include <qabstractitemview.h>
#include <qboxlayout.h>
#include <qchar.h>
#include <qcombobox.h>
#include <qdialog.h>
#include <qglobal.h>
#include <qlist.h>
#include <qstandarditemmodel.h>
#include <qurl.h>
#include <qwidget.h>

HistoryView::HistoryView(QWidget *parent)
    : QWidget(parent), ui(new Ui::HistoryView), m_model(new QStandardItemModel(this)),
      m_repo(AppContext::getService<InspectionRecordRepo>())
{
    ui->setupUi(this);

    // 初始化UI
    ui->tableView->setModel(m_model);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 初始化分页控件
    ui->pageSizeComboBox->addItems({"10", "20", "50", "100"});
    ui->pageSizeComboBox->setCurrentIndex(0);

    // 连接信号槽
    connect(ui->searchButton, &QPushButton::clicked, this, &HistoryView::onSearchClicked);
    connect(ui->prevButton, &QPushButton::clicked, this, &HistoryView::onPrevPageClicked);
    connect(ui->nextButton, &QPushButton::clicked, this, &HistoryView::onNextPageClicked);
    connect(ui->pageSizeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &HistoryView::onPageSizeChanged);
    connect(ui->tableView, &QTableView::clicked, this, &HistoryView::onTableViewItemClicked);

    connect(m_repo, &InspectionRecordRepo::recordInserted, [this] { refreshTable(); });
    connect(m_repo, &InspectionRecordRepo::recordUpdated, [this] { refreshTable(); });
    connect(m_repo, &InspectionRecordRepo::recordRemoved, [this] { refreshTable(); });

    refreshTable();
}

HistoryView::~HistoryView()
{
    delete ui;
    qDebug() << "HistoryView::~HistoryView()";
}

void HistoryView::refreshTable()
{
    auto startDate = ui->startDateEdit->dateTime();
    auto endDate = ui->endDateEdit->dateTime();

    auto [totalCount, records] = AppContext::getService<InspectionRecordRepo>()->queryByFilter(
        m_currentFilter, startDate, endDate, m_currentPage, m_pageSize);
    m_totalRecords = totalCount;

    m_model->clear();
    m_model->setHorizontalHeaderLabels(InspectionRecordRepo::COLUMN_NAMES);

    for (const auto &record : records)
    {
        QList<QStandardItem *> row;
        // {"sn", "costTime", "status", "total", "okCount", "ngCount", "result", "path", "createdAt"};
        // row << new QStandardItem(record.id);
        row << new QStandardItem(record.sn);
        row << new QStandardItem(QString::number(record.costTime));
        row << new QStandardItem(record.status == 0 ? tr("OK") : tr("NG"));
        row << new QStandardItem(QString::number(record.total));
        row << new QStandardItem(QString::number(record.okCount));
        row << new QStandardItem(QString::number(record.ngCount));
        row << new QStandardItem(record.result);
        row << new QStandardItem(record.path);
        row << new QStandardItem(record.createdAt.toString("yyyy-MM-dd hh:mm:ss"));

        m_model->appendRow(row);
    }
    // 更新分页信息
    updatePaginationInfo();
}

void HistoryView::updatePaginationInfo()
{
    int totalPages = qMax(1, (m_totalRecords + m_pageSize - 1) / m_pageSize);
    ui->pageLabel->setText(tr("Page %1/%2").arg(m_currentPage).arg(totalPages));
    ui->totalLabel->setText(tr("Total: %1 records").arg(m_totalRecords));

    ui->prevButton->setEnabled(m_currentPage > 1);
    ui->nextButton->setEnabled(m_currentPage < totalPages);
}

void HistoryView::onSearchClicked()
{
    QString searchText = ui->searchLineEdit->text().trimmed();

    if (searchText.isEmpty())
    {
        m_currentFilter.clear();
    }
    m_currentFilter = searchText;
    m_currentPage = 1;
    refreshTable();
}

void HistoryView::onPrevPageClicked()
{
    if (m_currentPage > 1)
    {
        m_currentPage--;
        refreshTable();
    }
}

void HistoryView::onNextPageClicked()
{
    int totalPages = qMax(1, (m_totalRecords + m_pageSize - 1) / m_pageSize);
    if (m_currentPage < totalPages)
    {
        m_currentPage++;
        refreshTable();
    }
}

void HistoryView::onPageSizeChanged(int index)
{
    Q_UNUSED(index);
    m_pageSize = ui->pageSizeComboBox->currentText().toInt();
    m_currentPage = 1;
    refreshTable();
}

void HistoryView::onTableViewItemClicked(const QModelIndex &index)
{
    QString sn = m_model->item(index.row(), InspectionRecordRepo::COLUMN_NAMES.indexOf("sn"))->text();
    QString path = m_model->item(index.row(), InspectionRecordRepo::COLUMN_NAMES.indexOf("path"))->text();
    QString result = m_model->item(index.row(), InspectionRecordRepo::COLUMN_NAMES.indexOf("result"))->text();
    qDebug() << "sn:" << sn << "path:" << path << "result:" << result;

    QImage qimage;
    if (!qimage.load(path))
    {
        qDebug() << "Failed to load image:" << path;
    }
    auto pixmap = QPixmap::fromImage(qimage);
    auto sz = ui->imageView->size();
    ui->imageView->setPixmap(pixmap.scaled(sz, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    QString detailText = tr("Path") + ": " + path + "\n";
    detailText += tr("Result") + ": " + result + "\n";
    ui->detailLabel->setText(detailText);
}
