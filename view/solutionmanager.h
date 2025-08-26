#ifndef SOLUTIONMANAGER_H
#define SOLUTIONMANAGER_H

#include <QColor>
#include <QDateTime>
#include <QDir>
#include <QImage>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QPolygonF>
#include <QString>
#include <opencv2/opencv.hpp>
#include <optional>
#include <qchar.h>
#include <qdir.h>
#include <qglobal.h>
#include <qimage.h>
#include <qjsonobject.h>
#include <qlist.h>
#include <qmap.h>
#include <qpair.h>
#include <qpoint.h>
#include <qvector.h>

// ROI 模板图像结构
struct ROIImage
{
    QString path;            // 图像路径
    QImage image;            // 图像数据
    QVector<float> features; // 特征描述符
};

// ROI 结构
struct ROI
{
    int id;                 // 唯一标识
    QString label;          // 标签， 如果空，则id
    QString type;           // "矩形", "圆形", "多边形"
    QColor color;           // 显示颜色
    int thickness = 3;      // 线宽
    QList<QPointF> points;  // 多边形点集
    cv::Mat mask;           // 遮罩
    double threshold = 0.7; // 匹配阈值
    bool isTTT = true;
    QList<ROIImage> templates; // 模板图像列表
};

// 方案结构
struct Solution
{
    QString id;                  // 唯一标识 (template0, template1, ...)
    QString name;                // 用户友好名称
    QDir path;                   // 路径
    bool isDefault = false;      // 是否默认方案
    bool saveCorrection = false; // 是否保存校正图像
    bool saveOrigin = false;     // 是否保存原始图像
    bool saveResult = true;      // 是否保存结果图像
    int flip = -1;               // 图像翻转设置
    double meanThreshold = 0.0;  // 平均阈值
    QMap<int, ROI> rois;         // ROI列表
    QString imagePath;           // 基础图像
    QImage image;                // 基础图像
    QSize imageSize;             // 图像尺寸
    QDateTime lastModified;      // 最后修改时间

    QString toBasicString() const
    {
        return QString("Solution(id=%1, name=%2, path=%3, isDefault=%4, lastModified=%5)")
            .arg(id)
            .arg(name)
            .arg(path.absolutePath())
            .arg(isDefault)
            .arg(lastModified.toString("yyyy-MM-dd hh:mm:ss"));
    }
    QString toString() const
    {
        return QString(
                   "Solution(id=%1, name=%2, path=%3, isDefault=%4, saveCorrection=%5, saveOrigin=%6, saveResult=%7, "
                   "flip=%8, meanThreshold=%9, rois=%10, imagePath=%11, imageSize=%12, lastModified=%13)")
            .arg(id)
            .arg(name)
            .arg(path.absolutePath())
            .arg(isDefault)
            .arg(saveCorrection)
            .arg(saveOrigin)
            .arg(saveResult)
            .arg(flip)
            .arg(meanThreshold)
            .arg(rois.size())
            .arg(imagePath)
            .arg(QString("%1x%2").arg(imageSize.width()).arg(imageSize.height()))
            .arg(lastModified.toString("yyyy-MM-dd hh:mm:ss"));
    }
};

class SolutionManager : public QObject
{
    Q_OBJECT
  public:
    inline static SolutionManager *instance()
    {
        static SolutionManager instance_;
        return &instance_;
    }

  public:
    explicit SolutionManager(QObject *parent = nullptr);

  public:
    bool loadConfig();
    bool saveConfig();

    // 方案管理
    bool loadSolution(const QString &solutionId);
    bool saveSolution(const QString &solutionId);
    Solution *createSolution(const QString &name, const QImage &baseImage);
    bool deleteSolution(const QString &solutionId);
    bool setDefaultSolution(const QString &solutionId);
    // 获取解决方案
    QList<Solution *> solutions()
    {
        QList<Solution *> sols;
        for (auto it = m_solutions.begin(); it != m_solutions.end(); ++it)
        {
            sols.append(&it.value());
        }
        return sols;
    }
    Solution *getSolution(const QString &solutionId)
    {
        return m_solutions.contains(solutionId) ? &m_solutions[solutionId] : nullptr;
    }

    // ROI 管理
    ROI *getROI(const QString &solutionId, int id)
    {
        if (auto sol = getSolution(solutionId))
        {
            return sol->rois.contains(id) ? &sol->rois[id] : nullptr;
        }
        return nullptr;
    }
    void addROI(const QString &solutionId, ROI &roi)
    {
        if (auto sol = getSolution(solutionId))
        {
            roi.id = sol->rois.isEmpty() ? 0 : sol->rois.lastKey() + 1;
            sol->rois.insert(roi.id, roi);
        }
    }
    void updateROI(const QString &solutionId, const QString &roiId, const ROI &roi);
    void removeROI(const QString &solutionId, const QString &roiId);

    // ROI图像管理
    bool addROIImage(const QString &solutionId, const QString &roiId, const QImage &image);
    bool removeROIImage(const QString &solutionId, const QString &roiId, const QString &imageName);

    // // 特征提取与匹配
    // cv::Mat matchTemplate(const cv::Mat &scene, const QString &solutionId, const QString &roiId);

  signals:
    void solutionsLoaded(QList<Solution *> solutions);
    void solutionLoaded(Solution *);
    void solutionSaved(Solution *);
    void errorOccurred(const QString &message);

  private:
    // 内部方法
    QString getNextSolutionId() const;

    QString getSolutionPath(const QString &solutionId)
    {
        return m_baseDir.filePath(solutionId);
    }
    QString getSolutionConfigPath(const QString &solutionId)
    {
        return QDir(getSolutionPath(solutionId)).filePath("template.json");
    }
    QString getSolutionImagePath(const QString &solutionId)
    {
        return QDir(getSolutionPath(solutionId)).filePath("template.png");
    }
    QString getROIPath(const QString &solutionId, int roiId, bool isTTT = true)
    {
        QString roiDirName = QString("%1_%2").arg(roiId).arg(isTTT ? "T" : "F");
        return QDir(getSolutionPath(solutionId)).filePath(roiDirName);
    }
    QString getROIImagePath(const QString &solutionId, int roiId, int imageIndex)
    {
        return QDir(getROIPath(solutionId, roiId)).filePath(QString::number(imageIndex) + ".png");
    }
    QList<QPair<QString, QImage>> getROIImages(const QString &roiPath);
    cv::Mat extractROIMask(const QList<QPointF> &points, const QSize &size);
    QVector<float> extractFeature(const QImage &image);
    // bool loadFeatures(const QString &path, cv::Mat &features);
    // bool saveFeatures(const QString &path, const cv::Mat &features);

    QDir m_baseDir;
    QString m_configPath;
    QHash<QString, Solution> m_solutions;
    QString m_defaultSolutionId;
};

#endif // SOLUTIONMANAGER_H