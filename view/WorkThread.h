#pragma once

#include <QDebug>
#include <QMutex>
#include <QMutexLocker>
#include <QThread>
#include <QThreadPool>
#include <QtConcurrent>

#include <Halcon.h>
#include <HalconCpp.h>
#include <atomic>
#include <qchar.h>
#include <qdatetime.h>
#include <qhash.h>
#include <qimage.h>
#include <qjsonobject.h>
#include <qlist.h>
#include <qvariant.h>

#include "Interface.hpp"
#include "ObjectType.hpp"
#include "commonZack.h"
#include "json.h"

#include "MVSCamera.h"
#include "opencv2/core/mat.hpp"
#include "ringbuffer.h"

using namespace cv;
using namespace HalconCpp;

static cv::Mat HObject2Mat(HObject Hobj)
{
    HTuple htCh;
    HString cType;
    cv::Mat Image;
    ConvertImageType(Hobj, &Hobj, "byte");
    CountChannels(Hobj, &htCh);
    Hlong wid = 0;
    Hlong hgt = 0;
    if (htCh[0].I() == 1)
    {
        HImage hImg(Hobj);
        void *ptr = hImg.GetImagePointer1(&cType, &wid, &hgt); // GetImagePointer1(Hobj, &ptr, &cType, &wid, &hgt);
        int W = wid;
        int H = hgt;
        Image.create(H, W, CV_8UC1);
        unsigned char *pdata = static_cast<unsigned char *>(ptr);
        memcpy(Image.data, pdata, W * H);
    }
    else if (htCh[0].I() == 3)
    {
        void *Rptr;
        void *Gptr;
        void *Bptr;
        HImage hImg(Hobj);
        hImg.GetImagePointer3(&Rptr, &Gptr, &Bptr, &cType, &wid, &hgt);
        int W = wid;
        int H = hgt;
        Image.create(H, W, CV_8UC3);
        vector<cv::Mat> VecM(3);

        VecM[0] = cv::Mat(H, W, CV_8UC1, Bptr);
        VecM[1] = cv::Mat(H, W, CV_8UC1, Gptr);
        VecM[2] = cv::Mat(H, W, CV_8UC1, Rptr);

        cv::merge(VecM, Image);
    }
    return Image;
}

static HObject Mat2HObject(cv::Mat image)
{
    HObject Hobj = HObject();
    int hgt = image.rows;
    int wid = image.cols;
    int i;
    if (image.type() == CV_8UC3)
    {
        vector<cv::Mat> imgchannel;
        split(image, imgchannel);
        cv::Mat imgB = imgchannel[0];
        cv::Mat imgG = imgchannel[1];
        cv::Mat imgR = imgchannel[2];
        uchar *dataR = new uchar[hgt * wid];
        uchar *dataG = new uchar[hgt * wid];
        uchar *dataB = new uchar[hgt * wid];
        for (i = 0; i < hgt; i++)
        {
            memcpy(dataR + wid * i, imgR.data + imgR.step * i, wid);
            memcpy(dataG + wid * i, imgG.data + imgG.step * i, wid);
            memcpy(dataB + wid * i, imgB.data + imgB.step * i, wid);
        }
        GenImage3(&Hobj, "byte", wid, hgt, (Hlong)dataR, (Hlong)dataG, (Hlong)dataB);
        delete[] dataR;
        delete[] dataG;
        delete[] dataB;
        dataR = NULL;
        dataG = NULL;
        dataB = NULL;
    }
    else if (image.type() == CV_8UC1)
    {
        uchar *data = new uchar[hgt * wid];
        for (i = 0; i < hgt; i++)
            memcpy(data + wid * i, image.data + image.step * i, wid);
        GenImage1(&Hobj, "byte", wid, hgt, (Hlong)data);
        delete[] data;
        data = NULL;
    }
    else
    {
        vector<cv::Mat> imgchannel;
        split(image, imgchannel);
        cv::Mat imgB = imgchannel[0];
        cv::Mat imgG = imgchannel[1];
        cv::Mat imgR = imgchannel[2];
        uchar *dataR = new uchar[hgt * wid];
        uchar *dataG = new uchar[hgt * wid];
        uchar *dataB = new uchar[hgt * wid];
        for (i = 0; i < hgt; i++)
        {
            memcpy(dataR + wid * i, imgR.data + imgR.step * i, wid);
            memcpy(dataG + wid * i, imgG.data + imgG.step * i, wid);
            memcpy(dataB + wid * i, imgB.data + imgB.step * i, wid);
        }
        GenImage3(&Hobj, "byte", wid, hgt, (Hlong)dataR, (Hlong)dataG, (Hlong)dataB);
        delete[] dataR;
        delete[] dataG;
        delete[] dataB;
        dataR = NULL;
        dataG = NULL;
        dataB = NULL;
    }
    return Hobj;
}

