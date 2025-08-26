#pragma once

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qdir.h>
#include <qfont.h>
#include <qlist.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QString>

class Globals
{
    /*
    YourApp/
    ├── bin/                  # 可执行文件
    ├── config/               # 配置文件
    │   ├── app.ini           # 主配置文件
    │   └── user.ini          # 用户特定配置
    ├── data/                 # 应用数据
    │   ├── cache/            # 缓存数据
    │   ├── database/         # 数据库文件
    │   └── temp/             # 临时文件
    ├── logs/                 # 日志文件
    │   ├── app.log           # 当前日志
    │   └── archive/          # 归档日志
    */

  private:
    Globals()
    {
    }

  public:
    ~Globals() = default;
    Globals(const Globals &) = delete;
    Globals &operator=(const Globals &) = delete;

    // 获取当前时间字符串（多种格式）
    static inline QString currentTimeString(const QString &format = "yyyy-MM-dd hh:mm:ss")
    {
        return QDateTime::currentDateTime().toString(format);
    }

    static inline QString appDir()
    {
        return QCoreApplication::applicationDirPath();
    }
    static inline QString appName()
    {
        return QCoreApplication::applicationName();
    }
    static inline QString rootDir()
    {
#ifdef QT_DEBUG
        return QCoreApplication::applicationDirPath();
#else
        return QDir::homePath();
        QString path = QDir::homePath() + "/" + QCoreApplication::applicationName();
        QDir().mkpath(path);
        return path;
#endif
    }

    template <typename... Args> static inline QString getOrCreateFolder(Args &&...args)
    {
        QStringList pathComponents = {QString(std::forward<Args>(args))...};
        QString fullPath = QDir::cleanPath(pathComponents.join(QDir::separator()));
        QDir dir(fullPath);

        if (!dir.exists())
        {
            if (!dir.mkpath(fullPath))
            {
                qWarning() << "Failed to create directory:" << fullPath;
            }
        }

        return fullPath;
    }

    // 配置相关路径
    static inline QString configDir()
    {
        return getOrCreateFolder(rootDir(), "config");
    }
    static inline QString configFilePath(const QString &fileName = "app.ini")
    {
        return configDir() + "/" + fileName;
    }
    static inline QString userConfigPath()
    {
        return configDir() + "/user.ini";
    }

    // 数据相关路径
    static inline QString dataDir()
    {
        return getOrCreateFolder(rootDir() + "/data");
    }

    // 按日期组织的子目录
    static inline QString dailyDataDir()
    {
        return getOrCreateFolder(dataDir(), QDate::currentDate().toString("yyyyMMdd"));
    }
    static inline QString hourlyDataDir()
    {
        return getOrCreateFolder(dailyDataDir(), QTime::currentTime().toString("hh"));
    }

    static inline QString resultsDir()
    {
        return getOrCreateFolder(dataDir(), "results");
    }

    static inline QString imageDataDir()
    {
        return getOrCreateFolder(dataDir(), "images");
    }

    static inline QString genImageFilePath(const QString &prefix = "image", const QString &extension = "png")
    {
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmsszzz");
        return imageDataDir() + "/" + prefix + "_" + timestamp + "." + extension;
    }

    static inline QString cacheDir()
    {
        return getOrCreateFolder(rootDir(), "cache");
    }
    static inline QString tempDir()
    {
        return getOrCreateFolder(rootDir(), "temp");
    }

    // 日志相关路径
    static inline QString logDir()
    {
        return getOrCreateFolder(rootDir(), "logs");
    }
    static inline QString currentLogFile()
    {
        return logDir() + "/application.log";
    }
    static inline QString dailyLogFile()
    {
        return logDir() + "/" + QDate::currentDate().toString("yyyyMMdd") + ".log";
    }

    // 清理临时文件
    static inline void cleanupTempFiles()
    {
        QDir tmpDir(tempDir());
        tmpDir.setNameFilters(QStringList() << "*.*");
        tmpDir.setFilter(QDir::Files);

        int removedCount = 0;
        qint64 freedSpace = 0;

        foreach (QString dirFile, tmpDir.entryList())
        {
            QFileInfo fi(tmpDir.filePath(dirFile));
            freedSpace += fi.size();
            if (tmpDir.remove(dirFile))
            {
                removedCount++;
            }
        }
    }

    class Fonts
    {
      public:
        // 根据google 的 fonts 规范，生成常用的字体
        static QFont title1()
        {
            return QFont("Microsoft YaHei", 16, QFont::Bold);
        }

        static QFont title2()
        {
            return QFont("Microsoft YaHei", 14, QFont::Bold);
        }
    };
};