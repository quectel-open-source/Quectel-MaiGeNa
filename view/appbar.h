// TitleBar.h
#ifndef TITLEBAR_H
#define TITLEBAR_H

#include "globals.h"
#include "opencv2/highgui.hpp"
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPixmap>
#include <QSize>
#include <QToolButton>
#include <QWidget>
#include <qaction.h>
#include <qapplication.h>
#include <qcoreapplication.h>
#include <qevent.h>
#include <qglobal.h>
#include <qicon.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qpoint.h>
#include <qscreen.h>
#include <qsize.h>
#include <qtoolbutton.h>
#include <qwidget.h>
#include <qwindowdefs.h>

class TitleBar : public QWidget
{
    Q_OBJECT
  public:
    inline static QSize ICON_SIZE = QSize(20, 20);

  signals:
    void requestMinimize();
    void requestMaximize();
    void requestClose();
    void requestMove(const QPoint &pos);

  public:
    explicit TitleBar(QWidget *parent = nullptr) : TitleBar(QIcon(), QString(), parent)
    {
    }
    explicit TitleBar(const QString &title, QWidget *parent = nullptr) : TitleBar(QIcon(), title, parent)
    {
    }
    explicit TitleBar(const QIcon &logo, const QString &title, QWidget *parent = nullptr) : QWidget(parent)
    {
        setObjectName("TitleBar");
        // 初始化UI
        setAttribute(Qt::WA_TranslucentBackground);
        // 设置标题栏背景为透明, 设置close, maximize, minimize 等按钮为透明
        setStyleSheet("QWidget{background-color:red; color: #FFFFFF;}"
                      "QToolButton#min{border:none; background:transparent;}"
                      "QToolButton#max{border:none; background:transparent;}"
                      "QToolButton#close{border:none; background:transparent;}"
                      "QLabel#logo{border:none; background:transparent;color:white;}"
                      "QLabel#title{border:none; background:transparent;color:white;}");
        // 主布局
        QHBoxLayout *mainLayout = new QHBoxLayout(this);
        mainLayout->setContentsMargins(9, 3, 9, 3);
        mainLayout->setSpacing(0);

        // 图标
        logoLabel_ = new QLabel(this);
        logoLabel_->setObjectName("logo");
        logoLabel_->setMinimumSize(40, 40);
        logoLabel_->setMaximumSize(40, 40);
        logoLabel_->setScaledContents(true);
        logoLabel_->setPixmap(QPixmap(":/icon/Resources/main-1.ico"));
        logoLabel_->setAttribute(Qt::WA_TranslucentBackground);
        mainLayout->addWidget(logoLabel_);

        // 标题
        titleLabel_ = new QLabel("Title", this);
        titleLabel_->setObjectName("title");
        titleLabel_->setAttribute(Qt::WA_TranslucentBackground);
        titleLabel_->setFont(Globals::Fonts::title1());
        mainLayout->addWidget(titleLabel_);

        // 弹簧 使用QWidget来替换，以后方便在这个widget中增加一些导航按钮
        QWidget *actionWidget = new QWidget(this);
        actionWidget->setObjectName("actionWidget");
        actionWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        actionWidget->setAttribute(Qt::WA_TranslucentBackground);
        // 设置布局
        actionLayout_ = new QHBoxLayout(actionWidget);
        actionLayout_->setContentsMargins(0, 0, 0, 0);

        mainLayout->addWidget(actionWidget, 1);

        // 创建一个layout，subactionlayout, 存储下面的按钮
        subactionLayout_ = new QHBoxLayout;
        subactionLayout_->setSpacing(0);
        subactionLayout_->setContentsMargins(0, 0, 0, 0);
        subactionLayout_->setObjectName("subactionLayout");

        // 最小化按钮
        minBtn_ = new QToolButton(this);
        minBtn_->setObjectName("min");
        minBtn_->setIcon(QIcon(":/Resources/Resources/winmix.png"));
        minBtn_->setIconSize(ICON_SIZE);
        minBtn_->setToolTip(tr("Minimize"));
        minBtn_->setAutoRaise(true);
        subactionLayout_->addWidget(minBtn_);

        // 最大化按钮
        maxBtn_ = new QToolButton(this);
        maxBtn_->setObjectName("max");
        maxBtn_->setIcon(QIcon(":/Resources/Resources/win-max.png"));
        maxBtn_->setIconSize(ICON_SIZE);
        maxBtn_->setToolTip(tr("Maximize"));
        maxBtn_->setAutoRaise(true);
        subactionLayout_->addWidget(maxBtn_);

        // 关闭按钮
        closeBtn_ = new QToolButton(this);
        closeBtn_->setObjectName("close");
        closeBtn_->setIcon(QIcon(":/Resources/Resources/close.png"));
        closeBtn_->setIconSize(ICON_SIZE);
        closeBtn_->setToolTip(tr("Close"));
        closeBtn_->setAutoRaise(true);
        subactionLayout_->addWidget(closeBtn_);

        mainLayout->addLayout(subactionLayout_);

        setLayout(mainLayout);
        setIcon(logo);
        setTitle(title);
        // ----- end setup ui

        setMouseTracking(true); // 启用鼠标跟踪

        // 连接按钮信号
        connect(minBtn_, &QToolButton::clicked, this, &TitleBar::requestMinimize);
        connect(maxBtn_, &QToolButton::clicked, this, &TitleBar::requestMaximize);
        connect(closeBtn_, &QToolButton::clicked, this, &TitleBar::requestClose);

        connect(this, &TitleBar::requestMinimize, window(), &QWidget::showMinimized);
        connect(this, &TitleBar::requestMaximize, [this]() {
            static QRect normalGeometry = window()->geometry();
            auto screen = qApp->screenAt(normalGeometry.center());
            if (window()->isMaximized())
            {
                window()->setGeometry(normalGeometry);
                window()->showNormal();
            }
            else
            {
                window()->setGeometry(screen->availableGeometry());
                window()->showMaximized();
            }
        });
        connect(this, &TitleBar::requestClose, window(), &QWidget::close);
        connect(this, &TitleBar::requestMove, window(), QOverload<const QPoint &>::of(&QWidget::move));
    }

