#include "qdrawer.h"

BPointItem::BPointItem(QAbstractGraphicsShapeItem *parent, QPointF p, PointType type)
    : QAbstractGraphicsShapeItem(parent), m_point(p), m_type(type)
{
    this->setPos(m_point);
    this->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsFocusable);

    switch (type)
    {
    case Center:
        this->setCursor(Qt::OpenHandCursor);
        break;
    case Edge:
        this->setCursor(Qt::PointingHandCursor);
        break;
    case Special:
        this->setCursor(Qt::PointingHandCursor);
        break;
    default:
        break;
    }
}

QRectF BPointItem::boundingRect() const
{
    return QRectF(-4, -4, 8, 8);
}

void BPointItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->setPen(this->pen());
    painter->setBrush(this->brush());
    this->setPos(m_point);

    switch (m_type)
    {
    case Center:
        painter->drawEllipse(-4, -4, 8, 8);
        break;
    case Edge:
        painter->drawRect(QRectF(-4, -4, 8, 8));
        break;
    case Special:
        painter->drawRect(QRectF(-4, -4, 8, 8));
        break;
    default:
        break;
    }
}

void BPointItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton)
    {
        qreal dx = event->scenePos().x() - event->lastScenePos().x();
        qreal dy = event->scenePos().y() - event->lastScenePos().y();

        BGraphicsItem *item = static_cast<BGraphicsItem *>(this->parentItem());
        BGraphicsItem::ItemType itemType = item->getType();

        switch (m_type)
        {
        case Center: {
            item->moveBy(dx, dy);
            this->scene()->update();
        }
        break;
        case Edge: {
            if (itemType != BGraphicsItem::ItemType::Rounded_Rectangle)
            {
                m_point = this->mapToParent(event->pos());
                this->setPos(m_point);
                this->scene()->update();
            }

            switch (itemType)
            {
            case BGraphicsItem::ItemType::Ellipse: {
                BEllipse *ellipse = dynamic_cast<BEllipse *>(item);
                ellipse->setEdge(m_point);
            }
            break;
            case BGraphicsItem::ItemType::Circle: {
                BCircle *circle = dynamic_cast<BCircle *>(item);
                circle->setEdge(m_point);
                circle->updateRadius();
            }
            break;
            case BGraphicsItem::ItemType::Concentric_Circle: {
                BCircle *circle = dynamic_cast<BCircle *>(item);
                circle->setEdge(m_point);
                circle->updateRadius();
            }
            break;
            case BGraphicsItem::ItemType::Pie: {
                BPie *pie = dynamic_cast<BPie *>(item);
                pie->setEdge(m_point);
                pie->updateRadius();
                pie->updateAngle();
            }
            break;
            case BGraphicsItem::ItemType::Chord: {
                BChord *chord = dynamic_cast<BChord *>(item);
                chord->setEdge(m_point);
                chord->updateRadius();
                chord->updateEndAngle();
            }
            break;
            case BGraphicsItem::ItemType::Rectangle: {
                BRectangle *rectangle = dynamic_cast<BRectangle *>(item);
                rectangle->setEdge(m_point);
            }
            break;
            case BGraphicsItem::ItemType::Square: {
                BSquare *square = dynamic_cast<BSquare *>(item);
                qreal ret = m_point.x() > m_point.y() ? m_point.x() : m_point.y();
                m_point.setX(ret);
                m_point.setY(ret);
                square->setEdge(m_point);
            }
            break;
            case BGraphicsItem::ItemType::Polygon: {
                BPolygon *polygon = dynamic_cast<BPolygon *>(item);
                polygon->updatePolygon(QPointF(event->lastScenePos().x(), event->lastScenePos().y()),
                                       QPointF(event->scenePos().x(), event->scenePos().y()));
            }
            break;
            case BGraphicsItem::ItemType::Round_End_Rectangle: {
                BRound_End_Rectangle *round_End_Rectangle = dynamic_cast<BRound_End_Rectangle *>(item);
                round_End_Rectangle->setEdge(m_point);
            }
            break;
            case BGraphicsItem::ItemType::Rounded_Rectangle: {
                BRounded_Rectangle *rounded_Rectangle = dynamic_cast<BRounded_Rectangle *>(item);

                m_point = this->mapToParent(event->pos());

                if (m_point.x() <= rounded_Rectangle->getRadius())
                {
                    m_point.setX(rounded_Rectangle->getRadius());
                }
                if (m_point.y() <= rounded_Rectangle->getRadius())
                {
                    m_point.setY(rounded_Rectangle->getRadius());
                }

                this->setPos(m_point);
                this->scene()->update();

                QPointF p = rounded_Rectangle->getEdge();
                rounded_Rectangle->setEdge(m_point);
                rounded_Rectangle->updateAnotherEdge(p);
            }
            break;
            default:
                break;
            }
        }
        break;
        case Special: {
            switch (itemType)
            {
            case BGraphicsItem::ItemType::Concentric_Circle: {
                m_point = this->mapToParent(event->pos());
                this->setPos(m_point);
                this->scene()->update();

                BConcentricCircle *conCircle = dynamic_cast<BConcentricCircle *>(item);
                conCircle->setAnotherEdge(m_point);
                conCircle->updateOtherRadius();
            }
            break;
            case BGraphicsItem::ItemType::Rounded_Rectangle: {
                BRounded_Rectangle *rounded_Rectangle = dynamic_cast<BRounded_Rectangle *>(item);

                qreal retX = 0;
                qreal retY = 0;

                if (rounded_Rectangle->getEdge().x() == rounded_Rectangle->getAnotherEdge().x() &&
                    (-1) * rounded_Rectangle->getEdge().y() != rounded_Rectangle->getAnotherEdge().y())
                {
                    retX = rounded_Rectangle->getAnotherEdge().x();
                    retY = rounded_Rectangle->getAnotherEdge().y() + dy;
                }
                else if (rounded_Rectangle->getEdge().x() != rounded_Rectangle->getAnotherEdge().x() &&
                         (-1) * rounded_Rectangle->getEdge().y() == rounded_Rectangle->getAnotherEdge().y())
                {
                    retX = rounded_Rectangle->getAnotherEdge().x() + dx;
                    retY = rounded_Rectangle->getAnotherEdge().y();
                }
                else if (rounded_Rectangle->getEdge().x() == rounded_Rectangle->getAnotherEdge().x() &&
                         (-1) * rounded_Rectangle->getEdge().y() == rounded_Rectangle->getAnotherEdge().y())
                {
                    if (abs(dx) >= abs(dy))
                    {
                        retX = rounded_Rectangle->getAnotherEdge().x() + dx;
                        retY = rounded_Rectangle->getAnotherEdge().y();
                    }
                    else
                    {
                        retX = rounded_Rectangle->getAnotherEdge().x();
                        retY = rounded_Rectangle->getAnotherEdge().y() + dy;
                    }
                }

                if (retX > rounded_Rectangle->getEdge().x())
                {
                    retX = rounded_Rectangle->getEdge().x();
                }
                else if (retX < 0)
                {
                    retX = 0;
                }

                if (retY < (-1) * rounded_Rectangle->getEdge().y())
                {
                    retY = (-1) * rounded_Rectangle->getEdge().y();
                }
                else if (retY > 0)
                {
                    retY = 0;
                }

                m_point.setX(retX);
                m_point.setY(retY);
                this->setPos(m_point);
                this->scene()->update();

                rounded_Rectangle->setAnotherEdge(m_point);
                rounded_Rectangle->updateRadius();
            }
            default:
                break;
            }
        }
        break;
        default:
            break;
        }
    }
}

