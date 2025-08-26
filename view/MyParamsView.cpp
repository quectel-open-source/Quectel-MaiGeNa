#include "MyParamsView.h"
#include "commonZack.h"
#include <qboxlayout.h>
#include <qlabel.h>
#include <qspinbox.h>
#include <qwidget.h>

MyParamsView::MyParamsView(QWidget *parent, QString qss) : QtMyGroupBox(parent)
{
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    this->Init_UI(qss);
}

MyParamsView::~MyParamsView()
{
}

void MyParamsView::Init_UI(QString const &qss)
{
    this->setContentsMargins(8, 8, 8, 8);
    v_titleBar = new MyTitleBar(this, {false, false, true}, qss, "检测参数设置");
    QFont font("Microsoft YaHei", 13);

    //    QLabel* label = newLabel(this, "当前模版");
    //    label->setFont(font);
    //    v_comboBox = newComboBox(this, {}, "请选择模板", -1, 30, 0);
    //    v_comboBox->setFont(font);
    //    MyWidget* w = new MyWidget(this);
    //    newHBoxLayout(w, {label, v_comboBox}, 2, {5,5,5,5});

    QWidget *mainWidget = new QWidget(this);
    mainWidget->setObjectName("border0");
    mainWidget->setMinimumSize(200, 300);
    v_mainLayout = new QHBoxLayout(mainWidget);
    v_mainLayout->setContentsMargins(10, 10, 10, 10);
    v_mainLayout->setSpacing(0);

    QToolButton *button_saveOne = newToolButton(this, "单工位保存一张", "单工位, 保存一张模版", -1, 36, "", false,
                                                false, Qt::ToolButtonTextBesideIcon);
    button_saveOne->setFont(font);
    button_saveOne->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(button_saveOne, &QToolButton::clicked, this, &MyParamsView::toolButton_saveOne_clicked);
    QToolButton *button_saveBatch = newToolButton(this, "所有工位保存一张", "所有工位, 保存一张模版", -1, 36, "", false,
                                                  false, Qt::ToolButtonTextBesideIcon);
    button_saveBatch->setFont(font);
    button_saveBatch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(button_saveBatch, &QToolButton::clicked, this, &MyParamsView::toolButton_saveBatch_clicked);
    MyWidget *w2 = new MyWidget(this);
    w2->setObjectName("border0");
    newVBoxLayout(w2, {button_saveOne, button_saveBatch}, 5, {5, 10, 5, 10});

    newVBoxLayout(this, {v_titleBar, mainWidget, w2}, 0, {8, 8, 8, 8});
    loadQSS_all(this, qss);
}

void MyParamsView::addListWidget(int camid, const QList<double> &scores, QString qss)
{
    //    if (!m_templateNames.contains(name)){
    //        v_comboBox->addItem(name);
    //        v_comboBox->setCurrentIndex(1);
    //    }
    int counts = v_listWidgets.count();
    QFont font("Microsoft YaHei", 12);
    QList<QWidget *> widgets;
    QList<QDoubleSpinBox *> boxes;
    for (int m = 0; m < scores.count(); ++m)
    {
        QLabel *label = newLabel(this, "位置" + QString::number(m + 1) + " 得分:");
        label->setFont(font);
        label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        QDoubleSpinBox *spinBox2 = new QDoubleSpinBox(this);
        spinBox2->setRange(0, 1);
        spinBox2->setSingleStep(0.01);
        spinBox2->setValue(scores[m]);
        spinBox2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        spinBox2->setFont(font);
        connect(spinBox2, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this,
                [=](double value) { emit scoreChanged(counts, m, value); });
        spinBox2->setStyleSheet("padding-left: 10px;");
        boxes.append(spinBox2);

        MyWidget *ww = new MyWidget(this);
        ww->setObjectName("border0");
        QHBoxLayout *lay = new QHBoxLayout(ww);
        lay->addWidget(label);
        lay->addWidget(spinBox2);
        lay->setSpacing(4);
        lay->setContentsMargins(6, 2, 6, 2);
        loadQSS_all(ww, qss);
        widgets.append(ww);
    }
    v_labels_scoreBoxes.append(boxes);

    QListWidget *w = new QListWidget(this);
    w->setMinimumSize(200, 400);
    w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    w->setViewMode(QListView::ListMode);
    w->setSpacing(2);
    w->setIconSize(QSize(220, 40));
    w->setResizeMode(QListView::Adjust);
    w->setContentsMargins(0, 0, 0, 0);
    connect(w, &QListWidget::itemClicked, this, [=](QListWidgetItem *item) { listWidget_itemClicked(counts, item); });

    for (int m = 0; m < widgets.count(); ++m)
    {
        QListWidgetItem *item = new QListWidgetItem(w);
        item->setSizeHint(QSize(220, 36)); // 设置 item 大小
        w->setItemWidget(item, widgets[m]);
    }
    v_listWidgets.append(w);

    v_mainLayout->addWidget(w);
    for (int i = 0; i < v_listWidgets.count(); ++i)
    {
        if (i == 0)
        {
            v_listWidgets[i]->show();
        }
        else
        {
            v_listWidgets[i]->hide();
        }
    }
}

void MyParamsView::addMeanThreshWidget(int camid, double meanThresh)
{
    QFont font("Microsoft YaHei", 12);

    QWidget *w = new QWidget(this);
    QLabel *label = new QLabel("均值阈值:");
    label->setFont(font);
    QDoubleSpinBox *spinBox = new QDoubleSpinBox(w);
    spinBox->setFont(font);
    spinBox->setRange(0, 9999);
    connect(spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [=](double value) { emit meanThreshChanged(camid, value); });
    QHBoxLayout *wlayout = new QHBoxLayout(w);
    wlayout->addWidget(label);
    wlayout->addWidget(spinBox, 1);
    w->setLayout(wlayout);

    static_cast<QVBoxLayout *>(layout())->insertWidget(layout()->count() - 1, w);
}

void MyParamsView::listWidget_itemClicked(int camid, QListWidgetItem *item)
{
    int row = v_listWidgets[camid]->row(item);
    emit saveModeChanged(2, camid, row);
}

void MyParamsView::toolButton_saveOne_clicked()
{
    int camid = 0;
    int index = v_listWidgets[camid]->row(v_listWidgets[camid]->currentItem());
    emit saveModeChanged(0, camid, index);
}
void MyParamsView::toolButton_saveBatch_clicked()
{
    int camid = 0;
    emit saveModeChanged(1, camid, -1);
}
