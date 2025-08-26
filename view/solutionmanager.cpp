#include "solutionmanager.h"
#include "globals.h"
#include "utils.h"
#include <QDebug>
#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <algorithm>
#include <opencv2/features2d.hpp>
#include <qapplication.h>
#include <qcolor.h>
#include <qdebug.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qglobal.h>
#include <qjsonarray.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qnumeric.h>
#include <qpoint.h>
#include <qvector.h>

SolutionManager::SolutionManager(QObject *parent) : QObject(parent)
{

    m_baseDir = QDir(Globals::getOrCreateFolder(Globals::dataDir(), "models", "templates"));
    m_configPath = m_baseDir.filePath("config.json");
    qDebug() << "SolutionManager::baseDir" << m_baseDir.absolutePath();
    qDebug() << "SolutionManager::configPath" << m_configPath;

    if (!m_baseDir.exists())
    {
        if (!m_baseDir.mkpath("."))
        {
            emit errorOccurred("无法创建解决方案目录: " + m_baseDir.path());
            qCritical() << "无法创建解决方案目录: " << m_baseDir.path();
        }
    }
}

bool SolutionManager::loadConfig()
{
    auto templatesConfig = Utils::loadJson(m_configPath);
    if (templatesConfig.isEmpty())
    {
        qCritical() << "无法加载配置文件: " << m_configPath;
        return saveConfig();
    }
    qDebug() << "加载配置文件成功: " << m_configPath;
    qDebug() << "Loaded config: " << QJsonDocument(templatesConfig).toJson(QJsonDocument::Indented);

    auto templatesArr = templatesConfig["templates"].toArray();
    foreach (auto templateObj, templatesArr)
    {
        Solution obj;
        obj.id = templateObj["id"].toString();
        obj.path = m_baseDir.filePath(templateObj["path"].toString(obj.id)); // 默认是相对路径，所以需要转换成绝对路径
        obj.name = templateObj["name"].toString();
        obj.isDefault = templateObj["is_default"].toBool();
        obj.lastModified = QDateTime::fromString(templateObj["last_modified"].toString(), Qt::ISODate);
        m_solutions[obj.id] = obj;
        qDebug() << " Found Solution: " << obj.toBasicString();
    }

    m_defaultSolutionId = templatesConfig["default"].toString();
    if (m_defaultSolutionId.isEmpty())
    {
        auto it =
            std::find_if(m_solutions.begin(), m_solutions.end(), [](const Solution &obj) { return obj.isDefault; });
        if (it != m_solutions.end())
            m_defaultSolutionId = it->id;
        else if (!m_solutions.isEmpty())
            m_defaultSolutionId = m_solutions.begin()->id;
        else
            m_defaultSolutionId.clear();
    }

    qInfo() << "Found " << m_solutions.size() << " solutions";

    emit solutionsLoaded(solutions());

    return true;
}

bool SolutionManager::saveConfig()
{
    QJsonObject templatesConfig;
    QJsonArray templatesArr;
    templatesConfig["default"] = "";
    foreach (auto sol, m_solutions)
    {
        QJsonObject templateObj;
        templateObj["id"] = sol.id;
        templateObj["name"] = sol.name;
        templateObj["is_default"] = sol.isDefault;
        templateObj["last_modified"] = sol.lastModified.toString(Qt::ISODate);
        templateObj["path"] = sol.path.path();
        if (sol.isDefault)
        {
            templatesConfig["default"] = sol.id;
        }
        templatesArr.append(templateObj);
        qDebug() << "Saving config: " << templateObj;
    }
    templatesConfig["templates"] = templatesArr;
    if (!Utils::saveJson(m_configPath, templatesConfig))
    {
        emit errorOccurred(tr("写入模板配置文件失败"));
        return false;
    }

    qDebug() << "Saved config: " << QJsonDocument(templatesConfig).toJson(QJsonDocument::Indented);
    return true;
}