//------------------------------------------------------------------------------

void BPointItemList::setRandColor()
{
    this->setColor(QColor(qrand() % 256, qrand() % 256, qrand() % 256));
}

void BPointItemList::setColor(const QColor color)
{
    for (auto &temp : *this)
    {
        temp->setBrush(QBrush(color));
    }
}

void BPointItemList::setVisible(bool visible)
{
    for (auto &temp : *this)
    {
        temp->setVisible(visible);
    }
}

//-------------------------------------------------------------
//-------------------------------------------------------------
//-------------------------------------------------------------
#include <QDebug>
#include <QMenu>
#include <QSpinBox>
#include <QVector>
#include <QWidgetAction>
#include <math.h>
using namespace std;

BGraphicsItem::BGraphicsItem(QPointF center, QPointF edge, ItemType type) : m_center(center), m_edge(edge), m_type(type)
{
    m_pen_noSelected.setColor(QColor(0, 160, 230));
    m_pen_noSelected.setWidth(2);
    m_pen_isSelected.setColor(QColor(255, 0, 255));
    m_pen_isSelected.setWidth(2);

    this->setPen(m_pen_noSelected);
    this->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsFocusable);
}

void BGraphicsItem::focusInEvent(QFocusEvent *event)
{
    Q_UNUSED(event);
    this->setPen(m_pen_isSelected);
}

void BGraphicsItem::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event);
    this->setPen(m_pen_noSelected);
}

//------------------------------------------------------------------------------

BEllipse::BEllipse(qreal x, qreal y, qreal width, qreal height)
    : BGraphicsItem(QPointF(x, y), QPointF(x + width / 2, y + height / 2), ItemType::Ellipse)
{
    BPointItem *point = new BPointItem(this, m_edge, BPointItem::Edge);
    point->setParentItem(this);
    m_pointList.append(point);
    m_pointList.append(new BPointItem(this, m_center, BPointItem::Center));
    m_pointList.setRandColor();
}

QRectF BEllipse::boundingRect() const
{
    return QRectF(m_center.x() - abs(m_edge.x()), m_center.y() - abs(m_edge.y()), abs(m_edge.x()) * 2,
                  abs(m_edge.y()) * 2);
}

void BEllipse::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->setPen(this->pen());
    painter->setBrush(this->brush());

    QRectF ret(m_center.x() - abs(m_edge.x()), m_center.y() - abs(m_edge.y()), abs(m_edge.x()) * 2,
               abs(m_edge.y()) * 2);
    painter->drawEllipse(ret);
}

