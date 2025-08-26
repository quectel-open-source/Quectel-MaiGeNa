#pragma once
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <qframe.h>
#include <QProgressDialog>

class LoadingWidget : public QProgressDialog
{
    Q_OBJECT
  public:
    explicit LoadingWidget(QWidget *parent = nullptr) : QProgressDialog(parent)
    {
        // 设置窗口标志为无边框和始终在最前
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        setAttribute(Qt::WA_TranslucentBackground);

        // 创建布局
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setAlignment(Qt::AlignCenter);

        // 创建标签
        label = new QLabel(this);
        label->setAlignment(Qt::AlignCenter);
        layout->addWidget(label, 0);

        // 创建进度条
        progressBar = new QProgressBar(this);
        progressBar->setRange(0, 0); // 设置为不确定模式
        progressBar->setTextVisible(false);
        progressBar->setFixedWidth(200);
        layout->addWidget(progressBar, 1);

        // 设置样式
        setStyleSheet("QFrame { background-color: rgba(255, 255, 255, 220); "
                      "border-radius: 10px; padding: 20px; }"
                      "QLabel { font-size: 14px; margin-bottom: 10px; }");

        // 初始化定时器（用于模拟进度）
        timer = new QTimer(this);
        progressValue = 0;
        connect(timer, &QTimer::timeout, [this]() {
            progressValue = (progressValue + 5) % 100;
            progressBar->setValue(progressValue);
        });

        // 默认隐藏
        hide();
    }

  public slots:
    void setLoading(const QString &title, bool loading)

    {
       assert(label);
        label->setText(title);

        if (loading)
        {
            // 居中显示
            if (parentWidget())
            {
                move(parentWidget()->geometry().center() - rect().center());
            }

            // 开始动画
            setRange(0, 0); // 不确定模式
            show();
        }
        else
        {
            // 停止动画
            timer->stop();
            hide();
        }
    }

  private:
    QProgressBar *progressBar;
    QLabel *label;
    QTimer *timer;
    int progressValue;
};
