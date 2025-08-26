
#include "WorkThread.h"
#include "CameraParams.h"
#include "MVSCamera.h"
#include "MvErrorDefine.h"
#include "appcontext.h"
#include "commonZack.h"
#include "globals.h"
#include "inspectionrecordrepo.h"
#include <algorithm>
#include <cassert>
#include <exception>
#include <qalgorithms.h>
#include <qapplication.h>
#include <qchar.h>
#include <qdatetime.h>
#include <qexception.h>
#include <qglobal.h>
#include <qimage.h>
#include <qjsonarray.h>
#include <qstandardpaths.h>
#include <qtconcurrentrun.h>
#include <qthread.h>
#include <qtimer.h>
#include <thread>

WorkThread::WorkThread(QObject *parent) : QThread(parent), m_pool(4, 2, 90), queque_(5)
{
    createDir();

    QPen pen_ok(QColor(0, 200, 0));
    QPen pen_ng(Qt::red);
    QPen pen_ok2(QColor(0, 200, 0));
    QPen pen_ng2(QColor(255, 0, 0));
    pen_ok.setWidth(4 * 3);
    pen_ng.setWidth(4 * 3);
    pen_ok2.setWidth(12 * 2);
    pen_ng2.setWidth(12 * 2);
    m_pens.append(pen_ok);
    m_pens.append(pen_ng);
    m_pens.append(pen_ok2);
    m_pens.append(pen_ng2);

    m_isInitOK = false;
}

WorkThread::~WorkThread()
{
    m_isInitOK = false;
    stop();
    qDebug() << "WorkThread::~WorkThread()" << m_camid;
}

void WorkThread::setCamera(int camid, MVSCamera *camOne)
{
    m_camid = camid;
    m_template_ID = m_camid;
    m_camera = camOne;
    m_camera->setFrameCallback([this](FrameData frame) {
        qDebug() << "Frame received, camid:" << m_camid << "frameNum:" << frame.frameNum
                 << "timestamp:" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        if (!queque_.canPush())
        {
            qWarning() << "Frame buffer overflow! Dropped, camid:" << m_camid << "frameNum:" << frame.frameNum;
            return;
        }

        if (!frame.data)
        {
            qDebug() << "Frame data is null! Dropped, camid:" << m_camid << "frameNum:" << frame.frameNum;
            return;
        }

        if (frame.camid != m_camid)
        {
            // 丢弃其他相机的帧
            qWarning() << "Frame from other camera, camid:" << frame.camid << "frameNum:" << frame.frameNum
                       << "dropped";
            return;
        }

        if (queque_.push(frame, std::chrono::milliseconds(0)))
        {
            qInfo() << "push image to queque, camid:" << m_camid << "frameNum:" << frame.frameNum
                    << "queue size:" << queque_.size();
        }
    });

    m_camera->setErrorCallback([this](int nMsgType) {
        if (nMsgType == MV_EXCEPTION_DEV_DISCONNECT)
        {
            heartbeat();
        }
    });
}

void WorkThread::createDir()
{
    g_resultDir.m_saveDir = Globals::dataDir();
    g_resultDir.m_saveDir_cur = Globals::dailyDataDir();
    QString m_saveDir_cur_log = Globals::getOrCreateFolder(g_resultDir.m_saveDir_cur, "Logs/");
    g_resultDir.m_saveDir_cur_org = Globals::getOrCreateFolder(g_resultDir.m_saveDir_cur, "Origin");
    g_resultDir.m_saveDir_cur_res = Globals::getOrCreateFolder(g_resultDir.m_saveDir_cur, "Result");
    g_resultDir.m_saveDir_cur_tmp = Globals::getOrCreateFolder(g_resultDir.m_saveDir_cur, "Temp");
    g_resultDir.m_saveDir_cur_ng = Globals::getOrCreateFolder(g_resultDir.m_saveDir_cur, "ng_ROI");
}

void WorkThread::saveConfig()
{
    writeJson(config_, templateConfigPath_);
}

