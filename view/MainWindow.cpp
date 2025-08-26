#include "MainWindow.h"
#include "CameraParams.h"
#include "DrawView.h"
#include "MVSCamera.h"
#include "MvErrorDefine.h"
#include "WorkThread.h"
#include "basedialog.h"
#include "commonZack.h"
#include "globals.h"
#include "historyview.h"
#include <qapplication.h>
#include <qcheckbox.h>
#include <qdialog.h>
#include <qeventloop.h>
#include <qglobal.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qobjectdefs.h>
#include <qscopedpointer.h>
#include <qthread.h>
#include <qwidget.h>

static void log_callback(std::string a, void *pUser)
{
    MainWindow *pThis = (MainWindow *)pUser;
    emit pThis->emit_log(QString::fromStdString(a));
}

///////////////////////////
MainWindow::MainWindow(QWidget *parent) : BaseWindow(parent)
{
    // 初始化 UI
    ui.setupUi(contentWidget());
    initUI("");
    titleBar()->setTitle(tr("Quectel宏景PCBA板AI检测"));
}
MainWindow::~MainWindow()
{
}

void MainWindow::updateLog(QString text)
{
    QString currenttime = QDateTime::currentDateTime().toString("[yyyy-MM-dd hh:mm:ss.zzz]");
    ui.textEdit->append(currenttime + ": " + text);
    ui.textEdit->ensureCursorVisible();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    event->ignore();

    setEnabled(false);
    showLoading(true, tr("正在关闭..."));

    QEventLoop waitLoop;
    QFutureWatcher<void> watcher;
    connect(&watcher, &QFutureWatcher<void>::finished, &waitLoop, &QEventLoop::quit);
    QFuture<void> future = QtConcurrent::run([this]() {
        // 停止所有线程
        for (auto &th : m_threads)
        {
            if (th && th->isRunning())
            {
                th->stop();
            }
            if (!th->wait(2000)) // 设置超时时间
            {
                qWarning() << "Thread" << th << "failed to stop in time";
                th->terminate(); // 最后手段
            }
            qDebug() << "Thread closed:" << th;
        }
        m_threads.clear();

        // 关闭相机
        for (int i = 0; i < m_mvsCameras.size(); i++)
        {
            try
            {
                m_mvsCameras[i]->stopGrabbing(i);
                m_mvsCameras[i]->closeCamera(i);
                qDebug() << "Camera closed:" << i;
            }
            catch (const std::exception &e)
            {
                qCritical() << "Error closing camera:" << e.what();
            }
        }
        m_mvsCameras.clear();
    });

    watcher.setFuture(future);
    waitLoop.exec(); // 阻塞直到完成
    showLoading(false);
    qApp->quit(); // 所有操作完成后退出

    // // 退出时关闭进程
    QProcess::execute("killall -9 " + qApp->applicationName()); // Linux/Unix系统
}