void BEllipse::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    if (!this->isSelected())
        return;

    QMenu *menu = new QMenu();
    menu->setStyleSheet("QMenu { background-color:rgb(89,87,87); border: 5px solid rgb(235,110,36); }");

    QSpinBox *width_spinBox = new QSpinBox(menu);
    width_spinBox->setStyleSheet("QSpinBox{ width:120px; height:30px; font-size:16px; font-weight:bold; }");
    width_spinBox->setRange(0, 1000);
    width_spinBox->setPrefix("w: ");
    width_spinBox->setSuffix(" mm");
    width_spinBox->setSingleStep(1);
    width_spinBox->setValue(2 * abs(m_edge.x()));
    connect(width_spinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [=](int v) {
        if (m_edge.x() < 0)
        {
            m_edge.setX((-1) * v / 2);
        }
        else
        {
            m_edge.setX(v / 2);
        }
        m_pointList.at(0)->setPoint(m_edge);
        this->hide();
        this->update();
        this->show();
    });

    QSpinBox *height__spinBox = new QSpinBox(menu);
    height__spinBox->setStyleSheet("QSpinBox{ width:120px; height:30px; font-size:16px; font-weight:bold; }");
    height__spinBox->setRange(0, 1000);
    height__spinBox->setPrefix("h: ");
    height__spinBox->setSuffix(" mm");
    height__spinBox->setSingleStep(1);
    height__spinBox->setValue(2 * abs(m_edge.y()));
    connect(height__spinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [=](int v) {
        if (m_edge.y() < 0)
        {
            m_edge.setY((-1) * v / 2);
        }
        else
        {
            m_edge.setY(v / 2);
        }
        m_pointList.at(0)->setPoint(m_edge);
        this->hide();
        this->update();
        this->show();
    });

    QWidgetAction *width_widgetAction = new QWidgetAction(menu);
    width_widgetAction->setDefaultWidget(width_spinBox);
    menu->addAction(width_widgetAction);

    QWidgetAction *height_widgetAction = new QWidgetAction(menu);
    height_widgetAction->setDefaultWidget(height__spinBox);
    menu->addAction(height_widgetAction);

    menu->exec(QCursor::pos());
    delete menu;

    QGraphicsItem::contextMenuEvent(event);
}

//------------------------------------------------------------------------------

BCircle::BCircle(qreal x, qreal y, qreal radius) : BEllipse(x, y, radius * sqrt(2), radius * sqrt(2))
{
    m_type = ItemType::Circle;
    updateRadius();
}

void BCircle::updateRadius()
{
    m_radius = sqrt(pow(m_center.x() - m_edge.x(), 2) + pow(m_center.y() - m_edge.y(), 2));
}

QRectF BCircle::boundingRect() const
{
    return QRectF(m_center.x() - m_radius, m_center.y() - m_radius, m_radius * 2, m_radius * 2);
}

void BCircle::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->setPen(this->pen());
    painter->setBrush(this->brush());

    QRectF ret(m_center.x() - m_radius, m_center.y() - m_radius, m_radius * 2, m_radius * 2);
    painter->drawEllipse(ret);
}

void BCircle::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    if (!this->isSelected())
        return;

    QMenu *menu = new QMenu();
    menu->setStyleSheet("QMenu { background-color:rgb(89,87,87); border: 5px solid rgb(235,110,36); }");

    QSpinBox *radius_spinBox = new QSpinBox(menu);
    radius_spinBox->setStyleSheet("QSpinBox{ width:120px; height:30px; font-size:16px; font-weight:bold; }");
    radius_spinBox->setRange(0, 1000);
    radius_spinBox->setPrefix("r: ");
    radius_spinBox->setSuffix(" mm");
    radius_spinBox->setSingleStep(1);
    radius_spinBox->setValue(m_radius);
    connect(radius_spinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [=](int v) {
        m_radius = v;

        if (m_edge.x() < 0)
        {
            m_edge.setX(m_center.x() - m_radius * sqrt(2) / 2);
        }
        else
        {
            m_edge.setX(m_center.x() + m_radius * sqrt(2) / 2);
        }

        if (m_edge.y() < 0)
        {
            m_edge.setY(m_center.y() - m_radius * sqrt(2) / 2);
        }
        else
        {
            m_edge.setY(m_center.y() + m_radius * sqrt(2) / 2);
        }

        m_pointList.at(0)->setPoint(m_edge);
        this->hide();
        this->update();
        this->show();
    });

    QWidgetAction *radius_widgetAction = new QWidgetAction(menu);
    radius_widgetAction->setDefaultWidget(radius_spinBox);
    menu->addAction(radius_widgetAction);

    menu->exec(QCursor::pos());
    delete menu;

    QGraphicsItem::contextMenuEvent(event);
}

//------------------------------------------------------------------------------

BConcentricCircle::BConcentricCircle(qreal x, qreal y, qreal radius1, qreal radius2)
    : BCircle(x, y, radius1), m_another_edge(x + radius2 * sqrt(2) / 2, y + radius2 * sqrt(2) / 2)
{
    m_type = ItemType::Concentric_Circle;
    BPointItem *point = new BPointItem(this, m_another_edge, BPointItem::Special);
    point->setParentItem(this);
    m_pointList.append(point);
    m_pointList.setRandColor();

    updateOtherRadius();
}