bool SolutionManager::loadSolution(const QString &solutionId)
{
    qDebug() << "Loading solution: id=" << solutionId;
    if (!m_solutions.contains(solutionId))
    {
        emit errorOccurred(tr("方案不存在！"));
        return false;
    }

    Solution &solution = m_solutions[solutionId];
    qDebug() << "Loading solution: " << solution.toBasicString();

    // 加载templatepath/template.json
    auto templateConfigPath = getSolutionConfigPath(solutionId);
    if (!QFileInfo(templateConfigPath).exists())
    {
        emit errorOccurred(tr("模板配置文件不存在:%1").arg(templateConfigPath));
        qCritical() << "模板配置文件不存在:%1" << templateConfigPath;
        return false;
    }
    auto templateConfig = Utils::loadJson(templateConfigPath);
    qDebug() << "Loaded template config: " << QJsonDocument(templateConfig).toJson(QJsonDocument::Indented);
    if (templateConfig.isEmpty())
    {
        emit errorOccurred(tr("模板配置文件解析失败:%1").arg(templateConfigPath));
        qCritical() << "模板配置文件解析失败:" << templateConfigPath;
        return false;
    }

    // 解析 templateConfig
    solution.name = templateConfig["name"].toString(solution.name.isEmpty() ? solutionId : solution.name);
    solution.isDefault = templateConfig["is_default"].toVariant().toBool();
    solution.flip = templateConfig["flip"].toInt(0);
    solution.saveOrigin = templateConfig["save_correction"].toVariant().toBool();
    solution.saveOrigin = templateConfig["save_origin"].toVariant().toBool();
    solution.saveResult = templateConfig["save_result"].toVariant().toBool();
    solution.meanThreshold = templateConfig["mean_thresh"].toDouble(0);
    solution.imageSize = QSize(templateConfig["image_width"].toInt(0), templateConfig["image_height"].toInt(0));
    solution.imagePath = getSolutionImagePath(solutionId);
    solution.image = QImage(solution.imagePath);

    // 解析shapes
    auto shapesArr = templateConfig["shapes"].toArray();
    for (const QJsonValue &value : shapesArr)
    {
        if (!value.isObject())
        {
            errorOccurred("Invalid template shape");
            continue;
        }
        const QJsonObject &shape = value.toObject();
        ROI roi;
        bool ok;
        roi.id = shape["id"].toVariant().toInt(&ok);
        if (!ok || roi.id < 0)
        {
            errorOccurred(tr("无效ROI ID:") + QString::number(roi.id));
            qCritical() << "Invalid ROI ID:" << roi.id;
            continue;
        }
        roi.type = shape["shape_type"].toString();
        roi.color = QColor(shape["color"].toString("#00FF00"));
        roi.threshold = shape["threshold"].toDouble(0);
        roi.thickness = shape["thickness"].toInt(roi.thickness);
        roi.isTTT = shape["is_ttt"].toBool(true);

        for (const QJsonValue &point : shape["points"].toArray())
        {
            // 检查 point 是否合规
            if (point.toArray().size() != 2)
            {
                qCritical() << "Invalid point:" << point;
                continue;
            }
            roi.points << QPointF(point.toArray()[0].toDouble(), point.toArray()[1].toDouble());
        }

        roi.mask = extractROIMask(roi.points, solution.imageSize);

        solution.rois[roi.id] = roi;
    }

    qDebug() << "Found " << solution.rois.size() << "ROIs";

    // 加载所有模板图像和特征
    for (ROI &roi : solution.rois)
    {
        // 0_T/image_0.png
        // 1_T/image_0.png
        QString roiPath = getROIPath(solutionId, roi.id); //  0_T
        auto roiImages = getROIImages(roiPath);
        foreach (auto &val, roiImages)
        {
            ROIImage roiImage;
            roiImage.path = val.first;
            roiImage.image = val.second;
            roiImage.features = extractFeature(roiImage.image);
            roi.templates.append(roiImage);
        }

        qDebug() << "ROI: id=" << roi.id << ", type=" << roi.type << ", threshold=" << roi.threshold << ", =points"
                 << roi.points << ", templates=" << roi.templates.size();
        solution.rois[roi.id] = roi;
    }

    m_solutions[solutionId] = solution;
    qDebug() << "Loaded solution: " << solution.toString();
    emit solutionLoaded(&solution);
    return true;
}