void MainWindow::initUI(const QString &v_qss)
{
    //    v_resizeView = new ResizeHandle(this);
    v_views.append(new QGraphicsViews(this));
    v_views.append(new QGraphicsViews(this));

    v_labels_total = {ui.label_8, ui.label_9, ui.label_10, ui.label_4, ui.label_11};

    // loadQSS_all(this, v_qss);
    ui.label_6->setStyleSheet("color: rgb(102, 204, 102);");
    v_labels_total[1]->setStyleSheet("color: rgb(102, 204, 102);");
    v_labels_total[3]->setStyleSheet("color: rgb(102, 204, 102);");
    v_labels_total[2]->setStyleSheet("color: rgb(225, 64, 51);");
    v_labels_total[4]->setStyleSheet("color: rgb(225, 64, 51);");
    ui.label_7->setStyleSheet("color: rgb(225, 64, 51);");
    v_pixs = {
        QPixmap(":/Resources/Resources/ok.png").scaled(110, 110, Qt::KeepAspectRatio, Qt::SmoothTransformation),
        QPixmap(":/Resources/Resources/ng.png").scaled(110, 110, Qt::KeepAspectRatio, Qt::SmoothTransformation),
    };
    v_icons = {QIcon(":/Resources/Resources/run2.png"), QIcon(":/Resources/Resources/stop2.png"),
               QIcon(":/Resources/Resources/admin-no.png"), QIcon(":/Resources/Resources/admin-ok.png")};

    qRegisterMetaType<ResultData>("ResultData");
    qRegisterMetaType<QTextCursor>("QTextCursor");
    qRegisterMetaType<QPointF>("QPointF");
    connect(this, &MainWindow::emit_log, this, &MainWindow::updateLog);
    connect(this, &MainWindow::emit_runCamera, this, &MainWindow::onRunCameraRecv);

    QList<QString> headers = m_headers;
    headers.insert(0, "");
    headers.append("");
    v_logView = new HistoryLabelView(ui.scrollArea, headers);
    connect(v_logView, &HistoryLabelView::emit_text_dir, this, &MainWindow::updateHistoryLoad);
    newHBoxLayout(ui.widget_scrollArea, {v_logView}, 0, {2, 5, 2, 5}, Qt::AlignLeft | Qt::AlignTop);

    v_systemUsedView = new SystemUsedWidget(this);
    v_systemUsedView->setFixedHeight(30);
    ui.statusBar_used->addWidget(v_systemUsedView);
    m_timerUsed = new QTimer(this);
    connect(m_timerUsed, &QTimer::timeout, this, &MainWindow::updateTime);
    m_timerUsed->start(1000);

    connect(this, &MainWindow::resultReady, this, &MainWindow::onWorkerResultReady);
    connect(ui.toolButton_run, &QToolButton::clicked, this, &MainWindow::runOnce);
    connect(ui.toolButton_param, &QToolButton::clicked, this, &MainWindow::showParamView);
    connect(ui.toolButton_draw, &QToolButton::clicked, this, &MainWindow::showDrawView);
    connect(ui.toolButton_sql, &QToolButton::clicked, this, &MainWindow::showHistoryView);

    connect(ui.toolButton_dir, &QToolButton::clicked, this, &MainWindow::openResultDir);
    connect(ui.cb_save_jiaozheng_image, &QToolButton::clicked, this, &MainWindow::enableSaveJiaoZhengImage);
    connect(ui.cb_save_result_image, &QToolButton::clicked, this, &MainWindow::enableSaveResultImage);
    connect(ui.cb_save_origin_image, &QToolButton::clicked, this, &MainWindow::enableSaveOriginImage);
    connect(ui.cb_disp_result, &QToolButton::clicked, this, &MainWindow::enableDisplayResult);

    // initialize devices
    QTimer::singleShot(100, [this]() {
        showLoading(true, tr("正在初始化..."));
        initDevices();
    });
}

// show loading implemetation
void MainWindow::showLoading(bool loading, const QString &text)
{
    if (!m_loadingWidget)
    {
        m_loadingWidget = new QProgressDialog(this);
        m_loadingWidget->setModal(true);
        m_loadingWidget->setRange(0, 0);
        m_loadingWidget->setWindowFlag(Qt::FramelessWindowHint);
    }

    m_loadingWidget->setVisible(loading);
    m_loadingWidget->setLabelText(text);
    if (m_loadingWidget && this->isVisible())
    {
        QPoint center = this->frameGeometry().center();
        m_loadingWidget->move(center - m_loadingWidget->rect().center());
    }
}
void MainWindow::initDevices()
{
    /////////
    if (!MVSCamera::enumDevice(m_cameras_labels))
    {
        QMessageBox::critical(this, "错误", "相机枚举失败, 请重启软件...");
        return;
    }
    int camera_counts = m_cameras_labels.size();
    if (camera_counts <= 0)
    {
        QMessageBox::critical(this, "错误", "相机未连接, 请重启软件...");
        return;
    }
    for (int i = 0; i < v_views.count(); ++i)
    {
        ui.v_layout_view_1->addWidget(v_views[i]);
        if (i < camera_counts)
        {
            v_views[i]->show();
        }
        else
        {
            v_views[i]->hide();
        }
    }
    QList<int> ips;
    for (int i = 0; i < camera_counts; ++i)
    {
        QString ip_str = QString::fromStdString(m_cameras_labels[i][5]);
        QStringList ipss = ip_str.split(".");
        ips.append(ipss[2].toInt());
    }
    for (int i = 0; i < camera_counts; ++i)
    {
        auto mvsCam = QSharedPointer<MVSCamera>(new MVSCamera);
        mvsCam->sendmsg(log_callback, this);
        int nRet = mvsCam->connectCamera(i);
        if (nRet != MV_OK)
        {
            updateLog(tr("连接相机[%1]失败").arg(i));
            continue;
        }

        updateLog(tr("连接相机[%1]成功").arg(i));
        auto infoss = m_cameras_labels[i];
        ui.comboBox->addItem(
            ">> 相机 " + QString::number(i + 1) + " : " +
            QString::fromStdString(infoss[0] + "(" + infoss[1] + " " + infoss[2] + "    " + infoss[5] + ")"));

        m_mvsCameras.append(mvsCam);
    }
    //////////
    for (int i = 0; i < m_mvsCameras.size(); ++i)
    {
        WorkThread *th = new WorkThread(this);
        th->setCamera(i, m_mvsCameras[i].data());
        connect(th, &WorkThread::stateChanged, [this, i](WorkThread::State state) {
            QMetaObject::invokeMethod(this, [this, i, state]() { onWorkerStateChanged(state, i); });
        });
        connect(th, &WorkThread::resultReady, this, &MainWindow::resultReady);
        connect(th, &WorkThread::finished, th, &WorkThread::deleteLater);
        m_threads.append(th);
    }

    /////////
    for (int i = 0; i < m_mvsCameras.count(); ++i)
    {
        m_threads[i]->setCamera(i, m_mvsCameras[i].data());
        m_threads[i]->start();
    }
}

