#ifndef EditSolution_H
#define EditSolution_H

#include "solutionmanager.h"
#include <QComboBox>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QImage>
#include <QInputDialog>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QToolButton>
#include <QWidget>
#include <qchar.h>
#include <qcolor.h>
#include <qimage.h>
#include <qlist.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qobjectdefs.h>

namespace Ui
{
class EditSolution;
}

struct TemplateConfig
{
    int id;
    QString name;
    QString path;
};

struct SolutionConfig
{
    QString name;
    QString path;
    bool isDefault = false;
};

class EditSolution : public QWidget
{
    Q_OBJECT

  public:
    explicit EditSolution(QWidget *parent = nullptr);
    ~EditSolution();

  signals:
    void captureRequested();

  public slots:
    void loadSolutions();
    void setSolutions(QList<Solution *> solutions);
    void setSolution(const Solution *solution);
    void setImage(const QImage &image);
    void setROIs(const QList<ROI> &rois);

  private slots:
    // 方案管理
    void onAddSolution();
    void onDeleteSolution();
    void onSaveSolution();

    // 图像操作
    void onCaptureImage();
    void onOpenImage();
    void onSaveImage();

    // ROI管理
    void onDrawROI();
    void onTableItemChanged(QTableWidgetItem *item);
    void onTableItemClicked(QTableWidgetItem *item);
    void onDeleteROIClicked();

  private:
    Ui::EditSolution *ui;
    QGraphicsScene *m_scene; // 用于显示图像的场景

    // 初始化函数
    void initUI();
    void initConnections();

    // 辅助函数
    void updateStatus(const QString &message, bool isError);
    void updateStatus(const QString &message)
    {
        updateStatus(message, false);
    }
    void updateStatusError(const QString &message)
    {
        updateStatus(message, true);
    }
    void addROIToTable(const ROI &roi);
    void paintROI(const ROI &roi);

    QString currentSolutionId() const;
    Solution *currentSolution();
    QImage currentSolutionImage();

    // 当前状态
    QGraphicsPixmapItem *m_backgroundItem = nullptr;
    int m_selectedROIId = -1;
};

#endif // EditSolution_H