bool SolutionManager::saveSolution(const QString &solutionId)
{
    qInfo() << "Saving solution: id=" << solutionId;
    auto it = m_solutions.find(solutionId);
    if (it == m_solutions.end())
    {
        emit errorOccurred("保存方案失败，方案不存在: " + solutionId);
        qCritical() << "保存方案失败，方案不存在: " << solutionId;
        return false;
    }
    Solution &solution = it.value();
    solution.path = getSolutionPath(solutionId);
    if (!solution.path.exists())
    {
        qDebug() << "创建路径" << solution.path;
        solution.path.mkpath(".");
    }

    solution.lastModified = QDateTime::currentDateTime();

    // 保存基础图像
    QString baseImagePath = getSolutionImagePath(solutionId);
    if (!solution.image.save(baseImagePath))
    {
        emit errorOccurred("保存基础图像失败: " + baseImagePath);
        qWarning() << "保存基础图像失败: " << baseImagePath;
        // return false;
    }
    solution.imagePath = baseImagePath;

    QJsonObject root;
    root["id"] = solution.id;
    root["name"] = solution.name;
    root["is_default"] = solution.isDefault;
    root["save_correction"] = solution.saveCorrection;
    root["save_origin"] = solution.saveOrigin;
    root["save_result"] = solution.saveResult;
    root["flip"] = solution.flip;
    root["mean_thresh"] = solution.flip;
    root["image_width"] = solution.imageSize.width();
    root["image_height"] = solution.imageSize.height();
    root["image_path"] = solution.path.relativeFilePath(getSolutionImagePath(solutionId));
    root["last_modified"] = solution.lastModified.toString(Qt::ISODate);

    auto roiArr = QJsonArray();
    for (ROI &roi : solution.rois)
    {
        QJsonObject roiJson;
        roiJson["color"] = roi.color.name();
        roiJson["id"] = roi.id;
        roiJson["is_ttt"] = roi.isTTT;
        roiJson["threshold"] = roi.threshold;
        roiJson["shape_type"] = roi.type;
        auto pointsArr = QJsonArray();
        foreach (QPointF point, roi.points)
        {
            pointsArr.append(QJsonArray() << point.x() << point.y());
        }
        roiJson["points"] = pointsArr;

        roiArr.append(roiJson);
    }
    root["shapes"] = roiArr;

    //  保存模板图片
    for (ROI &roi : solution.rois)
    {
        QString roiPath = getROIPath(solutionId, roi.id);
        QDir roiDir(roiPath);
        if (!roiDir.exists())
        {
            roiDir.mkpath(".");
        }

        for (int i = 0; i < roi.templates.size(); i++)
        {
            auto &roiImage = roi.templates[i];
            if (roiImage.path.isEmpty())
            {
                roiImage.path = getROIImagePath(solutionId, roi.id, i);
            }

            if (!roiImage.image.save(roiImage.path))
            {
                emit errorOccurred(tr("无法保存模板图像: ") + roiImage.path);
                qCritical() << "无法保存模板图像: " << roiImage.path;
                continue;
            }

            // TODO: save features?
        }
    }

    if (!Utils::saveJson(getSolutionConfigPath(solutionId), root))
    {
        emit errorOccurred(tr("无法保存解决方案配置: ") + getSolutionConfigPath(solutionId));
        qCritical() << "无法保存解决方案配置: " << getSolutionConfigPath(solutionId);
        return false;
    }

    m_solutions[solutionId] = solution;

    emit solutionSaved(&solution);
    qInfo() << "已保存解决方案: " << solution.toString();
    return true;
}

Solution *SolutionManager::createSolution(const QString &name, const QImage &baseImage)
{
    qInfo() << "Creating solution: " << name << ", baseImage=" << baseImage;
    if (name.isEmpty() || m_solutions.contains(name))
    {
        qCritical() << "无法创建解决方案: " << name;
        errorOccurred(tr("无法创建解决方案: ") + name);
        return nullptr;
    }
    // 创建解决方案对象
    QString solutionId = getNextSolutionId();
    Solution solution;
    solution.id = solutionId;
    solution.path = getSolutionPath(solutionId);
    solution.name = name;
    solution.image = baseImage;
    solution.imagePath = getSolutionImagePath(solutionId);
    solution.imageSize = baseImage.size();
    solution.lastModified = QDateTime::currentDateTime();
    if (m_solutions.empty())
    {
        solution.isDefault = true;
    }

    m_solutions[solutionId] = solution;

    // 保存解决方案
    if (!saveSolution(solutionId))
    {
        qCritical() << "Failed to create and save solution:" << solutionId;
        deleteSolution(solutionId);
        return nullptr;
    }

    // 保存索引
    if (!saveConfig())
    {
        return nullptr;
    }

    emit solutionsLoaded(solutions());

    qInfo() << "已创建解决方案: " << solution.toString();
    return getSolution(solutionId);
}