void MainWindow::updateUIEnabled(bool is_enabled)
{
    ui.toolBar->setEnabled(is_enabled);
    ui.statusBar->setEnabled(is_enabled);
    ui.textEdit->setReadOnly(!is_enabled);
}

void MainWindow::updateTime()
{
    QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    ui.label_13->setText(currentTime);

    v_systemUsedView->updateShowMem();
}

////////////////
void MainWindow::onWorkerResultReady(ResultData res_one)
{
    try
    {
        v_views[res_one.camid]->DispImage(res_one.qimage);

        if (!m_totalCounts.is_software)
        {
            bool is_ok = false;
            for (const auto &re : res_one.results)
            {
                is_ok = is_ok && (re == "OK");
            }
            m_totalCounts.total++;
            if (is_ok)
            {
                m_totalCounts.ok++;
                ui.label_res->setPixmap(v_pixs[0]);
            }
            else
            {
                m_totalCounts.ng++;
                ui.label_res->setPixmap(v_pixs[1]);
            }
            v_labels_total[0]->setText(QString::number(m_totalCounts.total));
            v_labels_total[1]->setText(QString::number(m_totalCounts.ok));
            v_labels_total[2]->setText(QString::number(m_totalCounts.ng));
            v_labels_total[3]->setText(QString::number(m_totalCounts.ok * 100.0 / m_totalCounts.total, 'f', 1) + "%");
            v_labels_total[4]->setText(QString::number(m_totalCounts.ng * 100.0 / m_totalCounts.total, 'f', 1) + "%");
        }
    }
    catch (const std::exception &e)
    {
        qCritical() << "MainWindow::onWorkerResultReady Error:  " << e.what() << "  \n";
    }
}