void BConcentricCircle::updateOtherRadius()
{
    m_another_radius = sqrt(pow(m_center.x() - m_another_edge.x(), 2) + pow(m_center.y() - m_another_edge.y(), 2));
}

void BConcentricCircle::setAnotherEdge(QPointF p)
{
    m_another_edge = p;
}

QRectF BConcentricCircle::boundingRect() const
{
    qreal temp = m_radius > m_another_radius ? m_radius : m_another_radius;
    return QRectF(m_center.x() - temp, m_center.y() - temp, temp * 2, temp * 2);
}

void BConcentricCircle::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->setPen(this->pen());
    painter->setBrush(this->brush());

    QRectF ret(m_center.x() - m_another_radius, m_center.y() - m_another_radius, m_another_radius * 2,
               m_another_radius * 2);
    painter->drawEllipse(ret);

    BCircle::paint(painter, option, widget);
}

void BConcentricCircle::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    if (!this->isSelected())
        return;

    QMenu *menu = new QMenu();
    menu->setStyleSheet("QMenu { background-color:rgb(89,87,87); border: 5px solid rgb(235,110,36); }");

    QSpinBox *radius_spinBox = new QSpinBox(menu);
    radius_spinBox->setStyleSheet("QSpinBox{ width:120px; height:30px; font-size:16px; font-weight:bold; }");
    radius_spinBox->setRange(0, 1000);
    radius_spinBox->setPrefix("r1: ");
    radius_spinBox->setSuffix(" mm");
    radius_spinBox->setSingleStep(1);
    radius_spinBox->setValue(m_radius);
    connect(radius_spinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [=](int v) {
        m_radius = v;

        if (m_edge.x() < 0)
        {
            m_edge.setX(m_center.x() - m_radius * sqrt(2) / 2);
        }
        else
        {
            m_edge.setX(m_center.x() + m_radius * sqrt(2) / 2);
        }

        if (m_edge.y() < 0)
        {
            m_edge.setY(m_center.y() - m_radius * sqrt(2) / 2);
        }
        else
        {
            m_edge.setY(m_center.y() + m_radius * sqrt(2) / 2);
        }

        m_pointList.at(0)->setPoint(m_edge);
        this->hide();
        this->update();
        this->show();
    });

    QSpinBox *another_radius_spinBox = new QSpinBox(menu);
    another_radius_spinBox->setStyleSheet("QSpinBox{ width:120px; height:30px; font-size:16px; font-weight:bold; }");
    another_radius_spinBox->setRange(0, 1000);
    another_radius_spinBox->setPrefix("r2: ");
    another_radius_spinBox->setSuffix(" mm");
    another_radius_spinBox->setSingleStep(1);
    another_radius_spinBox->setValue(m_another_radius);
    connect(another_radius_spinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [=](int v) {
        m_another_radius = v;

        if (m_another_edge.x() < 0)
        {
            m_another_edge.setX(m_center.x() - m_another_radius * sqrt(2) / 2);
        }
        else
        {
            m_another_edge.setX(m_center.x() + m_another_radius * sqrt(2) / 2);
        }

        if (m_another_edge.y() < 0)
        {
            m_another_edge.setY(m_center.y() - m_another_radius * sqrt(2) / 2);
        }
        else
        {
            m_another_edge.setY(m_center.y() + m_another_radius * sqrt(2) / 2);
        }

        m_pointList.at(2)->setPoint(m_another_edge);
        this->hide();
        this->update();
        this->show();
    });

    QWidgetAction *radius_widgetAction = new QWidgetAction(menu);
    radius_widgetAction->setDefaultWidget(radius_spinBox);
    menu->addAction(radius_widgetAction);

    QWidgetAction *another_radius_widgetAction = new QWidgetAction(menu);
    another_radius_widgetAction->setDefaultWidget(another_radius_spinBox);
    menu->addAction(another_radius_widgetAction);

    menu->exec(QCursor::pos());
    delete menu;

    QGraphicsItem::contextMenuEvent(event);
}

//------------------------------------------------------------------------------

BPie::BPie(qreal x, qreal y, qreal radius, qreal angle) : BCircle(x, y, radius), m_angle(angle)
{
    m_type = ItemType::Pie;
    if ((angle >= 0 && angle < 90) || (angle >= 270 && angle < 360))
    {
        m_edge.setX(m_center.x() + radius * cos(angle / 180 * PI));
        m_edge.setY(m_center.y() + radius * sin(angle / 180 * PI) * (-1));
    }
    else if ((angle >= 90 && angle < 270))
    {
        m_edge.setY(m_center.y() + radius * sin(angle / 180 * PI) * (-1));
        m_edge.setX(m_center.x() + radius * cos(angle / 180 * PI));
    }

    m_pointList.at(0)->setPoint(m_edge);
    m_radius = radius;
}

void BPie::updateAngle()
{
    qreal dx = m_edge.x() - m_center.x();
    qreal dy = m_edge.y() - m_center.y();

    if (dx >= 0 && dy < 0)
    {
        m_angle = atan2((-1) * (dy), dx) * 180 / PI;
    }
    else if (dx < 0 && dy < 0)
    {
        m_angle = atan2((-1) * dy, dx) * 180 / PI;
    }
    else if (dx < 0 && dy >= 0)
    {
        m_angle = 360 + atan2((-1) * dy, dx) * 180 / PI;
    }
    else if (dx >= 0 && dy >= 0)
    {
        m_angle = 360 - atan2(dy, dx) * 180 / PI;
    }
}