bool WorkThread::initAlgo()
{
    m_isInitOK = false;
    if (m_camera->getWidth(m_camid, m_cameraWidth) != MV_OK)
    {
        setState({.status = State::Error, .message = tr("获取相机分辨率宽度失败")});
        return false;
    }
    if (m_camera->getHeight(m_camid, m_cameraHeight) != MV_OK)
    {
        setState({.status = State::Error, .message = tr("获取相机分辨率高度失败")});
        return false;
    }

    Json::Reader reader;
    Json::Value config;
    auto modelJsonPath = Globals::dataDir() + "/models/model.json";
    if (!QFile::exists(modelJsonPath))
    {
        qCritical() << "Model json file not exists:" << modelJsonPath;
        setState({.status = State::Error, .message = tr("模型配置文件不存在")});
        return false;
    }
    std::ifstream is(modelJsonPath.toStdString());
    if (!reader.parse(is, config))
    {
        qCritical() << "Failed to parse model.json";
        setState({.status = State::Error, .message = tr("模型配置文件解析失败")});
        return false;
    }

    for (auto &item : config["inference"])
    {
        item["Ftc.v1_model"]["model_dir"] = QString(Globals::dataDir() + "/models/").toStdString();
    }
    int flag = m_infer.initial_inference(config);
    if (0 != flag)
    {
        setState({.status = State::Error, .message = tr("模型初始化失败")});
        return false;
    }

    if (!loadTemplates(m_template_ID, g_templateDatas))
    {
        setState({.status = State::Error, .message = tr("加载模板数据失败")});
        return false;
    }

    if (!loadTemplateFeatures(m_camid))
    {
        setState({.status = State::Error, .message = tr("加载模板特征数据失败")});
        return false;
    }

    m_isInitOK = true;
    return true;
}

bool WorkThread::loadTemplates(int camid, QList<TemplateData> &templateDataList)
{
    m_pts_all.clear();
    m_mats_template.clear();
    m_rects_template.clear();
    m_qrects_template.clear();
    m_qpolygons_template.clear();

    QString path = Globals::dataDir() + "/models/templates/template" + QString::number(m_camid) + "/template.json";
    if (!QFile::exists(path))
    {
        qCritical() << "template.json not exists: " << path;
        return false;
    }
    QJsonObject jsonObject = readJson(path);
    config_ = jsonObject;
    templateConfigPath_ = path;

    QJsonArray shapesArray = jsonObject["shapes"].toArray();

    int cnts_load = 0;
    for (const QJsonValue &shapeValue : shapesArray)
    {
        QJsonObject shapeObject = shapeValue.toObject();

        // 获取各个字段
        QString label = shapeObject["label"].toString();
        QString id = shapeObject["id"].toString();
        double rotation = shapeObject["rotation"].toDouble();
        QString shape_type = shapeObject["shape_type"].toString();

        // 获取 points 数组并转换为 QList<QPointF>
        QVector<QPoint> points;
        std::vector<cv::Point> pts;
        cv::Mat img = cv::Mat::zeros(m_cameraHeight, m_cameraWidth, CV_8UC3);
        int x_min = 10000, x_max = 0, y_min = 10000, y_max = 0;

        QJsonArray pointsArray = shapeObject["points"].toArray();
        for (const QJsonValue &pointValue : pointsArray)
        {
            QJsonArray pointArray = pointValue.toArray();
            double x = pointArray.at(0).toDouble();
            double y = pointArray.at(1).toDouble();

            int xx = int(x);
            int yy = int(y);
            x_min = std::min(xx, x_min);
            x_max = std::max(xx, x_max);
            y_min = std::min(yy, y_min);
            y_max = std::max(yy, y_max);

            points.append(QPoint(x, y));
            pts.emplace_back(cv::Point(int(x), int(y)));
        }
        if (x_max >= m_cameraWidth || y_max >= m_cameraHeight)
        {
            continue;
        }
        cnts_load++;
        m_pts_all.emplace_back(pts);

        cv::fillPoly(img, std::vector<std::vector<cv::Point>>{pts}, cv::Scalar(255, 255, 255));
        m_mats_template.append(img);

        cv::Rect rect = cv::Rect(x_min, y_min, x_max - x_min, y_max - y_min);
        m_rects_template.append(rect);

        m_qrects_template.append(QRect(x_min, y_min, x_max - x_min, y_max - y_min));
        QPolygon polygon(points);
        m_qpolygons_template.append(polygon);

        // 创建 TemplateData 并添加到列表
        TemplateData data(label, id, rotation, shape_type, points);
        templateDataList.append(data);
    }

    // 解析 scores
    QList<double> scores;
    QJsonArray scoresArray = jsonObject["a_scores"].toArray();
    for (const QJsonValue &scoreValue : scoresArray)
    {
        scores.append(scoreValue.toDouble());
    }

    if (scores.count() < templateDataList.count())
    {
        qWarning() << "scores count less than template count, fill with 0.5";
        scores.reserve(templateDataList.count());
        qFill(scores, 0.5);
    }
    qInfo() << "scores: " << scores;
    set_scores(scores);

    //
    setSaveJiaoZhengImage(jsonObject.value("a_jiaoz").toBool(false));
    setSaveOriginImage(jsonObject.value("a_save_origin").toBool(false));
    setSaveResultImage(jsonObject.value("a_save_result").toBool(false));
    setMeanThresh(jsonObject.value("a_mean_thresh").toDouble(50.0f));

    setState({.status = State::Idle, .message = tr("successfully loaded %1 regions").arg(templateDataList.count())});
    return true;
}