bool SolutionManager::deleteSolution(const QString &solutionId)
{
    qInfo() << "Deleting solution: " << solutionId;
    // 从内存中移除
    if (!m_solutions.contains(solutionId))
    {
        qWarning() << "删除方案不存在: " << solutionId;
        return true;
    }

    auto sz = m_solutions.remove(solutionId);
    qDebug() << "已删除" << sz << "个方案";

    // 保存索引
    if (!saveConfig())
    {
        return false;
    }

    // 删除文件
    QString solutionPath = getSolutionPath(solutionId);
    QDir solutionDir(solutionPath);
    if (solutionDir.exists())
    {
        if (!solutionDir.removeRecursively())
        {
            emit errorOccurred("无法删除解决方案目录: " + solutionPath);
            qCritical() << "无法删除解决方案目录: " << solutionPath;
            return false;
        }
    }

    qInfo() << "已删除解决方案: " << solutionId;
    return true;
}

bool SolutionManager::setDefaultSolution(const QString &solutionId)
{
    if (!m_solutions.contains(solutionId))
    {
        emit errorOccurred("解决方案不存在: " + solutionId);
        return false;
    }

    m_defaultSolutionId = solutionId;

    // 更新所有解决方案的isDefault标志
    for (auto &solution : m_solutions)
    {
        solution.isDefault = (solution.id == solutionId);
    }

    return saveConfig();
}

cv::Mat SolutionManager::extractROIMask(const QList<QPointF> &points, const QSize &size)
{
    cv::Mat maskMat = cv::Mat::zeros(size.width(), size.height(), CV_8UC3);
    auto pts = std::vector<cv::Point>{};
    foreach (auto &point, points)
    {
        pts.push_back(cv::Point(point.x(), point.y()));
    }
    cv::fillPoly(maskMat, std::vector<std::vector<cv::Point>>{pts}, cv::Scalar(255, 255, 255));
    return maskMat;
}

QVector<float> SolutionManager::extractFeature(const QImage &image)
{
    return QVector<float>();
}

// // 特征匹配
// cv::Mat SolutionManager::matchTemplate(const cv::Mat &scene, const QString &solutionId, const QString &roiId)
// {
//     if (!m_solutions.contains(solutionId))
//     {
//         emit errorOccurred("解决方案不存在: " + solutionId);
//         return cv::Mat();
//     }

//     Solution solution = m_solutions[solutionId];
//     ROI *targetROI = nullptr;

//     // 查找目标ROI
//     for (ROI &roi : solution.rois)
//     {
//         if (roi.id == roiId)
//         {
//             targetROI = &roi;
//             break;
//         }
//     }

//     if (!targetROI || targetROI->templates.isEmpty())
//     {
//         emit errorOccurred("ROI不存在或没有模板图像: " + roiId);
//         return cv::Mat();
//     }

//     // 创建匹配器
//     cv::BFMatcher matcher(cv::NORM_HAMMING);
//     cv::Mat result;

//     // 使用第一个模板进行匹配
//     const ROIImage &templateImg = targetROI->templates.first();

//     // 提取场景特征
//     cv::Mat sceneDescriptors = extractORB(scene);
//     if (sceneDescriptors.empty())
//     {
//         emit errorOccurred("无法提取场景特征");
//         return cv::Mat();
//     }

//     // 匹配特征
//     std::vector<cv::DMatch> matches;
//     matcher.match(templateImg.features, sceneDescriptors, matches);

//     // 绘制匹配结果
//     cv::drawMatches(QImageToMat(templateImg.image), templateImg.keypoints, scene, sceneKeypoints, matches, result);

//     return result;
// }

QString SolutionManager::getNextSolutionId() const
{
    // 生成唯一ID (template0, template1, ...)
    QString solutionId = "template0";
    int index = 0;
    while (m_solutions.contains(solutionId))
    {
        solutionId = "template" + QString::number(++index);
    }
    return solutionId;
}

QList<QPair<QString, QImage>> SolutionManager::getROIImages(const QString &roiPath)
{
    QList<QPair<QString, QImage>> images;

    // 获取ROIPath 文件夹下的全部图像文件
    auto filePaths = QDir(roiPath).entryInfoList(QStringList() << "*.png" << "*.jpg" << "*.jpeg" << "*.bmp",
                                                 QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
    for (auto &filePath : filePaths)
    {
        auto image = QImage(filePath.filePath());
        if (!image.isNull())
        {
            images.append(qMakePair(filePath.filePath(), image));
        }
        else
        {
            qDebug() << "Failed to load image: " << filePath.filePath();
        }
    }

    qDebug() << "Found " << images.size() << " images from " << roiPath;
    return images;
}