void BPie::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->setPen(this->pen());
    painter->setBrush(this->brush());

    QRectF ret(m_center.x() - m_radius, m_center.y() - m_radius, m_radius * 2, m_radius * 2);
    painter->drawPie(ret, 16 * 0, 16 * (m_angle));
}

//------------------------------------------------------------------------------

BChord::BChord(qreal x, qreal y, qreal radius, qreal angle) : BPie(x, y, radius, angle)
{
    m_type = ItemType::Chord;
    updateEndAngle();
}

void BChord::updateEndAngle()
{
    qreal dx = m_edge.x() - m_center.x();
    qreal dy = m_edge.y() - m_center.y();

    if (dx >= 0 && dy < 0)
    {
        m_angle = atan2((-1) * (dy), dx) * 180 / PI;
    }
    else if (dx < 0 && dy < 0)
    {
        m_angle = 180 - atan2((-1) * dy, dx) * 180 / PI;
    }
    else if (dx < 0 && dy >= 0)
    {
        m_angle = 360 + atan2((-1) * dy, dx) * 180 / PI;
    }
    else if (dx >= 0 && dy >= 0)
    {
        m_angle = atan2((-1) * dy, dx) * 180 / PI;
    }

    m_end_angle = 180 - m_angle * 2;
}

void BChord::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->setPen(this->pen());
    painter->setBrush(this->brush());

    QRectF ret(m_center.x() - m_radius, m_center.y() - m_radius, m_radius * 2, m_radius * 2);
    painter->drawChord(ret, 16 * (m_angle), 16 * (m_end_angle));
}

//------------------------------------------------------------------------------

BRectangle::BRectangle(qreal x, qreal y, qreal width, qreal height)
    : BGraphicsItem(QPointF(x, y), QPointF(width / 2, height / 2), ItemType::Rectangle)
{
    BPointItem *point = new BPointItem(this, m_edge, BPointItem::Edge);
    point->setPos(m_edge);
    point->setParentItem(this);
    m_pointList.append(point);
    m_pointList.append(new BPointItem(this, m_center, BPointItem::Center));
    m_pointList.setRandColor();
}

QRectF BRectangle::boundingRect() const
{
    return QRectF(m_center.x() - abs(m_edge.x()), m_center.y() - abs(m_edge.y()), abs(m_edge.x()) * 2,
                  abs(m_edge.y()) * 2);
}

void BRectangle::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->setPen(this->pen());
    painter->setBrush(this->brush());

    QRectF ret(m_center.x() - abs(m_edge.x()), m_center.y() - abs(m_edge.y()), abs(m_edge.x()) * 2,
               abs(m_edge.y()) * 2);
    painter->drawRect(ret);
}

void BRectangle::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    if (!this->isSelected())
        return;

    QMenu *menu = new QMenu();
    menu->setStyleSheet("QMenu { background-color:rgb(89,87,87); border: 5px solid rgb(235,110,36); }");

    QSpinBox *width_spinBox = new QSpinBox(menu);
    width_spinBox->setStyleSheet("QSpinBox{ width:120px; height:30px; font-size:16px; font-weight:bold; }");
    width_spinBox->setRange(0, 1000);
    width_spinBox->setPrefix("w: ");
    width_spinBox->setSuffix(" mm");
    width_spinBox->setSingleStep(1);
    width_spinBox->setValue(2 * abs(m_edge.x()));
    connect(width_spinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [=](int v) {
        if (m_edge.x() < 0)
        {
            m_edge.setX((-1) * v / 2);
        }
        else
        {
            m_edge.setX(v / 2);
        }
        m_pointList.at(0)->setPoint(m_edge);
        this->hide();
        this->update();
        this->show();
    });

    QSpinBox *height__spinBox = new QSpinBox(menu);
    height__spinBox->setStyleSheet("QSpinBox{ width:120px; height:30px; font-size:16px; font-weight:bold; }");
    height__spinBox->setRange(0, 1000);
    height__spinBox->setPrefix("h: ");
    height__spinBox->setSuffix(" mm");
    height__spinBox->setSingleStep(1);
    height__spinBox->setValue(2 * abs(m_edge.y()));
    connect(height__spinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [=](int v) {
        if (m_edge.y() < 0)
        {
            m_edge.setY((-1) * v / 2);
        }
        else
        {
            m_edge.setY(v / 2);
        }
        m_pointList.at(0)->setPoint(m_edge);
        this->hide();
        this->update();
        this->show();
    });

    QWidgetAction *width_widgetAction = new QWidgetAction(menu);
    width_widgetAction->setDefaultWidget(width_spinBox);
    menu->addAction(width_widgetAction);

    QWidgetAction *height_widgetAction = new QWidgetAction(menu);
    height_widgetAction->setDefaultWidget(height__spinBox);
    menu->addAction(height_widgetAction);

    menu->exec(QCursor::pos());
    delete menu;

    QGraphicsItem::contextMenuEvent(event);
}