bool WorkThread::loadTemplateFeatures(int camid)
{
    setState({.status = State::Idle, .message = "loading templates"});

    g_features_paths_index.clear();
    g_features_templates_index.clear();
    g_features_templates_index_FFF.clear();
    const QDir templateDir(Globals::dataDir() + "/models/templates/template" + QString::number(camid));

    if (!templateDir.exists())
    {
        qCritical() << "Template directory does not exist:" << templateDir.absolutePath();
        setState({.status = State::Error, .message = "Template directory does not exist."});
        return false;
    }

    QString templatePath = templateDir.absoluteFilePath("ModelID.ncm");
    try
    {
        ReadNccModel(HTuple(templatePath.toLatin1().data()), &m_modelID);
        qInfo() << "Loaded template from: " << templatePath << " successfully!";
    }
    catch (const HOperatorException &e)
    {
        qCritical() << "Failed to load template from: " << templatePath << "!" << e.ErrorMessage().ToUtf8();
        setState({.status = State::Error,
                  .message = tr("Failed to load template, Error: %1").arg(e.ErrorMessage().ToUtf8())});
        return false;
    }

    QFileInfoList subDirs = templateDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    int count = subDirs.count();
    int idx = 0;
    foreach (const QFileInfo &subDirInfo, subDirs)
    {
        QString folder_name = subDirInfo.fileName();
        qDebug() << "Loading templates from: " << folder_name;

        int index = folder_name.split("_").at(0).toInt();
        QDir subDir(subDirInfo.absoluteFilePath());
        bool is_TTT = true;
        if (folder_name.contains("F"))
        {
            if (!g_features_templates_index_FFF.keys().contains(index))
            {
                g_features_templates_index_FFF[index] = QList<std::vector<float>>();
            }
            is_TTT = false;
        }
        else
        {
            if (!g_features_paths_index.keys().contains(index))
            {
                g_features_paths_index[index] = QList<QString>();
                g_features_templates_index[index] = QList<std::vector<float>>();
            }
            g_features_dir[index] = subDirInfo.absoluteFilePath();
        }

        // 获取子目录下所有文件
        QFileInfoList files =
            subDir.entryInfoList(QStringList() << "*.jpg" << "*.png" << "*.bmp" << "*.jpeg", QDir::Files);
        foreach (const QFileInfo &fileInfo, files)
        {
            QString path = fileInfo.absoluteFilePath();
            cv::Mat img = cv::imread(path.toStdString());

            std::vector<float> feature(768);
            m_infer.inference(img.data, img.rows, img.cols, img.channels(), feature.data());

            if (is_TTT)
            {
                g_features_paths_index[index].append(path);
                g_features_templates_index[index].append(feature);
            }
            else
            {
                g_features_templates_index_FFF[index].append(feature);
            }
        }
        idx++;
        setState({.status = State::Idle,
                  .message = QString("load template: %1/%2  %3").arg(idx).arg(count).arg(files.count())});
    }

    setState({.status = State::Idle, .message = "load template done"});
    return true;
}

void WorkThread::setSaveRoiIndexs(const QHash<int, bool> &indexs)
{
    m_saveRoiIndexs = indexs;
    qInfo() << "保存ROI列表更新, 相机id=%1" << m_camid << ", 索引列表: " << indexs;
    setState({.status = State::Idle, .message = tr("保存ROI列表更新, 相机id=%1").arg(m_camid)});
}