    ~TitleBar()
    {
        qDebug() << "TitleBar::~TitleBar()";
    }

    // 设置图标
    void setIcon(const QPixmap &pixmap)
    {
        logoLabel_->setVisible(pixmap.isNull());
        if (!pixmap.isNull())
        {
            logoLabel_->setPixmap(pixmap.scaled(logoLabel_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }

    void setIcon(const QIcon &icon)
    {
        setIcon(icon.pixmap(logoLabel_->size()));
    }

    // 设置标题
    void setTitle(const QString &title)
    {
        titleLabel_->setText(title);
    }

    // 获取标题
    QString title() const
    {
        return titleLabel_->text();
    }

    void addAction(QAction *action)
    {
        action->setParent(this);
        action->setFont(font());
        action->installEventFilter(this);
        QToolButton *button = new QToolButton(this);
        button->setDefaultAction(action);
        button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        actionLayout_->addWidget(button);
    }
    void removeAction(QAction *action)
    {
        actionLayout_->removeWidget(action->parentWidget());
    }

    void addSubAction(QAction *action)
    {
        action->setParent(this);
        action->installEventFilter(this);
        QToolButton *button = new QToolButton(this);
        button->setDefaultAction(action);
        button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        subactionLayout_->addWidget(button);
    }
    void removeSubAction(QAction *action)
    {
        subactionLayout_->removeWidget(action->parentWidget());
    }

  protected:
    // 鼠标事件实现窗口拖动
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton)
        {
            dragPosition_ = event->globalPos() - this->window()->pos();
        }
        QWidget::mouseMoveEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (event->buttons() & Qt::LeftButton && !dragPosition_.isNull())
        {
            QPoint newPos = event->globalPos() - dragPosition_;
            requestMove(newPos);
        }
        QWidget::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        dragPosition_ = QPoint();
        QWidget::mouseReleaseEvent(event);
    }

  private:
    QLabel *logoLabel_;
    QLabel *titleLabel_;
    QHBoxLayout *subactionLayout_;
    QHBoxLayout *actionLayout_;
    QToolButton *minBtn_;
    QToolButton *maxBtn_;
    QToolButton *closeBtn_;
    QPoint dragPosition_;
};

#endif // TITLEBAR_H