//------------------------------------------------------------------------------

BSquare::BSquare(qreal x, qreal y, qreal width) : BRectangle(x, y, width, width)
{
    m_type = ItemType::Square;
}

void BSquare::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    if (!this->isSelected())
        return;

    QMenu *menu = new QMenu();
    menu->setStyleSheet("QMenu { background-color:rgb(89,87,87); border: 5px solid rgb(235,110,36); }");

    QSpinBox *distance_spinBox = new QSpinBox(menu);
    distance_spinBox->setStyleSheet("QSpinBox{ width:120px; height:30px; font-size:16px; font-weight:bold; }");
    distance_spinBox->setRange(0, 1000);
    distance_spinBox->setPrefix("d: ");
    distance_spinBox->setSuffix(" mm");
    distance_spinBox->setSingleStep(1);
    distance_spinBox->setValue(2 * abs(m_edge.x()));
    connect(distance_spinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [=](int v) {
        if (m_edge.x() < 0)
        {
            m_edge.setX((-1) * v / 2);
            m_edge.setY((-1) * v / 2);
        }
        else
        {
            m_edge.setX(v / 2);
            m_edge.setY(v / 2);
        }
        m_pointList.at(0)->setPoint(m_edge);
        this->hide();
        this->update();
        this->show();
    });

    QWidgetAction *distance_widgetAction = new QWidgetAction(menu);
    distance_widgetAction->setDefaultWidget(distance_spinBox);
    menu->addAction(distance_widgetAction);

    menu->exec(QCursor::pos());
    delete menu;

    QGraphicsItem::contextMenuEvent(event);
}

//------------------------------------------------------------------------------

BPolygon::BPolygon() : BGraphicsItem(QPointF(0, 0), QPointF(0, 0), ItemType::Polygon)
{
    is_create_finished = false;
}

void BPolygon::pushPoint(QPointF p, QList<QPointF> list, bool isCenter)
{
    if (!is_create_finished)
    {
        m_center = getCentroid(list);
        getMaxLength();

        if (isCenter)
        {
            m_pointList.append(new BPointItem(this, m_center, BPointItem::Center));
            m_pointList.setRandColor();
            is_create_finished = true;
        }
        else
        {
            BPointItem *point = new BPointItem(this, p, BPointItem::Edge);
            point->setParentItem(this);
            m_pointList.append(point);
            m_pointList.setColor(QColor(0, 255, 0));
        }

        this->update();
    }
}

QPointF BPolygon::getCentroid(QList<QPointF> list)
{
    qreal x = 0;
    qreal y = 0;
    for (auto &temp : list)
    {
        x += temp.x();
        y += temp.y();
    }
    x = x / list.size();
    y = y / list.size();
    return QPointF(x, y);
}

void BPolygon::getMaxLength()
{
    QVector<qreal> vec;
    for (auto &temp : m_pointList)
    {
        qreal dis = sqrt(pow(m_center.x() - temp->x(), 2) + pow(m_center.y() - temp->y(), 2));
        vec.append(dis);
    }

    qreal ret = 0;
    for (auto &temp : vec)
    {
        if (temp > ret)
        {
            ret = temp;
        }
    }
    m_radius = ret;
}

void BPolygon::updatePolygon(QPointF origin, QPointF end)
{
    QList<QPointF> list;

    for (auto &temp : m_pointList)
    {
        if (temp->getPoint() == origin)
        {
            temp->setPoint(end);
        }
        list.append(temp->getPoint());
    }

    m_center = getCentroid(list);
    m_pointList.at(m_pointList.size() - 1)->setPoint(m_center);

    getMaxLength();
}

QRectF BPolygon::boundingRect() const
{
    return QRectF(m_center.x() - m_radius, m_center.y() - m_radius, m_radius * 2, m_radius * 2);
}

void BPolygon::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->setPen(this->pen());
    painter->setBrush(this->brush());

    if (is_create_finished)
    {
        for (int i = 1; i < m_pointList.size() - 1; i++)
        {
            painter->drawLine(m_pointList.at(i - 1)->getPoint(), m_pointList.at(i)->getPoint());
        }

        painter->drawLine(m_pointList.at(m_pointList.size() - 2)->getPoint(), m_pointList.at(0)->getPoint());
    }
    else
    {
        for (int i = 1; i < m_pointList.size(); i++)
        {
            painter->drawLine(m_pointList.at(i - 1)->getPoint(), m_pointList.at(i)->getPoint());
        }
    }
}

//------------------------------------------------------------------------------

BRound_End_Rectangle::BRound_End_Rectangle(qreal x, qreal y, qreal width, qreal height)
    : BRectangle(x, y, width, height)
{
    m_type = ItemType::Round_End_Rectangle;
}

