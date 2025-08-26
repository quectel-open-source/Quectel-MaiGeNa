#pragma once
#include <QAbstractGraphicsShapeItem>
#include <QCursor>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QList>
#include <QObject>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <qgraphicsitem.h>
#include <qpoint.h>

// 自定义功能图元 - 点
class BPointItem : public QObject, public QAbstractGraphicsShapeItem
{
    Q_OBJECT

  public:
    enum PointType
    {
        Center = 0, // 中心点
        Edge,       // 边缘点（可拖动改变图形的形状、大小）
        Special     // 特殊功能点
    };

    BPointItem(QAbstractGraphicsShapeItem *parent, QPointF p, PointType type);

    QPointF getPoint()
    {
        return m_point;
    }
    void setPoint(QPointF p)
    {
        m_point = p;
    }

  protected:
    virtual QRectF boundingRect() const override;

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;

  public:
    QPointF m_point;
    PointType m_type;
};

//------------------------------------------------------------------------------

// 存放点的容器
class BPointItemList : public QList<BPointItem *>
{
  public:
    void setRandColor();
    void setColor(const QColor color);
    void setVisible(bool visible);
};

#ifndef PI
#define PI 3.1415926
#endif

// 自定义图元 - 基础类
class BGraphicsItem : public QObject, public QAbstractGraphicsShapeItem
{
    Q_OBJECT

  public:
    enum ItemType
    {
        Circle = 0,          // 圆
        Ellipse,             // 椭圆
        Concentric_Circle,   // 同心圆
        Pie,                 // 饼
        Chord,               // 和弦
        Rectangle,           // 矩形
        Square,              // 正方形
        Polygon,             // 多边形
        Round_End_Rectangle, // 圆端矩形
        Rounded_Rectangle    // 圆角矩形
    };

    QPointF getCenter()
    {
        return m_center;
    }
    void setCenter(QPointF p)
    {
        m_center = p;
    }

    QPointF getEdge()
    {
        return m_edge;
    }
    void setEdge(QPointF p)
    {
        m_edge = p;
    }

    ItemType getType()
    {
        return m_type;
    }

  protected:
    BGraphicsItem(QPointF center, QPointF edge, ItemType type);

    virtual void focusInEvent(QFocusEvent *event) override;
    virtual void focusOutEvent(QFocusEvent *event) override;

  public:
    QPointF m_center;
    QPointF m_edge;
    ItemType m_type;
    BPointItemList m_pointList;

    QPen m_pen_isSelected;
    QPen m_pen_noSelected;
};

//------------------------------------------------------------------------------

// 椭圆
class BEllipse : public BGraphicsItem
{
    Q_OBJECT

  public:
    BEllipse(qreal x, qreal y, qreal width, qreal height);

  protected:
    virtual QRectF boundingRect() const override;

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
};

//------------------------------------------------------------------------------

// 圆
class BCircle : public BEllipse
{
  public:
    BCircle(qreal x, qreal y, qreal radius);

    void updateRadius();

  protected:
    virtual QRectF boundingRect() const override;

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;

  public:
    qreal m_radius;
};

//------------------------------------------------------------------------------

// 同心圆
class BConcentricCircle : public BCircle
{
  public:
    BConcentricCircle(qreal x, qreal y, qreal radius1, qreal radius2);

    void updateOtherRadius();
    void setAnotherEdge(QPointF p);

  protected:
    virtual QRectF boundingRect() const override;

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;

  public:
    QPointF m_another_edge;
    qreal m_another_radius;
};

//------------------------------------------------------------------------------

// 饼
class BPie : public BCircle
{
  public:
    BPie(qreal x, qreal y, qreal radius, qreal angle);

    void updateAngle();

  protected:
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

  public:
    qreal m_angle;
};

//------------------------------------------------------------------------------

// 和弦
class BChord : public BPie
{
  public:
    BChord(qreal x, qreal y, qreal radius, qreal angle);

    void updateEndAngle();

  protected:
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

  public:
    qreal m_end_angle;
};

//------------------------------------------------------------------------------

// 矩形
class BRectangle : public BGraphicsItem
{
  public:
    BRectangle(qreal x, qreal y, qreal width, qreal height);

  protected:
    virtual QRectF boundingRect() const override;

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
};

//------------------------------------------------------------------------------

// 正方形
class BSquare : public BRectangle
{
  public:
    BSquare(qreal x, qreal y, qreal width);

  protected:
    virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
};

//------------------------------------------------------------------------------

// 多边形
class BPolygon : public BGraphicsItem
{
    Q_OBJECT

  public:
    BPolygon();

    QPointF getCentroid(QList<QPointF> list);
    void getMaxLength();
    void updatePolygon(QPointF origin, QPointF end);

  public slots:
    void pushPoint(QPointF p, QList<QPointF> list, bool isCenter);

  protected:
    virtual QRectF boundingRect() const override;

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

  public:
    qreal m_radius;
    bool is_create_finished;
};

//------------------------------------------------------------------------------

// 圆端矩形
class BRound_End_Rectangle : public BRectangle
{
  public:
    BRound_End_Rectangle(qreal x, qreal y, qreal width, qreal height);

  protected:
    virtual QRectF boundingRect() const override;

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
};

//------------------------------------------------------------------------------

// 圆角矩形
class BRounded_Rectangle : public BRectangle
{
  public:
    BRounded_Rectangle(qreal x, qreal y, qreal width, qreal height);

    void updateRadius();
    void updateAnotherEdge(QPointF p);
    qreal getRadius();
    QPointF getAnotherEdge();
    void setAnotherEdge(QPointF p);

  protected:
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;

  public:
    QPointF m_another_edge;
    qreal m_radius;
};

//------------------------------------------------------------------------------

// 定义一个装饰器类，用于给一个对象添加一些额外的功能
class LabeledGraphicsItem : public QObject, public QGraphicsItemGroup
{
    Q_OBJECT

  private:
    QString m_name;
    QGraphicsSimpleTextItem *m_label;
    QGraphicsItem *m_item;

  public:
    LabeledGraphicsItem(const QString &text, QGraphicsItem *item, QObject *parent = nullptr)
        : QObject(parent), m_name(text), m_item(item)

    {
        item->setParentItem(this);
        // 设置标签文本
        m_label = new QGraphicsSimpleTextItem(text, this);
        m_label->setBrush(Qt::white);
        m_label->setFont(QFont("Arial", 10));

        // 将目标项目添加到组中
        if (m_item)
        {
            addToGroup(m_item);
        }

        // 更新标签位置
        updateLabelPosition();

        // 设置标志以接收位置变化事件
        setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    }

    void setText(const QString &text)
    {
        if (m_name != text)
        {
            m_name = text;
            m_label->setText(text);
            emit labelChanged(text);
        }
    }

  protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override
    {
        if (change == ItemPositionHasChanged || change == ItemTransformHasChanged)
        {
            updateLabelPosition();
            emit positionChanged(pos());
        }
        return QGraphicsItemGroup::itemChange(change, value);
    }

  private:
    void updateLabelPosition()
    {
        if (!m_item || !m_label)
            return;

        // 计算标签位置（在项目上方）
        QRectF itemRect = m_item->boundingRect();
        QPointF labelPos(0, -m_label->boundingRect().height() - 5);
        m_label->setPos(labelPos);
    }

  signals:
    void positionChanged(const QPointF &pos);
    void labelChanged(const QString &value);
};