#pragma once

#include <QMap>
#include <QString>
#include <qapplication.h>
#include <qchar.h>
#include <qdebug.h>
#include <qfileinfo.h>
#include <qglobal.h>
#include <qobject.h>

class AppContext
{
  private:
    // 提供注册服务，获取服务 和其他全局的功能
    inline static QMap<QString, QObject *> m_services;

  public:
    AppContext() = delete;

    template <typename T> static T *getService()
    {
        if (m_services.contains(typeid(T).name()))
        {
            qDebug() << "getService:" << typeid(T).name() << "success";
            return static_cast<T *>(m_services.value(typeid(T).name()));
        }
        qWarning() << "getService:" << typeid(T).name() << "failed";
        return nullptr;
    }

    template <typename T> static void registerService(QObject *service)
    {
        qInfo() << "registerService:" << typeid(T).name();
        m_services.insert(typeid(T).name(), service);
    }

    static void loadTheme(const QString &theme)
    {
        QFile file(":/qss/" + theme + ".qss");
        if (file.exists())
        {
            if (file.open(QFile::ReadOnly | QFile::Text))
            {
                QTextStream ts(&file);
                qApp->setStyleSheet(ts.readAll());
                qDebug() << "load style";
            }
        }
    }
};
