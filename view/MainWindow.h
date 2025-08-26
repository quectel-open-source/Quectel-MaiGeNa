#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "BaseWindow.h"
#include "DrawView.h"
#include "EditSolution.h"
#include "commonZack.h"
#include "historyview.h"
#include "ui_MainWindow.h" // 自动生成的头文件
#include <QDoubleSpinBox>
#include <QListWidget>
#include <QSpinBox>
#include <QtWidgets/QWidget>
#include <qpointer.h>
#include <qprogressdialog.h>
#include <qscopedpointer.h>
#include <qwidget.h>


#include "MyParamsView.h"
#include "QGraphicsViews.h"
#include "WorkThread.h"


struct TotalCounts
{
    int total = 0;
    int ok = 0;
    int ng = 0;
    bool is_software = false;
};

class MainWindow : public BaseWindow
{
    Q_OBJECT

  public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

  protected:
    void closeEvent(QCloseEvent *event) override;

  private:
    Ui::MainWindow ui;
    bool m_isPressed = false;
    // bool   m_isMax = false;
    QPoint m_mousePressPoint;
    QPoint m_windowPosition;

    QList<QIcon> v_icons;
    QList<QString> m_headers;
    QList<QLabel *> v_labels_total;
    TotalCounts m_totalCounts;
    QList<QPixmap> v_pixs;

    QPointer<HistoryView> m_historyView = nullptr;
    QPointer<EditSolution> m_drawView = nullptr;
    MyParamsView *v_paramView = nullptr;
    HistoryLabelView *v_logView;
    // QtMyGroupBox*  v_sqlWindow;
    // MySqlView*     v_sqlView;
    // QtMyGroupBox*  v_drawWindow;
    SystemUsedWidget *v_systemUsedView;
    QTimer *m_timerUsed;

    QList<QSharedPointer<MVSCamera>> m_mvsCameras;
    std::vector<std::vector<std::string>> m_cameras_labels;
    QList<QGraphicsViews *> v_views;
    QList<WorkThread *> m_threads;
    std::vector<std::vector<std::string>> m_mvsCamera_infoList;
    QList<QString> m_mvs_ipList;
    QList<QString> m_mvs_ipList_first;

    QProgressDialog *m_loadingWidget = nullptr;

  private:
    void initUI(const QString &v_qs);
    void initDevices();
    void showView(QWidget *view, QString title = QObject::tr("Dialog"));

  private slots:
    void showLoading(bool loading = true, const QString &title = QString());
    void updateLog(QString text);
    void updateTime();
    void onWorkerResultReady(ResultData res_one);
    void onWorkerStateChanged(WorkThread::State state, int index);
    void onTemplateScoreChanged(int camid, int index, double value);
    void updateHistoryLoad(QString dir);

    void onSaveModeChanged(int save_type, int camid, int index);
    void softTrigger();
    void onRunCameraRecv(bool is_run);
    void updateUIEnabled(bool is_enabled);

    void runOnce(bool checked);
    void showParamView();
    void showDrawView();
    void showHistoryView();
    void openResultDir();
    void enableSaveJiaoZhengImage(int arg1);
    void enableSaveResultImage(int arg1);
    void enableSaveOriginImage(int arg1);
    void enableDisplayResult(int arg1);

  signals:
    void resultReady(ResultData resultData);
    void emit_log(QString text);
    void emit_runCamera(bool);
    void emit_updateSql(int);
};

#endif // MAINWINDOW_H
