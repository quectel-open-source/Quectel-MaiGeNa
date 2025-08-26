#pragma once

#include "appbar.h"
#include "basewidget.h"
#include <qcoreapplication.h>
#include <qlayout.h>
#include <qwidget.h>
class BaseWindow : public QWidget
{
  public:
    explicit BaseWindow(QWidget *parent = nullptr) : QWidget(parent), m_centralWidget(nullptr)
    {
        setWindowFlag(Qt::FramelessWindowHint);
        setWindowFlag(Qt::Window);
        setWindowFlag(Qt::WindowMinMaxButtonsHint);
        qDebug() << "BaseWindow::geometry() = " << geometry() << endl;

        // setAutoFillBackground(true);
        // setAttribute(Qt::WA_TranslucentBackground);
        setStyleSheet("QFrame{border: 0px}"
                      "QListWidget{border: 0px}"
                      "QTableWidget{border: 0px}"
                      "QGroupBox{border: 0px}"
                      "background-color: #0E1827 ;"
                      "border: 0px solid #0E1827;"
                      "color: #FFFFFF;"
                    );

        // 主布局
        m_mainLayout = new QVBoxLayout(this);
        m_mainLayout->setContentsMargins(9, 9, 9, 9);
        m_mainLayout->setSpacing(6);

        m_titleBar = new TitleBar(qAppName(), this);
        m_mainLayout->addWidget(m_titleBar); // 设置伸缩因子为1

        // 内容区域
        m_centralWidget = new QWidget(this);
        m_mainLayout->addWidget(m_centralWidget, 1); // 不拉伸

        setLayout(m_mainLayout);
        resize(m_titleBar->size() + m_centralWidget->size());
    }
    virtual ~BaseWindow() = default;

    TitleBar *titleBar() const
    {
        return m_titleBar;
    }
    QWidget *contentWidget()
    {
        return m_centralWidget;
    }

  protected:
    void paintEvent(QPaintEvent *event) override
    {
        QWidget::paintEvent(event);
        paintWidget(this);
    }

  private:
    QWidget *m_centralWidget;
    TitleBar *m_titleBar;
    QVBoxLayout *m_mainLayout;
};