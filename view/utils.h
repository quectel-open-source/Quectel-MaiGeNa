#pragma once
#include "opencv2/core/mat.hpp"
#include "opencv2/imgproc.hpp"
#include <QIODevice>
#include <QJsonObject>
#include <QString>
#include <functional>
#include <qchar.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qimage.h>
#include <qjsondocument.h>
#include <qlist.h>
#include <qpoint.h>
#include <type_traits>
namespace Utils
{
// load json
inline QJsonObject loadJson(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return QJsonObject();
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError)
    {
        return QJsonObject();
    }
    return doc.object();
}

// save json
inline bool saveJson(const QString &filePath, const QJsonObject &json)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return false;
    }
    QJsonDocument doc(json);
    auto bdata = doc.toJson();
    auto sz = file.write(bdata);
    return sz == bdata.size();
}

inline bool qImageToCvMat(const QImage &image, cv::Mat &dst, bool copy = true)
{
    cv::Mat mat;
    switch (image.format())
    {
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
        mat = cv::Mat(image.height(), image.width(), CV_8UC4, const_cast<uchar *>(image.bits()), image.bytesPerLine());
        cv::cvtColor(mat, mat, cv::COLOR_BGRA2BGR);
        break;
    case QImage::Format_RGB888:
        mat = cv::Mat(image.height(), image.width(), CV_8UC3, const_cast<uchar *>(image.bits()), image.bytesPerLine());
        cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR);
        break;
    case QImage::Format_Grayscale8:
        mat = cv::Mat(image.height(), image.width(), CV_8UC1, const_cast<uchar *>(image.bits()), image.bytesPerLine());
        break;
    default:
        return false;
        break;
    }

    dst = copy ? mat.clone() : mat;

    return true;
}

template <typename T> QString join(const QList<T> &container, const QString &sep, std::function<T> wrapper = nullptr)
{
    QString result;
    for (auto it = container.begin(); it != container.end(); ++it)
    {
        if (it != container.begin())
            result += sep;

        if (wrapper)
            result += wrapper(*it);
        else
        {
            // 判断是否能转换成字符串，是否是基本数据类型
            if (std::is_arithmetic_v<T>())
                result += QString::number(*it);
            else if (std::is_same_v<T, QString>() || std::is_same_v<T, std::string>)
                result += QString(*it);
            else if (std::is_same_v<T, QChar>())
                result += QString(QChar(*it));
            else if (std::is_same_v<T, QStringList>())
                result += it->join(",");
            else
                result += QString::fromStdString(std::to_string(*it));
        }
    }
    return result;
}

inline QString pointsToString(const QList<QPointF> &points)
{
    QString result = "[";
    for (auto it = points.begin(); it != points.end(); ++it)
    {
        result += QString::number(it->x()) + "," + QString::number(it->y());
        if (it != points.end() - 1)
            result += ",";
    }
    result += "]";
    return result;
}
} // namespace Utils