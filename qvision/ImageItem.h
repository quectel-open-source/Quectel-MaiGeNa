#pragma once
#pragma execution_character_set("utf-8")
#include <QGraphicsPixmapItem>
#include <QMouseEvent>

//通过鼠标点选获取当前灰度值
class ImageItem :public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT

public:
    explicit ImageItem(QWidget *parent = nullptr);

protected:
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);

public:
    int w;
    int h;

signals:
    void RGBValue(QString InfoVal);

    void emit_pt(int, int);
};
