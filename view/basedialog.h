#pragma once

#include "appbar.h"
#include "basewidget.h"
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>
#include <qapplication.h>
#include <qdebug.h>
#include <qdialog.h>
#include <qevent.h>
#include <qnamespace.h>
#include <qpainter.h>
#include <qstyle.h>
#include <qtimer.h>
#include <qwidget.h>

class BaseDialog : public QDialog
{
    Q_OBJECT
  public:
    explicit BaseDialog(const QString &title, QWidget *content, QWidget *parent = nullptr) : QDialog(parent)
    {
        // 窗口样式
        setWindowFlag(Qt::FramelessWindowHint);
        // setAttribute(Qt::WA_TranslucentBackground);

        // 主布局
        m_mainLayout = new QVBoxLayout(this);
        m_mainLayout->setContentsMargins(3, 3, 3, 3);
        m_mainLayout->setSpacing(0);

        m_titleBar = new TitleBar(title, this);
        m_mainLayout->addWidget(m_titleBar); // 设置伸缩因子为1

        // 内容区域
        m_content = content;
        content->setParent(this);                                               // 确保父子关系正确
        content->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred); // 保持自身大小
        m_mainLayout->addWidget(content, 1);                                    // 不拉伸

        setLayout(m_mainLayout);
        resize(m_titleBar->size() + m_content->size());
    }

    ~BaseDialog()
    {
        qDebug() << "~BaseDialog()";
    }

    // 设置窗口标题
    void setWindowTitle(const QString &title)
    {
        if (m_titleLabel)
        {
            m_titleLabel->setText(title);
        }
        QWidget::setWindowTitle(title);
    }

    // 设置窗口图标
    void setWindowIcon(const QIcon &icon)
    {
        if (m_iconLabel)
        {
            m_iconLabel->setPixmap(icon.pixmap(24, 24));
        }
        QWidget::setWindowIcon(icon);
    }

  protected:
    void paintEvent(QPaintEvent *event) override
    {
        QDialog::paintEvent(event);
        paintWidget(this);
    }

  private:
    QVBoxLayout *m_mainLayout;
    TitleBar *m_titleBar;
    QLabel *m_iconLabel = nullptr;
    QLabel *m_titleLabel = nullptr;
    QWidget *m_content;
    QPoint m_dragPosition;
};