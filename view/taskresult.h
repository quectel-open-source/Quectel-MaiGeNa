#pragma once
#include "WorkThread.h"
#include <QString>
#include <QDateTime>
#include <QMap>
#include <algorithm>
#include <qdatetime.h>
#include <qimage.h>
#include <qjsonarray.h>
#include <qjsonobject.h>
#include <qmap.h>
#include <qobject.h>
#include <qpair.h>
#include <qpoint.h>
#include <qvector.h>

// 检测结果数据模型
class ResultData1
{
  public:
    enum ResultType
    {
        TASK,     // 任务级别结果
        IMAGE,    // 图像级别结果
        POSITION, // 位置级别结果
        METRIC    // 指标级别结果（最底层）
    };

    ResultData1(ResultType type, const QString &id = "")
        : m_type(type), m_id(id), m_isPass(false), m_timestamp(QDateTime::currentDateTime())
    {
    }

    // 设置基本属性
    void setResult(bool isPass, const QString &message = "")
    {
        m_isPass = isPass;
        m_message = message;
    }
    void addSubResult(ResultData1 result)
    {
        // 类型层级校验
        if ((m_type == TASK && result.type() != IMAGE) || (m_type == IMAGE && result.type() != POSITION) ||
            (m_type == POSITION && result.type() != METRIC))
        {
            throw std::invalid_argument("Invalid result type hierarchy");
        }
        m_subResults.push_back(std::move(result));
    }
    void addMetric(const QString &name, double value, double threshold)
    {
        m_metrics[name] = {value, threshold};
    }
    QPair<double, double> metric(const QString &name) const
    {
        return m_metrics.value(name);
    }

    // 获取属性
    ResultType type() const
    {
        return m_type;
    }
    QString id() const
    {
        return m_id;
    }
    bool isPass() const
    {
        if (m_type == ResultType::METRIC) return m_isPass;
        return std::all_of(m_subResults.begin(), m_subResults.end(),
                           [](const auto &result) { return result.isPass(); });
    }
    QDateTime timestamp() const
    {
        return m_timestamp;
    }
    void setTimestamp(const QDateTime &timestamp)
    {
        m_timestamp = timestamp;
    }
    QList<ResultData1> subResults() const
    {
        return m_subResults;
    }
    QMap<QString, QPair<double, double>> metrics() const
    {
        return m_metrics;
    }

    // 导出为JSON/XML
    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj["type"] = static_cast<int>(m_type);
        obj["id"] = m_id;
        obj["isPass"] = isPass();
        obj["message"] = m_message;

        // 序列化指标数据
        QJsonObject metrics;
        for (auto it = m_metrics.begin(); it != m_metrics.end(); ++it)
        {
            QJsonArray arr;
            arr.append(it.value().first);  // 实际值
            arr.append(it.value().second); // 阈值
            metrics[it.key()] = arr;
        }
        obj["metrics"] = metrics;

        // 递归序列化子结果
        QJsonArray children;
        for (const auto &child : m_subResults)
        {
            children.append(child.toJson());
        }
        obj["subResults"] = children;

        return obj;
    }

  private:
    ResultType m_type;                              // 结果类型
    QString m_id;                                   // 唯一标识符（任务ID/图像ID/位置ID）
    bool m_isPass;                                  // 是否通过检测
    QString m_message;                              // 结果描述
    QDateTime m_timestamp;                          // 检测时间戳
    QList<ResultData1> m_subResults;          // 子结果
    QMap<QString, QPair<double, double>> m_metrics; // 指标名 -> (实际值, 阈值)

    // 
};


// 实现一个类， 包含 string id, string message, int result, timestamp, parameters, 以及相关的函数