void WorkThread::set_scores(const QList<double> &value)
{
    m_scores = value;
    m_pens_ok.clear();
    m_pens_ng.clear();
    for (int m = 0; m < m_scores.count(); ++m)
    {
        m_pens_ok.append(m_pens.at(0));
        m_pens_ng.append(m_pens.at(1));
    }
}
void WorkThread::set_score(int index, double value)
{
    // 如果索引超出范围，则返回
    if (index < 0 || index >= m_scores.count())
    {
        qCritical() << "set_score: Index out of range: " << index;
        return;
    }

    m_scores[index] = value;
    setState(
        {.status = State::Idle,
         .message = tr("Update: camid=%1, index=%2, score=").arg(m_camid).arg(index) + QString::number(value, 'f', 3)});
}

void WorkThread::set_bold(int index)
{
    if (m_pens_ok.count() != m_scores.count())
    {
        throw std::out_of_range("set_bold: Pens ok count not equal to score count");
    }
    if (m_pens_ng.count() != m_scores.count())
    {
        throw std::out_of_range("set_bold: Pens ng count not equal to score count");
    }
    if (index >= m_scores.count())
    {
        qCritical() << "set_bold: Index out of range: " << index;
        return;
    }

    for (int m = 0; m < m_scores.count(); ++m)
    {
        if (index < 0)
        {
            m_pens_ok[m] = m_pens.at(0);
            m_pens_ng[m] = m_pens.at(1);
        }
        else
        {
            if (m == index)
            {
                m_pens_ok[m] = m_pens.at(2);
                m_pens_ng[m] = m_pens.at(3);
            }
            else
            {
                m_pens_ok[m] = m_pens.at(0);
                m_pens_ng[m] = m_pens.at(1);
            }
        }
    }
}

QImage WorkThread::getQImage()
{
    return m_qimage;
}

void WorkThread::onTemplateRecv(int index, QString temp_eng, QString temp_chn)
{
    static bool is_first = true;
    m_template_ID = index;
    if (is_first)
    {
        is_first = false;
    }
    else
    {
        m_isAlgoInit = true;
    }
}

void WorkThread::onFlipRecv(int mode)
{
    m_flip_mode = mode;
    QString name_mode;
    if (m_flip_mode == -1)
    {
        name_mode = "不翻转";
    }
    if (m_flip_mode == 0)
    {
        name_mode = "上下翻转";
    }
    else if (m_flip_mode == 1)
    {
        name_mode = "旋转180°";
    }
    else if (m_flip_mode == 2)
    {
    }
    else
    {
    }
    qDebug() << "onFlipRecv" << name_mode;
}

// void notifyFailure(CameraError error) {
//     switch (error.level()) {
//         case CRITICAL:
//             playAlarmSound();
//             sendSMSAlert();
//             break;
//         case WARNING:
//             systemTray.showMessage(tr("Camera Warning"), error.message());
//             break;
//     }
//     logToDatabase(error);
// }

// check for camera errors

bool WorkThread::handleReconnect()
{
    if (reconnectCount_ >= MAX_RETRIES)
    {
        setState({.status = State::Error,
                  .message = tr("相机[%1] 连接丢失, 超过最大重试次数%2").arg(m_camid).arg(MAX_RETRIES)});
        return false;
    }

    // 指数退避算法
    const int delayMs = BASE_DELAY_MS * (1 << reconnectCount_);
    QThread::msleep(delayMs);

    // 重连相机
    m_camera->stopGrabbing(m_camid);
    m_camera->closeCamera(m_camid);
    if (m_camera->connectCamera(m_camid) != MV_OK)
    {
        setState(
            {.status = State::Paused,
             .message = tr("相机[%1] 连接丢失, 重试中(%2/%3)").arg(m_camid).arg(reconnectCount_ + 1).arg(MAX_RETRIES)});
        ++reconnectCount_;
        return false;
    }

    if (m_camera->startGrabbing(m_camid) != MV_OK)
    {
        setState(
            {.status = State::Paused,
             .message = tr("相机[%1] 采集失败, 重试中(%2/%3)").arg(m_camid).arg(reconnectCount_ + 1).arg(MAX_RETRIES)});
        ++reconnectCount_;
        return false;
    }

    reconnectCount_ = 0;
    return true;
};

bool WorkThread::heartbeat()
{
    qDebug() << "heartbeat" << m_camid << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    if (isInterruptionRequested())
    {
        qInfo() << "线程已请求中断" << m_camid;
        return false;
    }
    // 检查连接状态
    if (!m_camera->isConnected(m_camid) || !m_camera->isGrabbing(m_camid))
    {
        state_.status = State::Paused;
        if (!handleReconnect())
            return false;
        else
            setState({.status = State::Running, .message = tr("相机[%1] 重连成功").arg(m_camid)});
    }

    if (!m_isInitOK)
    {
        state_.status = State::Paused;
        if (!initAlgo())
            return false;
        else
            setState({.status = State::Running, .message = tr("相机[%1] 初始化算法成功").arg(m_camid)});
    }

    return true;
}