QRectF BRound_End_Rectangle::boundingRect() const
{
    QRectF ret = QRectF(m_center.x() - m_edge.x() - m_edge.y(), m_center.y() - m_edge.y(),
                        abs(m_edge.x()) * 2 + abs(m_edge.y()) * 2, abs(m_edge.y()) * 2);

    if (m_edge.x() >= 0 && m_edge.y() < 0)
    {
        ret.moveTo(m_center.x() - m_edge.x() + m_edge.y(), m_center.y() + m_edge.y());
    }
    else if (m_edge.x() < 0 && m_edge.y() < 0)
    {
        ret.moveTo(m_center.x() + m_edge.x() + m_edge.y(), m_center.y() + m_edge.y());
    }
    else if (m_edge.x() < 0 && m_edge.y() >= 0)
    {
        ret.moveTo(m_center.x() + m_edge.x() - m_edge.y(), m_center.y() - m_edge.y());
    }
    else if (m_edge.x() >= 0 && m_edge.y() >= 0)
    {
        ret.moveTo(m_center.x() - m_edge.x() - m_edge.y(), m_center.y() - m_edge.y());
    }

    return ret;
}

void BRound_End_Rectangle::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->setPen(this->pen());
    painter->setBrush(this->brush());

    QPointF left_top, left_bottom, right_top, right_bottom;

    if (m_edge.x() >= 0 && m_edge.y() < 0)
    {
        left_top = QPointF(m_center.x() - m_edge.x() + m_edge.y(), m_edge.y());
        left_bottom = QPointF(m_center.x() - m_edge.x() + m_edge.y(), (-1) * m_edge.y());
        right_top = QPointF(m_center.x() + m_edge.x() - m_edge.y(), m_edge.y());
        right_bottom = QPointF(m_center.x() + m_edge.x() - m_edge.y(), (-1) * m_edge.y());
    }
    else if (m_edge.x() < 0 && m_edge.y() < 0)
    {
        left_top = QPointF(m_center.x() + m_edge.x() + m_edge.y(), m_edge.y());
        left_bottom = QPointF(m_center.x() + m_edge.x() + m_edge.y(), (-1) * m_edge.y());
        right_top = QPointF(m_center.x() - m_edge.x() - m_edge.y(), m_edge.y());
        right_bottom = QPointF(m_center.x() - m_edge.x() - m_edge.y(), (-1) * m_edge.y());
    }
    else if (m_edge.x() < 0 && m_edge.y() >= 0)
    {
        left_top = QPointF(m_center.x() + m_edge.x() - m_edge.y(), (-1) * m_edge.y());
        left_bottom = QPointF(m_center.x() + m_edge.x() - m_edge.y(), m_edge.y());
        right_top = QPointF(m_center.x() - m_edge.x() + m_edge.y(), (-1) * m_edge.y());
        right_bottom = QPointF(m_center.x() - m_edge.x() + m_edge.y(), m_edge.y());
    }
    else if (m_edge.x() >= 0 && m_edge.y() >= 0)
    {
        left_top = QPointF(m_center.x() - m_edge.x() - m_edge.y(), (-1) * m_edge.y());
        left_bottom = QPointF(m_center.x() - m_edge.x() - m_edge.y(), m_edge.y());
        right_top = QPointF(m_center.x() + m_edge.x() + m_edge.y(), (-1) * m_edge.y());
        right_bottom = QPointF(m_center.x() + m_edge.x() + m_edge.y(), m_edge.y());
    }

    int radius = abs(m_edge.y());
    QPointF deltax(radius, 0);
    QPointF deltay(0, radius);

    painter->drawLine(left_top + deltax, right_top - deltax);
    painter->drawLine(left_bottom + deltax, right_bottom - deltax);
    painter->drawLine(left_top + deltay, left_bottom - deltay);
    painter->drawLine(right_top + deltay, right_bottom - deltay);

    painter->drawArc(QRectF(left_top, QSizeF(radius * 2, radius * 2)), -180 * 16, -90 * 16);
    painter->drawArc(QRectF(left_bottom - deltay * 2, QSizeF(radius * 2, radius * 2)), -90 * 16, -90 * 16);
    painter->drawArc(QRectF(right_top - deltax * 2, QSizeF(radius * 2, radius * 2)), 0 * 16, 90 * 16);
    painter->drawArc(QRectF(right_bottom - deltax * 2 - deltay * 2, QSizeF(radius * 2, radius * 2)), 0 * 16, -90 * 16);
}

//------------------------------------------------------------------------------

BRounded_Rectangle::BRounded_Rectangle(qreal x, qreal y, qreal width, qreal height) : BRectangle(x, y, width, height)
{
    m_type = ItemType::Rounded_Rectangle;
    m_another_edge.setX(m_edge.x());
    m_another_edge.setY((-1) * m_edge.y());

    BPointItem *point = new BPointItem(this, m_another_edge, BPointItem::Special);
    point->setParentItem(this);
    point->setCursor(Qt::SizeAllCursor);
    m_pointList.append(point);
    m_pointList.setRandColor();

    updateRadius();
}

void BRounded_Rectangle::updateRadius()
{
    qreal dx = 0;
    qreal dy = 0;
    qreal absX = abs(m_edge.x());
    qreal absY = abs(m_edge.y());

    if (m_another_edge.x() >= 0 && m_another_edge.y() < 0)
    {
        dx = absX - m_another_edge.x();
        dy = absY + m_another_edge.y();
    }
    else if (m_another_edge.x() < 0 && m_another_edge.y() < 0)
    {
        dx = absX + m_another_edge.x();
        dy = absY + m_another_edge.y();
    }
    else if (m_another_edge.x() < 0 && m_another_edge.y() >= 0)
    {
        dx = absX + m_another_edge.x();
        dy = absY - m_another_edge.y();
    }
    else if (m_another_edge.x() >= 0 && m_another_edge.y() >= 0)
    {
        dx = absX - m_another_edge.x();
        dy = absY - m_another_edge.y();
    }

    m_radius = dx >= dy ? dx : dy;
}

