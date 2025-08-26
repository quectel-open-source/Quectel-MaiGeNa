#pragma once
#include "inspectionrecordrepo.h"
#include "QtMyGroupBox.h"
#include "basedialog.h"
#include "imageviewer.h"
#include <QStandardItemModel>
#include <QWidget>
#include <qobjectdefs.h>
#include <qsqldatabase.h>
#include <qwidget.h>

QT_BEGIN_NAMESPACE
namespace Ui
{
class HistoryView;
}
QT_END_NAMESPACE

class HistoryView : public QWidget
{
    Q_OBJECT

  public:
    HistoryView(QWidget *parent = nullptr);
    ~HistoryView();

  public slots:
    void refreshTable();
    
  private slots:
    void onSearchClicked();
    void onPrevPageClicked();
    void onNextPageClicked();
    void onPageSizeChanged(int size);
    void onTableViewItemClicked(const QModelIndex &index);

  private:
    void updatePaginationInfo();

    Ui::HistoryView *ui;
    QStandardItemModel *m_model;

    // 分页相关变量
    int m_currentPage = 1;
    int m_pageSize = 10;
    int m_totalRecords = 0;
    QString m_currentFilter;
    ImageViewer *m_imageViewer = nullptr;

    InspectionRecordRepo *m_repo = nullptr;
};