void WorkThread::stop()
{
    qInfo() << "Requesting stop work thread, camid:" << m_camid;
    requestInterruption();
    qDebug() << "Requested interruption";
    quit();
    qDebug() << "Called quit";
    queque_.clear();
    qDebug() << "Queue cleared";
}

void WorkThread::run()
{
    qInfo() << "WorkThread[" << this << "] started at" << QTime::currentTime().toString("hh:mm:ss.zzz");
    setState({.status = State::Initializing, .message = tr("正在初始化")});

    QTime prevTime;
    double fps;
    int64_t frameNum = 0;
    cv::Rect reg(600, 1400, 300, 300);

    QFont font("MicroSoft YaHei", 25 * 3);
    QPoint pt_gap(0, 10);

    if (!m_camera)
    {
        qCritical() << "WorkThread[" << this << "] invalid camera reference";
        setState({.status = State::Error, .message = tr("无效相机引用")});
        assert(m_camera);
        return;
    }

    if (!initAlgo())
    {
        qCritical() << "WorkThread[" << this << "] init algo failed";
    }

    QTimer timer;
    timer.setInterval(heartbeatTimeout_);
    connect(&timer, &QTimer::timeout, this, &WorkThread::heartbeat);

    setState({.status = State::Running, .message = "正在执行"});
    while (!isInterruptionRequested())
    {
        try
        {

            // pause state, error state handling
            if (state_.status == State::Paused || state_.status == State::Error)
            {
                qDebug() << "WorkThread[" << this << "] paused"
                         << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
                QThread::msleep(pauseInterval_);
                continue;
            }

            if (state_.status == State::Initializing)
            {
                qDebug() << "WorkThread[" << this << "] initializing"
                         << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
                QThread::msleep(pauseInterval_);
                continue;
            }

            FrameData camData;
            if (!queque_.pop(camData, std::chrono::milliseconds(5000)))
            {
                qDebug() << "WorkThread[" << this << "] no data, waiting"
                         << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
                continue;
            }
            qDebug() << "WorkThread[" << this << "] got data"
                     << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

            QString SN = Int2QString(camData.pushNum, 6);
            QString tt = QDateTime::fromMSecsSinceEpoch(camData.timestamp).toString("hh-mm-ss-zzz");

            QTime t_start = QTime::currentTime();
            frameNum++;

            cv::Mat mat_img, mat_rgb, mat_dst, mat_match;
            switch (camData.pixelType)
            {
            case PixelType_Gvsp_Mono8:
                mat_img = cv::Mat(camData.height, camData.width, CV_8UC1, camData.data);
                cv::cvtColor(mat_img, mat_rgb, cv::COLOR_GRAY2RGB);
                break;
            case PixelType_Gvsp_BayerGR8: // dushen
                mat_img = cv::Mat(camData.height, camData.width, CV_8UC1, camData.data);
                cv::cvtColor(mat_img, mat_rgb, cv::COLOR_BayerGR2RGB);
                break;
            case PixelType_Gvsp_BayerRG8: //////// usb
                mat_img = cv::Mat(camData.height, camData.width, CV_8UC1, camData.data);
                cv::cvtColor(mat_img, mat_rgb, cv::COLOR_BayerRG2RGB);
                break;
            case PixelType_Gvsp_BayerGB8:
                mat_img = cv::Mat(camData.height, camData.width, CV_8UC1, camData.data);
                cv::cvtColor(mat_img, mat_rgb, cv::COLOR_BayerGB2RGB);
                break;
            case PixelType_Gvsp_BayerBG8:
                mat_img = cv::Mat(camData.height, camData.width, CV_8UC1, camData.data);
                cv::cvtColor(mat_img, mat_rgb, cv::COLOR_BayerBG2RGB);
                break;
            }

            mat_dst = mat_rgb;
            if (camData.pushNum < 2)
            {
                m_saveOrigin20 = 0;
            }

            cv::Mat region = mat_dst(reg).clone();
            if (mat_dst.channels() == 1)
            {
                cv::cvtColor(mat_dst, mat_dst, COLOR_GRAY2BGR);
            }
            else
            {
                cv::cvtColor(region, region, COLOR_BGR2GRAY);
            }

            double mean_v = cv::mean(region)[0];
            if (mean_v <= m_meanThresh)
            {
                g_isStepOne = true;
                qCritical() << QString("camid=%1, pushNum=%2, mean=%3 < %4, continue...")
                                   .arg(m_camid)
                                   .arg(camData.pushNum)
                                   .arg(mean_v)
                                   .arg(m_meanThresh);

                if (m_isSaveJiaozheng)
                {
                    QString path_jiaoz =
                        QString("%1/%2_JiaoZ_%3.png").arg(g_resultDir.m_saveDir_cur_org).arg(SN).arg(tt);
                    cv::Mat img_jiaoz = mat_dst.clone();
                    m_pool.enqueue1(&WorkThread::saveImageNG2, this, std::move(img_jiaoz), std::move(path_jiaoz));
                }
                continue;
            }

            if (m_flip_mode > 0)
            {
                if (m_flip_mode == 0)
                {
                    cv::flip(mat_dst, mat_dst, 0);
                }
                else if (m_flip_mode == 1)
                {
                    cv::rotate(mat_dst, mat_dst, cv::ROTATE_180);
                }
            }

            QList<int> match_ok;
            QList<int> match_ng;
            QList<float> match_scores;
            QList<QString> results_isOK;
            QList<QPair<int, cv::Mat>> rois_ng_total;
            QList<cv::Mat> rois_ng;
            QList<QString> rois_paths;

            ResultData result;
            result.SN = Int2QString(camData.pushNum, 6);
            result.pushNum = camData.pushNum;
            result.camid = m_camid;
            result.results = results_isOK;

            if (!m_isInitOK)
            {
                result.qimage = Mat2QImage(mat_dst);
                resultReady(result);
                continue;
            }

            // trans image
            try
            {
                if (ImgTrans(m_modelID, mat_rgb, mat_dst) != 0)
                {
                    qCritical() << "ImgTrans error";
                }
                else
                {
                    qInfo() << QString("camid=%1, pushNum=%2, 图像校正成功").arg(m_camid).arg(camData.pushNum);
                }
            }
            catch (const HException &e)
            {
                qCritical() << "图像校正失败, 错误信息:" << e.ErrorMessage();
                continue;
            }

            for (int i = 0; i < m_mats_template.count(); ++i)
            {
                if (mat_dst.rows != m_mats_template.at(i).rows || mat_dst.cols != m_mats_template.at(i).cols)
                {
                    qDebug() << "相机尺寸和模板尺寸不一致, 正在适配...";
                    cv::resize(mat_dst, mat_dst, cv::Size(m_cameraWidth, m_cameraHeight));
                }
                // 模板匹配
                cv::bitwise_and(mat_dst, m_mats_template.at(i), mat_match);
                cv::Rect rect = m_rects_template.at(i);
                cv::Mat roi = mat_match(rect).clone();
                if (roi.empty())
                {
                    qDebug() << "roi is empty. continue";
                    match_ok.append(i);
                    results_isOK.append("OK");
                    match_scores.append(0.666);
                    continue;
                }

                float feature[768];
                m_infer.inference(roi.data, roi.rows, roi.cols, roi.channels(), feature);

                float score_max = 0.0f;
                for (int m = 0; m < g_features_templates_index.value(i).count(); ++m)
                {
                    int id = -1;
                    float score = 0.0f;
                    // TODO: 可以一次性比对多个特征
                    FeatureCompare(g_features_templates_index[i][m].data(), 1, feature, 0.8, id, score);
                    score_max = std::max(score_max, score);
                }

                float score_max_FFF = 0.0f;
                if (!g_features_templates_index_FFF[i].isEmpty())
                {
                    for (int m = 0; m < g_features_templates_index_FFF[i].count(); ++m)
                    {
                        int id = -1;
                        float score = 0.0f;
                        FeatureCompare(g_features_templates_index_FFF[i][m].data(), 1, feature, 0.8, id, score);
                        score_max_FFF = std::max(score_max_FFF, score);
                    }
                }

                if (score_max_FFF < m_scores.at(i) && score_max >= m_scores.at(i))
                {
                    match_ok.append(i);
                    results_isOK.append("OK");
                }
                else
                {
                    match_ng.append(i);
                    results_isOK.append("NG");

                    QString path_one = QString("%1/%2_%3.png").arg(g_resultDir.m_saveDir_cur_ng).arg(SN).arg(i);
                    rois_paths.append(path_one);
                    rois_ng.append(roi);
                }

                if (m_saveRoiIndexs.contains(i))
                {
                    m_saveRoiIndexs[i] = false;
                    rois_ng_total.append(QPair(i, roi.clone()));
                }

                match_scores.append(score_max);
            } // match templates for end

            QTime t_end1 = QTime::currentTime();
            if (!rois_ng_total.empty())
            {
                m_pool.enqueue1(&WorkThread::saveROI, this, std::move(rois_ng_total), camData.frameNum);
            }

            /////// draw
            QImage qimage = Mat2QImage(mat_dst);
            if (qimage.isNull())
            {
                qInfo() << QString("Convert Mat to QImage failed. camid:%1, pushNum:%2")
                               .arg(camData.camid)
                               .arg(camData.pushNum);
                continue;
            }

            m_qimage = qimage.copy();
            QPainter painter(&qimage);
            painter.setFont(font);
            for (const auto &i : match_ok)
            {
                painter.setPen(m_pens_ok.at(i));
                painter.drawRect(m_qrects_template.at(i));
                //                painter.drawPolygon(m_qpolygons_template.at(idx));
                QPoint pt = i == 2 ? (m_qrects_template.at(i).bottomLeft() + QPoint(0, 80))
                                   : (m_qrects_template.at(i).topLeft() - pt_gap);
                painter.drawText(pt, QString("%1:OK,").arg(i + 1) + QString::number(match_scores.at(i), 'f', 2));
            }

            for (const auto &idx : match_ng)
            {
                painter.setPen(m_pens_ng.at(idx));
                painter.drawRect(m_qrects_template.at(idx));
                //                painter.drawPolygon(m_qpolygons_template.at(idx));
                QPoint pt = idx == 2 ? (m_qrects_template.at(idx).bottomLeft() + QPoint(0, 80))
                                     : (m_qrects_template.at(idx).topLeft() - pt_gap);
                painter.drawText(pt, QString("%1:NG,").arg(idx + 1) + QString::number(match_scores.at(idx), 'f', 2));
            }
            painter.end();

            // collect results
            result.qimage = qimage.copy();
            result.results = results_isOK;

            QString res;
            for (int i = 0; i < results_isOK.count(); ++i)
            {
                res += (results_isOK.at(i) + "+");
            }
            if (res.length() > 0)
            {
                res.remove(res.length() - 1);
            }
            QString path_res =
                QString("%1/%2_%3_%4.png").arg(g_resultDir.m_saveDir_cur_res).arg(result.SN).arg(res).arg(tt);
            result.path = path_res;
            qDebug() << "save result path_res:" << path_res;

            // 通知结果
            emit resultReady(result);
            qInfo() << QString("camid=%1, pushNum=%2, 4-1.send result to Main...").arg(m_camid).arg(result.pushNum);

            /// save records
            InspectionRecord record = {
                // .id =
                .sn = result.SN,
                .costTime = t_start.msecsTo(QDateTime::currentDateTime().time()),
                .status = result.results.contains("NG") == 0 ? 0 : 1,
                .total = result.results.size(),
                .okCount = result.results.count("OK"),
                .ngCount = result.results.count("NG"),
                .result = result.results.join(", "),
                .path = path_res,
            };

            QtConcurrent ::run([this, record]() {
                if (auto repo = AppContext ::getService<InspectionRecordRepo>())
                {
                    if (!repo->insert(record))
                    {
                        qWarning() << "insert record failed,  cam id:" << m_camid << ", record: " << record.toString();
                    }
                }
            });

            // 保存结果图
            if (m_saveResult)
            {
                m_pool.enqueue1(&WorkThread::saveImageNG, this, std::move(qimage), std::move(path_res),
                                std::move(rois_paths), std::move(rois_ng));
            }

            // 保存原图到本地
            if (m_isSaveOrigin)
            {
                QString path_org =
                    QString("%1/%2_Org_%3.png").arg(g_resultDir.m_saveDir_cur_org).arg(result.SN).arg(tt);
                qDebug() << "save origin path_org:" << path_org;
                m_pool.enqueue1(&WorkThread::saveImage, this, std::move(mat_rgb), std::move(qimage),
                                std::move(path_org), std::move(path_res), std::move(rois_paths), std::move(rois_ng));
            }

            // 保存校正图片
            if (m_isSaveJiaozheng)
            {
                QString path_jiaoz =
                    QString("%1/%2_JiaoZ_%3.png").arg(g_resultDir.m_saveDir_cur_tmp).arg(result.SN).arg(tt);
                qDebug() << "校正图像保存路径 path_jiaoz:" << path_jiaoz;
                cv::Mat img_jiaoz = mat_dst.clone();
                m_pool.enqueue1(&WorkThread::saveImageNG2, this, std::move(img_jiaoz), std::move(path_jiaoz));
            }

            int duration = t_start.msecsTo(t_end1);
            int duration1 = t_start.msecsTo(QTime::currentTime());
            setState({.status = State::Running,
                      .message = tr(">> camid=%1, recv=%2, infer time: %3ms), duration time=%4ms")
                                     .arg(m_camid)
                                     .arg(frameNum)
                                     .arg(duration)
                                     .arg(duration1)});

        } // end of try
        catch (const std::exception &e)
        {
            qCritical() << "WorkThread[" << this << "] exception:" << e.what();
            setState({.status = State::Error, .message = tr("异常:  %1").arg(e.what())});
        }
    } // end of while

    // stop timer
    timer.stop();
    disconnect(&timer, &QTimer::timeout, this, nullptr);

    // stop camera
    m_camera->stopGrabbing(m_camid);

    setState({.status = State::Idle, .message = tr("相机[%1] 停止成功").arg(m_camid)});
    qInfo() << "WorkThread[" << this << "] stopped at" << QTime::currentTime().toString("hh:mm:ss.zzz");
}