void BRounded_Rectangle::updateAnotherEdge(QPointF p)
{
    qreal retX = 0;
    qreal retY = 0;

    if (p.x() == m_another_edge.x())
    {
        retX = m_edge.x();
        retY = (-1) * m_edge.y() + m_radius;
    }
    else
    {
        retX = m_edge.x() - m_radius;
        retY = (-1) * m_edge.y();
    }

    m_another_edge.setX(retX);
    m_another_edge.setY(retY);
    m_pointList.at(2)->setPoint(m_another_edge);
}

qreal BRounded_Rectangle::getRadius()
{
    return m_radius;
}

QPointF BRounded_Rectangle::getAnotherEdge()
{
    return m_another_edge;
}

void BRounded_Rectangle::setAnotherEdge(QPointF p)
{
    m_another_edge = p;
}

void BRounded_Rectangle::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->setPen(this->pen());
    painter->setBrush(this->brush());

    QRectF ret(m_center.x() - m_edge.x(), m_center.y() - m_edge.y(), m_edge.x() * 2, m_edge.y() * 2);
    painter->drawRoundedRect(ret, m_radius, m_radius);
}

void BRounded_Rectangle::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    if (!this->isSelected())
        return;

    QMenu *menu = new QMenu();
    menu->setStyleSheet("QMenu { background-color:rgb(89,87,87); border: 5px solid rgb(235,110,36); }");

    QSpinBox *width_spinBox = new QSpinBox(menu);
    width_spinBox->setStyleSheet("QSpinBox{ width:120px; height:30px; font-size:16px; font-weight:bold; }");
    width_spinBox->setRange(0, 1000);
    width_spinBox->setPrefix("w: ");
    width_spinBox->setSuffix(" mm");
    width_spinBox->setSingleStep(1);
    width_spinBox->setValue(2 * abs(m_edge.x()));
    connect(width_spinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [=](int v) {
        m_another_edge.setX(m_another_edge.x() + v / 2 - m_edge.x());
        m_edge.setX(v / 2);

        m_pointList.at(0)->setPoint(m_edge);
        m_pointList.at(2)->setPoint(m_another_edge);
        this->hide();
        this->update();
        this->show();
    });

    QSpinBox *height__spinBox = new QSpinBox(menu);
    height__spinBox->setStyleSheet("QSpinBox{ width:120px; height:30px; font-size:16px; font-weight:bold; }");
    height__spinBox->setRange(0, 1000);
    height__spinBox->setPrefix("h: ");
    height__spinBox->setSuffix(" mm");
    height__spinBox->setSingleStep(1);
    height__spinBox->setValue(2 * abs(m_edge.y()));
    connect(height__spinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [=](int v) {
        m_another_edge.setY(m_another_edge.y() - v / 2 + m_edge.y());
        m_edge.setY(v / 2);

        m_pointList.at(0)->setPoint(m_edge);
        m_pointList.at(2)->setPoint(m_another_edge);
        this->hide();
        this->update();
        this->show();
    });

    QSpinBox *radius__spinBox = new QSpinBox(menu);
    radius__spinBox->setStyleSheet("QSpinBox{ width:120px; height:30px; font-size:16px; font-weight:bold; }");
    radius__spinBox->setRange(0, 1000);
    radius__spinBox->setPrefix("r: ");
    radius__spinBox->setSuffix(" mm");
    radius__spinBox->setSingleStep(1);
    radius__spinBox->setValue(m_radius);
    connect(radius__spinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [=](int v) {
        if (m_another_edge.x() >= abs(m_another_edge.y()))
        {
            m_radius = v <= m_edge.y() ? v : m_edge.y();
            m_another_edge.setY((m_edge.y() - m_radius) * (-1));
        }
        else
        {
            m_radius = v <= m_edge.x() ? v : m_edge.x();
            m_another_edge.setX(m_edge.x() - m_radius);
        }

        m_pointList.at(2)->setPoint(m_another_edge);
        this->hide();
        this->update();
        this->show();
    });

    QWidgetAction *width_widgetAction = new QWidgetAction(menu);
    width_widgetAction->setDefaultWidget(width_spinBox);
    menu->addAction(width_widgetAction);

    QWidgetAction *height_widgetAction = new QWidgetAction(menu);
    height_widgetAction->setDefaultWidget(height__spinBox);
    menu->addAction(height_widgetAction);

    QWidgetAction *radius_widgetAction = new QWidgetAction(menu);
    radius_widgetAction->setDefaultWidget(radius__spinBox);
    menu->addAction(radius_widgetAction);

    menu->exec(QCursor::pos());
    delete menu;

    QGraphicsItem::contextMenuEvent(event);
}

//------------------------------------------------------------------------------