static int ImgTrans(const HTuple &ModelID, const cv::Mat &InputImg, cv::Mat &OutImg)
{
    try
    {
        HObject ho_Img = Mat2HObject(InputImg);
        HTuple width, height;
        GetImageSize(ho_Img, &width, &height);
        HObject grayImg;
        Rgb1ToGray(ho_Img, &grayImg);
        HTuple Row, Column, Angle, Score;
        FindNccModel(grayImg, ModelID, HTuple(-10).TupleRad(), HTuple(20).TupleRad(), 0.5, 1, 0.5, "true", 0, &Row,
                     &Column, &Angle, &Score);
        int rowCount = Row.TupleLength().I();
        if (rowCount == 1)
        {
            HTuple modelRow = 1750.61;
            HTuple modelCol = 2877.81;
            HTuple mat2D;
            VectorAngleToRigid(Row, Column, Angle, modelRow, modelCol, 0, &mat2D);
            HObject imgAffine;
            AffineTransImage(ho_Img, &imgAffine, mat2D, "constant", "false");
            ZoomImageSize(imgAffine, &imgAffine, width, height, "constant");
            OutImg = HObject2Mat(imgAffine);
        }
        else
        {
            OutImg = InputImg;
        }
        return 0;
    }
    catch (...)
    {
        OutImg = InputImg;
        return -1;
    }
}

struct TemplateData
{
    QString label;
    QString id;
    double rotation;
    QString shape_type;
    QVector<QPoint> points; // 使用 QPointF 存储坐标点

    // 重载构造函数来简化创建
    TemplateData(QString lbl, QString identifier, double rot, QString shape, QVector<QPoint> pts)
        : label(lbl), id(identifier), rotation(rot), shape_type(shape), points(pts)
    {
    }
};

struct ResultData
{
    int camid;
    int pushNum;
    QString SN;
    //    double fps;
    QImage qimage;

    QList<QString> results;
    QString path;
};

struct ResultDir
{
    QString m_saveDir;
    QString m_saveDir_cur_org;
    QString m_saveDir_cur_res;
    QString m_saveDir_cur_ng;
    QString m_saveDir_cur_tmp;
    QString m_saveDir_cur;
};

class WorkThread : public QThread
{
    Q_OBJECT

  public:
    struct State
    {
        enum Status
        {
            Idle,         // 空闲
            Initializing, // 初始化中
            Running,      // 运行中
            Paused,       // 暂停
            Error,        // 错误状态
            Stopped       // 已停止
        };

        Status status = Idle;
        QString message;
        QString toString() const
        {
            return QString("Status: %1, Message: %2").arg(static_cast<int>(status)).arg(message);
        }

        static State idle()
        {
            return {Status::Idle, ""};
        }
    };

    State state() const
    {
        return state_;
    }

    void pause()
    {
        if (state_.status == State::Running)
        {
            setState({.status = State::Paused, .message = "暂停执行"});
        }
    }

    void resume()
    {
        if (state_.status == State::Paused || state_.status == State::Idle)
        {
            setState({.status = State::Running, .message = "恢复执行"});
        }
    }

  private:
    void setState(State state)
    {
        if (state_.status != state.status || state_.message != state.message)
        {
            state_ = state;
            emit stateChanged(state);
        }
    }
    State state_ = State::idle();
    std::atomic<int> reconnectCount_{0};
    std::atomic_int heartbeatTimeout_ = 1000;
    std::atomic_int pauseInterval_ = 3000;

  public:
    WorkThread(QObject *parent = nullptr);
    ~WorkThread();

    void setCamera(int camid, MVSCamera *camOne);
    static void createDir();
    inline static ResultDir g_resultDir;
    inline static bool g_isStepOne;

    void setSaveRoiIndexs(const QHash<int, bool> &indexs);
    inline void setSaveRoiIndex(int index, bool isSave)
    {
        m_saveRoiIndexs[index] = isSave;
        qDebug() << "setSaveRoiIndexs" << index << isSave;
    }
    inline QHash<int, bool> getSaveRoiIndexs() const
    {
        return m_saveRoiIndexs;
    }
    void set_scores(const QList<double> &values);
    void set_score(int index, double value);
    const QList<double> scores() const
    {
        return m_scores;
    }