void WorkThread::saveResult()
{
}
void WorkThread::saveROI(QList<QPair<int, cv::Mat>> rois, int frameNum)
{
    for (int m = 0; m < rois.count(); ++m)
    {
        int index = rois.at(m).first;
        QString dir = g_features_dir.value(index).endsWith("/") ? g_features_dir.value(index)
                                                                : (g_features_dir.value(index) + "/");
        QString path = QString("%1%2_%3.png")
                           .arg(dir)
                           .arg(frameNum, 6, 10, QChar('0'))
                           .arg(QTime::currentTime().toString("hh-mm-ss-zzz"));
        if (!rois.at(m).second.empty())
        {
            cv::imwrite(path.toStdString(), rois.at(m).second);
        }
    }
}

void WorkThread::saveROIAll(cv::Mat src, int frameNum)
{
    for (int m = 0; m < m_pts_all.size(); ++m)
    {
        const auto &contour = m_pts_all.at(m);
        cv::Rect rect = cv::boundingRect(contour);
        cv::Mat mask = cv::Mat::zeros(src.size(), CV_8UC3);
        std::vector<std::vector<cv::Point>> pts = {contour};
        cv::fillPoly(mask, pts, cv::Scalar(255, 255, 255));
        cv::Mat dst;
        cv::bitwise_and(src, mask, dst);
        cv::Mat roi = dst(rect).clone();

        QString dir = g_features_dir.value(m).endsWith("/") ? g_features_dir.value(m) : (g_features_dir.value(m) + "/");
        QString path = QString("%1%2_%3.png")
                           .arg(dir)
                           .arg(frameNum, 6, 10, QChar('0'))
                           .arg(QTime::currentTime().toString("hh-mm-ss-zzz"));
        cv::imwrite(path.toStdString(), roi);
    }
}

void WorkThread::saveImage(cv::Mat src, QImage dst, QString path_org, QString path_res, QList<QString> path_roi,
                           QList<cv::Mat> ng_rois)
{
    for (int m = 0; m < path_roi.count(); ++m)
    {
        cv::imwrite(path_roi.at(m).toStdString(), ng_rois.at(m));
    }
    m_saveOrigin20++;
    if (m_saveOrigin20 > 20)
    {
        cv::imwrite(path_org.toStdString(), src);
    }
    dst.save(path_res);
}

void WorkThread::saveImageNG(QImage dst, QString path_res, QList<QString> path_roi, QList<cv::Mat> ng_rois)
{
    for (int m = 0; m < path_roi.count(); ++m)
    {
        if (!ng_rois.at(m).empty())
        {
            cv::imwrite(path_roi.at(m).toStdString(), ng_rois.at(m));
        }
    }
    if (!dst.isNull())
    {
        dst.save(path_res);
    }
}
void WorkThread::saveImageNG2(cv::Mat dst, QString path_res)
{
    if (!dst.empty())
    {
        cv::imwrite(path_res.toStdString(), dst);
    }
}
