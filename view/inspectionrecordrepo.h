#pragma once

// InspectionRecord  是工业检测记录
// static QList<QString> HISTORY_HEADERS = {"sn", "time", "status", "total", "okCount", "ngCount","result", "path"};
#include "dbmanager.h"
#include <QString>
#include <qchar.h>
#include <qdatetime.h>
#include <qlist.h>
#include <qobject.h>
#include <qobjectdefs.h>
#include <qpair.h>
#include <qvector.h>
#include <utility>

// enum class InspectionStatus {
//     OK = 0,
//     NG = 1,
//     Warning = 2,    // 可返工
//     Suspicious = 3  // 需人工复核
// };

struct InspectionRecord
{
    int id;
    QString sn;   // 检测序列号
    int costTime; // 耗时, 单位毫秒
    int status;   // 0: OK, 1: NG
    int total;
    int okCount;
    int ngCount;
    QString result;                                     // TODO: 后期改用QVariantMap?
    QString path;                                       // 文件路径, 通常是 图片路径
    QDateTime createdAt = QDateTime::currentDateTime(); // 创建时间, 默认当前时间, 通常表示检测结束时间

    QString toString() const
    {
        return QString("status: %1, total: %2, okCount: %3, ngCount: %4, result: %5, path: %6, createdAt: %7")
            .arg(status)
            .arg(total)
            .arg(okCount)
            .arg(ngCount)
            .arg(result)
            .arg(path)
            .arg(createdAt.toString("yyyy-MM-dd hh:mm:ss"));
    }
};

class InspectionRecordRepo : public QObject
{
    Q_OBJECT

  public:
    inline static QString TABLE_NAME = "inspection_record";
    inline static QStringList COLUMN_NAMES = {"sn",      "costTime", "status", "total",    "okCount",
                                              "ngCount", "result",   "path",   "createdAt"};

  private:
    DBManager *m_db;

  signals:
    void recordInserted(InspectionRecord record);
    void recordUpdated(InspectionRecord record);
    void recordRemoved(int id);

  public:
    InspectionRecordRepo(DBManager *db) : m_db(db)
    {
        if (!m_db->open())
        {
            qWarning() << "Failed to open database";
            return;
        }

        // 创建表
        QString sql = "CREATE TABLE IF NOT EXISTS " + TABLE_NAME +
                      " ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "sn VARCHAR(100) NOT NULL UNIQUE, "
                      "costTime INTEGER, "
                      "status INTEGER, "
                      "total INTEGER, "
                      "okCount INTEGER, "
                      "ngCount INTEGER, "
                      "path TEXT, "
                      "result TEXT, "
                      "createdAt TEXT " // DEFAULT datetime('now', 'localtime')
                      " )";

        if (!m_db->execute(sql))
        {
            qWarning() << "Failed to create table:" << TABLE_NAME;
        }
    }
    ~InspectionRecordRepo()
    {
        qDebug() << "InspectionRecordRepo::~InspectionRecordRepo()";
    }

    bool insert(InspectionRecord record)
    {
        QString sql = "INSERT INTO " + TABLE_NAME;
        sql += " (" + COLUMN_NAMES.join(",") + ")";
        sql += " VALUES (" + QStringList::fromVector(QVector<QString>(COLUMN_NAMES.size(), "?")).join(",") + ")";

        QVariantList params = QVariantList() << record.sn << record.costTime << record.status << record.total
                                             << record.okCount << record.ngCount << record.result << record.path
                                             << record.createdAt.toString("yyyy-MM-dd HH:mm:ss");

        if (COLUMN_NAMES.size() != params.size())
        {
            throw "column size not match";
        }

        if (!m_db->execute(sql, params))
        {
            return false;
        }
        emit recordInserted(std::move(record));
        return true;
    }
    bool update(InspectionRecord record)
    {
        QString sql = "UPDATE " + TABLE_NAME + " SET ";
        sql += COLUMN_NAMES.join("=?,");
        sql += " WHERE id=?";
        QVariantList params = QVariantList()
                              << record.sn << record.costTime << record.status << record.total << record.okCount
                              << record.ngCount << record.result << record.path << record.createdAt << record.id;
        if (COLUMN_NAMES.size() != params.size()-1)
        {
            throw "column size not match";
        }

        if (!m_db->execute(sql, params))
        {
            return false;
        }
        emit recordUpdated(std::move(record));
        return true;
    }
    bool remove(int id)
    {
        if (!m_db->execute("DELETE FROM " + TABLE_NAME + " WHERE id=?", QVariantList() << id))
        {
            return false;
        }
        emit recordRemoved(id);
        return true;
    }
    QList<InspectionRecord> queryAll()
    {
        QList<InspectionRecord> records;
        QSqlQuery query = m_db->query("SELECT * FROM " + TABLE_NAME);
        while (query.next())
        {
            InspectionRecord record = createRecordFromQuery(query);
            records.append(record);
            debugRecord(record);
        }
        return records;
    }

    // 查询, 筛选, 时间范围, 分页, return 总数, 数据
    QPair<int, QList<InspectionRecord>> queryByFilter(const QString &filter, const QDateTime &startDate,
                                                      const QDateTime &endDate, int page = 0, int pageSize = 0)
    {
        QList<InspectionRecord> records;

        QString whereClause = " WHERE 1=1 ";
        if (!filter.isEmpty())
        {
            whereClause += " AND ( ";
            for (auto column : COLUMN_NAMES)
            {
                whereClause += QString(" %1 LIKE '%2' OR ").arg(column).arg(filter);
            }
            whereClause += " 0=1 ";
            whereClause += " )";
        }

        whereClause += QString(" AND createdAt BETWEEN '%1' AND '%2'")
                           .arg(startDate.toString("yyyy-MM-dd HH:mm:ss"))
                           .arg(endDate.toString("yyyy-MM-dd HH:mm:ss"));

        int totalCount = 0;
        QString countQuery = "SELECT COUNT(*) FROM " + TABLE_NAME + whereClause;
        auto countResult = m_db->query(countQuery);
        if (countResult.next())
        {
            totalCount = countResult.value(0).toInt();
        }
        if (totalCount == 0)
            return {0, {}};

        if (pageSize > 0)
        {
            whereClause += QString(" LIMIT %1").arg(pageSize);
        }
        if (page > 0)
        {
            whereClause += QString(" OFFSET %1").arg((page - 1) * pageSize);
        }
        // 构建分页查询
        QString sql = "SELECT * FROM " + TABLE_NAME + whereClause;
        QSqlQuery query = m_db->query(sql);
        while (query.next())
        {
            InspectionRecord record = createRecordFromQuery(query);
            records.append(record);
            debugRecord(record);
        }
        return {totalCount, records};
    }

  private:
    inline void debugRecord(const InspectionRecord &record) const
    {
        qDebug() << "id:" << record.id << "sn:" << record.sn << "time:" << record.costTime << "status:" << record.status
                 << "total:" << record.total << "okCount:" << record.okCount << "ngCount:" << record.ngCount
                 << "result:" << record.result << "path:" << record.path;
    }
    inline InspectionRecord createRecordFromQuery(const QSqlQuery &query) const
    {
        InspectionRecord record;
        record.id = query.value("id").toInt();
        record.sn = query.value("sn").toString();
        record.costTime = query.value("costTime").toInt();
        record.status = query.value("status").toInt();
        record.total = query.value("total").toInt();
        record.okCount = query.value("okCount").toInt();
        record.ngCount = query.value("ngCount").toInt();
        record.result = query.value("result").toString();
        record.path = query.value("path").toString();
        record.createdAt = query.value("createdAt").toDateTime();
        return record;
    }
};