    void set_bold(int index);
    void setSaveJiaoZhengImage(bool isSave)
    {
        m_isSaveJiaozheng = isSave;
        qDebug() << "setSaveJiaoZhengImage" << isSave;
    }
    
    bool isSaveJiaoZhengImage() const
    {
        return m_isSaveJiaozheng;
    }
    void setSaveOriginImage(bool isSave)
    {
        m_isSaveOrigin = isSave;
        qDebug() << "setOriginImage" << isSave;
    }
    bool isSaveOriginImage() const
    {
        return m_isSaveOrigin;
    }

    void setSaveResultImage(bool isSave)
    {
        m_saveResult = isSave;
        qDebug() << "setOriginImage" << isSave;
    }
    bool isSaveResultImage() const
    {
        return m_saveResult;
    }

    void setMeanThresh(float value)
    {
        m_meanThresh = value;
    }
    float meanThresh() const
    {
        return m_meanThresh;
    }

    void setGrabTimeout(int timeoutMs)
    {
        qDebug() << "setGrabTimeout" << timeoutMs;
        grabTimeout_ = timeoutMs;
    }
    int grabTimeout() const
    {
        return grabTimeout_;
    }

    QImage getQImage();

    void run() override;
    void stop();

    void saveResult();

    // void loadConfig(const QString& path);
    void saveConfig();

  signals:
    void resultReady(ResultData result);
    void stateChanged(State state);

  private:

    std::atomic_int32_t grabTimeout_{1000};
    RingBuffer<FrameData> queque_;
    int m_camid;
    int m_template_ID;
    MVSCamera *m_camera;
    bool m_saveResult = true;
    bool m_isSaveOrigin = false;
    bool m_isSaveJiaozheng = false;
  int m_meanThresh = 0;

    QImage m_qimage;

    Quectel_Infer m_infer;
    QMutex m_mutex;
    ThreadPool m_pool;
    int m_cameraWidth = 0;
    int m_cameraHeight = 0;
    bool m_isAlgoInit = false;
    int m_flip_mode = -1;

    QMap<int, QList<std::vector<float>>> g_features_templates_index;
    QMap<int, QList<std::vector<float>>> g_features_templates_index_FFF;
    QMap<int, QList<QString>> g_features_paths_index;
    QList<TemplateData> g_templateDatas;
    QMap<int, QString> g_features_dir;

    bool m_isInitOK;
    HTuple m_modelID;
    int m_width_match;
    int m_height_match;
    QList<cv::Mat> m_mats_template;
    QList<cv::Rect> m_rects_template;
    QList<QRect> m_qrects_template;
    QList<QPolygon> m_qpolygons_template;
    QList<double> m_scores;
    std::vector<std::vector<cv::Point>> m_pts_all;
    QHash<int, bool> m_saveRoiIndexs;

    QList<QPen> m_pens_ok;
    QList<QPen> m_pens_ng;
    QList<QPen> m_pens;
    QList<bool> m_isSaveOne;
    int64_t m_saveOrigin20 = 0;

    QJsonObject config_;
    QString templateConfigPath_;

    bool initAlgo();
    bool loadTemplateFeatures(int camid);
    bool loadTemplates(int camid, QList<TemplateData> &templateDataList);

    cv::Mat process(const cv::Mat &frame, int top, int bottom, int left, int right, double roi_k);
    void saveImage(cv::Mat src, QImage dst, QString path_org, QString path_res, QList<QString> path_roi,
                   QList<cv::Mat> ng_rois);
    void saveImageNG(QImage dst, QString path_res, QList<QString> path_roi, QList<cv::Mat> ng_rois);
    void saveImageNG2(cv::Mat dst, QString path_res);

    void saveROI(QList<QPair<int, cv::Mat>> rois, int frameNum);
    void saveROIAll(cv::Mat src, int frameNum);

  public slots:
    void onTemplateRecv(int index, QString temp_eng, QString temp_chn);
    void onFlipRecv(int mode);

  private:
    static constexpr int MAX_RETRIES = 5;      // 最大重试次数
    static constexpr int BASE_DELAY_MS = 1000; // 基础重试间隔
    bool heartbeat();
    bool handleReconnect();

  signals:
    void emit_qimage(QImage image);
    void configChanged(const QString &key, const QVariant &value);
};
