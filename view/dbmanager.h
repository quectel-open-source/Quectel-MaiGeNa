// DBManager.h
#ifndef DBMANAGER_H
#define DBMANAGER_H

#include "globals.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>
#include <memory>
#include <qdatetime.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qobject.h>
#include <qscopedpointer.h>
#include <qsqldatabase.h>

class DBManager : public QObject
{
    Q_OBJECT

  private:
    static inline std::unique_ptr<DBManager> instance_;

  public:
    static DBManager &instance(const QString &dbname = "database")
    {
        if (!instance_)
        {
            instance_.reset(new DBManager(dbname));
        }
        return *instance_;
    }

    explicit DBManager(const QString &dbname = "database", QObject *parent = nullptr) : QObject(parent)
    {
        m_dbPath = QDir::cleanPath(Globals::dataDir() + "/" + QFileInfo(dbname).baseName() + ".db");
        qDebug() << "DBManager::DBManager()" << m_dbPath;
    }

    ~DBManager()
    {
        close();
    }

    QSqlDatabase db() const
    {
        return m_db;
    }

    // 打开数据库连接
    bool open(const QString &databaseName = QString())
    {
        close(); // 关闭现有连接

        if (!databaseName.isEmpty())
        {
            m_dbPath = databaseName;
        }

        m_db = QSqlDatabase::addDatabase("QSQLITE");
        m_db.setDatabaseName(m_dbPath);

        if (!m_db.open())
        {
            qWarning() << "Database open error:" << m_db.lastError().text();
            return false;
        }
        return true;
    }

    // 关闭数据库连接
    void close()
    {
        if (m_db.isOpen())
        {
            QString connectionName = m_db.connectionName();
            m_db.close();
            m_db = QSqlDatabase();
            QSqlDatabase::removeDatabase(connectionName);
        }
    }

    // 执行无返回的SQL语句 (CREATE/INSERT/UPDATE/DELETE)
    bool execute(const QString &query, const QVariantList &params = QVariantList())
    {
        if (!m_db.isOpen())
        {
            qWarning() << "Database not open!";
            return false;
        }

        QSqlQuery q(m_db);
        q.prepare(query);

        for (int i = 0; i < params.size(); ++i)
        {
            q.bindValue(i, params[i]);
        }

        if (!q.exec())
        {
            qWarning() << "Execute error:" << q.lastError().text() << "\nQuery:" << query;
            return false;
        }

        qDebug() << "Query:" << query;
        return true;
    }

    // 执行查询语句 (SELECT)
    QSqlQuery query(const QString &query, const QVariantList &params = QVariantList())
    {
        QSqlQuery q(m_db);
        if (!m_db.isOpen())
        {
            qWarning() << "Database not open!";
            return q;
        }

        q.prepare(query);
        for (int i = 0; i < params.size(); ++i)
        {
            q.bindValue(i, params[i]);
        }

        if (!q.exec())
        {
            qWarning() << "Query error:" << q.lastError().text() << "\nQuery:" << query;
        }
        else
        {
            qDebug() << "Query success:" << query;
        }
        return q;
    }

    // 日期范围查询
    QSqlQuery queryByDateRange(const QString &dateColumn, const QDate &startDate, const QDate &endDate,
                               const QString &additionalFilter = QString())
    {

        QString query = QString("SELECT * FROM products WHERE %1 BETWEEN ? AND ?").arg(dateColumn);

        if (!additionalFilter.isEmpty())
        {
            query += " AND " + additionalFilter;
        }

        QVariantList params;
        params << startDate.toString("yyyy-MM-dd") << endDate.toString("yyyy-MM-dd");

        return this->query(query, params);
    }

    // 日期相等查询
    QSqlQuery queryByDate(const QString &dateColumn, const QDate &date, const QString &additionalFilter = QString());

    // 事务操作
    bool transaction()
    {
        return m_db.transaction();
    }
    bool commit()
    {
        return m_db.commit();
    }
    bool rollback()
    {
        return m_db.rollback();
    }

    // 实用方法
    QString lastError() const
    {
        return m_db.lastError().text();
    }
    bool isOpen() const
    {
        return m_db.isOpen();
    }
    QString databasePath() const
    {
        return m_dbPath;
    }

    // 备份数据库
    bool backup(const QString &backupPath)
    {
        close();
        if (QFile::exists(m_dbPath))
        {
            return QFile::copy(m_dbPath, backupPath);
        }
        return false;
    }

    // 压缩数据库 (释放未使用空间)
    bool vacuum()
    {
        return execute("VACUUM;");
    }

  private:
    QSqlDatabase m_db;
    QString m_dbPath;
};

#endif // DBMANAGER_H