void MainWindow::onWorkerStateChanged(WorkThread::State state, int index)
{
    qInfo() << "onWorkerStateChanged" << state.toString();

    switch (state.status)
    {
    case WorkThread::State::Idle:
        updateLog(state.message);
        // showLoading(false, state.message);
        break;
    case WorkThread::State::Initializing: {
        updateLog(state.message);
        showLoading(true, state.message);
        break;
    }
    case WorkThread::State::Running:
        updateLog(state.message);
        ui.cb_save_jiaozheng_image->setChecked(m_threads[index]->isSaveJiaoZhengImage());
        ui.cb_save_result_image->setChecked(m_threads[index]->isSaveResultImage());
        ui.cb_save_origin_image->setChecked(m_threads[index]->isSaveOriginImage());
        showLoading(false, state.message);
        break;
    case WorkThread::State::Paused:
        updateLog(tr("相机[%1]已暂停：%2").arg(index).arg(state.message));
        showLoading(false, state.message);
        break;
    case WorkThread::State::Error:
        updateLog(tr("相机[%1]出错：%2").arg(index).arg(state.message));
        showLoading(false, state.message);

        // show error dialog
        if (QMessageBox::critical(this, tr("错误"), tr("相机[%1]出错：%2\n是否继续？").arg(index).arg(state.message),
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
        {
            close();
        }
        else
        {
            showLoading(false, state.message);
            m_threads[index]->resume();
        }
        break;
    case WorkThread::State::Stopped:
        close();
        break;
    }
}
void MainWindow::updateHistoryLoad(QString dir)
{
}

void MainWindow::softTrigger()
{
    for (int i = 0; i < m_mvsCameras.count(); ++i)
    {
        qDebug() << "MainWindow::softTrigger" << i;
        m_mvsCameras[i]->softTrigger(i);
    }
}
void MainWindow::onRunCameraRecv(bool is_run)
{
    m_totalCounts.is_software = is_run;
    if (is_run)
    {
        ui.toolButton_run->setIcon(v_icons[1]); //  show stop icon
        ui.toolButton_run->setEnabled(false);
        softTrigger();
        QTimer::singleShot(100, [&]() { runOnce(false); });
    }
    else
    {
        ui.toolButton_run->setEnabled(true);
        ui.toolButton_run->setIcon(v_icons[0]); // show run icon
    }
}
void MainWindow::runOnce(bool checked)
{
    ui.toolButton_run->setEnabled(false);

    if (checked)
    {
        qDebug() << "start run";
        QtConcurrent::run([=] {
            m_totalCounts.is_software = true;
            bool success = false;
            for (int i = 0; i < m_mvsCameras.count(); ++i)
            {
                success = m_mvsCameras[i]->stopGrabbing(i) == MV_OK;
                success = m_mvsCameras[i]->SetTriggerMode(i, MV_TRIGGER_MODE_ON);
                success = m_mvsCameras[i]->SetTriggerSource(i, MV_TRIGGER_SOURCE_SOFTWARE);
                QThread::msleep(100);
                success = m_mvsCameras[i]->startGrabbing(i) == MV_OK;
            }

            QMetaObject::invokeMethod(this, [this]() {
                ui.toolButton_run->setIcon(v_icons[1]); //  show stop icon
                ui.toolButton_run->setEnabled(false);
                softTrigger();
                QTimer::singleShot(500, [&]() { runOnce(false); });
            });
        });
    }
    else
    {
        qDebug() << "stop run";
        QtConcurrent::run([=] {
            // emit emit_runCamera(false);
            bool success = false;
            for (int i = 0; i < m_mvsCameras.count(); ++i)
            {
                success = m_mvsCameras[i]->stopGrabbing(i) == MV_OK;
                success = m_mvsCameras[i]->SetTriggerMode(i, MV_TRIGGER_MODE_ON);
                success = m_mvsCameras[i]->SetTriggerSource(i, MV_TRIGGER_SOURCE_LINE0);
                QThread::msleep(100);
                success = m_mvsCameras[i]->startGrabbing(i) == MV_OK;
            }
            m_totalCounts.is_software = false;

            QMetaObject::invokeMethod(this, [this]() {
                ui.toolButton_run->setEnabled(true);
                ui.toolButton_run->setChecked(false);
                ui.toolButton_run->setIcon(v_icons[0]); // show run icon
            });
        });
    }
}

////////////
void MainWindow::onTemplateScoreChanged(int camid, int index, double value)
{
    m_threads[camid]->set_score(index, value);
    m_threads[camid]->saveConfig();
}

void MainWindow::onSaveModeChanged(int save_type, int camid, int index)
{
    if (save_type == 0) // save one
    {
        m_threads[camid]->setSaveRoiIndex(index, true);
    }
    else if (save_type == 1) // save all
    {
        auto temp = m_threads[camid]->getSaveRoiIndexs();
        for (int i = 0; i < temp.count(); ++i)
        {
            m_threads[camid]->setSaveRoiIndex(temp[i], true);
        }
    }
    else
    {
        m_threads[camid]->set_bold(index);
    }
}
void MainWindow::showParamView()
{
    if (!v_paramView)
    {
        v_paramView = new MyParamsView(this);
        connect(v_paramView, &MyParamsView::saveModeChanged, this, &MainWindow::onSaveModeChanged);
        connect(v_paramView, &MyParamsView::scoreChanged, this, &MainWindow::onTemplateScoreChanged);
        connect(v_paramView, &MyParamsView::meanThreshChanged, [&](int camid, double value) {
            qDebug() << "meanThreshChanged" << camid << value;
            m_threads[camid]->setMeanThresh(value);
        });
        v_paramView->hide();
        for (int i = 0; i < m_threads.count(); ++i)
        {
            v_paramView->addListWidget(i, m_threads[i]->scores());
            v_paramView->addMeanThreshWidget(i, m_threads[i]->meanThresh());
        }
    }

    if (!v_paramView->isVisible())
    {
        QRect desktop = QApplication::desktop()->screenGeometry(0);
        auto pos = ui.widget_view_1->mapToGlobal(QPoint(ui.widget_view_1->rect().width(), 0));
        v_paramView->move(pos.x() + 20, desktop.center().y() - v_paramView->rect().center().y());
        v_paramView->show();
    }
}

void MainWindow::showDrawView()
{
    if (!m_drawView)
    {
        m_drawView = new EditSolution();
        connect(m_drawView, &EditSolution::captureRequested, [this]() {
            qDebug() << "callback  DrawView::getQImage";
            runOnce(true);
        });

        connect(this, &MainWindow::resultReady, [this](ResultData data) {
            if (m_drawView.isNull())
                return;
            m_drawView->setImage(m_threads[data.camid]->getQImage());
        });
    }

    showView(m_drawView.data(), tr("编辑方案"));
}

void MainWindow::showHistoryView()
{
    if (!m_historyView)
    {
        m_historyView = new HistoryView();
    }

    showView(m_historyView.data(), tr("历史记录"));
}

void MainWindow::openResultDir()
{
    QString path = Globals::dataDir();
    QProcess::startDetached("xdg-open", {path});
}

/*
void MainWindow::on_comboBox_activated(int index)
{

    if (index < 0 || index >= v_views.size()) {
        qWarning() << "Invalid floor index:" << index;
        return;
    }

    //  如果切换的是当前视图，直接返回
    if (currentView == v_views[index]) {
        return;
    }
    if (index == 0){
        QtConcurrent::run([=]{
            m_mvsCameras[1]->StopGrabbing();
            QThread::msleep(50);
            m_mvsCameras[0]->StartGrabbing();
        });
        ui.label_fps->show();
        ui.label_fps_2->hide();
        ui.label_fps_2->setText("0");
    }
    else if (index == 1){
        QtConcurrent::run([=]{
            m_mvsCameras[0]->StopGrabbing();
            QThread::msleep(50);
            m_mvsCameras[1]->StartGrabbing();
        });

        ui.label_fps->hide();
        ui.label_fps->setText("0");
        ui.label_fps_2->show();
    }
    for(int m=0; m<v_labels_widgets.count(); ++m){
        if (m == index){
            v_labels_widgets[m]->show();
        }
        else{
            v_labels_widgets[m]->hide();
        }
    }


    //  相机视图切换
    if (currentView) {
        ui.v_layout_view_1->removeWidget(currentView);
        currentView->hide();
    }

    currentView = v_views[index];
    if (currentView) {
        ui.v_layout_view_1->addWidget(currentView);
        currentView->show();
        ui.v_layout_view_1->setContentsMargins(0, 0, 0, 0);
    } else {
        qCritical() << "Camera view at index" << index << "is null";
    }

}
*/

void MainWindow::enableDisplayResult(int arg1)
{
    // TODO: 启动关闭显示结果
}

void MainWindow::enableSaveOriginImage(int arg1)
{
    for (int i = 0; i < m_threads.size(); ++i)
    {
        m_threads[i]->setSaveOriginImage(arg1 == Qt::CheckState::Checked);
        m_threads[i]->saveConfig();
    }
}

void MainWindow::enableSaveResultImage(int arg1)
{
    for (int i = 0; i < m_threads.size(); ++i)
    {
        m_threads[i]->setSaveResultImage(arg1 == Qt::CheckState::Checked);
        m_threads[i]->saveConfig();
    }
}

void MainWindow::enableSaveJiaoZhengImage(int arg1)
{
    for (int i = 0; i < m_threads.size(); ++i)
    {
        m_threads[i]->setSaveJiaoZhengImage(arg1 == Qt::CheckState::Checked);
        m_threads[i]->saveConfig();
    }
}

void MainWindow::showView(QWidget *view, QString title)
{
    qDebug() << "ShowView: view=" << view << ", title=" << title;
    if (!view)
    {
        qWarning() << "view is null";
        return;
    }

    if (!view->isVisible())
    {
        BaseDialog *dlg = new BaseDialog(title, view);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->show();
    }
    else
    {
        view->parentWidget()->show();
        view->parentWidget()->raise();
    }
}
