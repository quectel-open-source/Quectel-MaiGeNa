#pragma once
#include <QMouseEvent>
#include <qgraphicsview.h>
#include <qobjectdefs.h>
class QDrawerView : public QGraphicsView
{
    Q_OBJECT

  signals:
    void mouseMoved(QPoint pos);
    void zoomChanged(int zoom);

  public:
    QDrawerView(QGraphicsScene *scene, QWidget *parent = nullptr)
    {
        setScene(scene);
        setMouseTracking(true);
        viewport()->installEventFilter(this);
    }
    virtual ~QDrawerView()
    {
    }

  public:
    int zoom() const
    {
        return m_currentZoom;
    }
    void setZoom(int value)
    {
        zoom(value);
    }

  protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (event->type() == QEvent::MouseMove)
        {
            onMouseMoveEvent(static_cast<QMouseEvent *>(event));
        }
        else if (event->type() == QEvent::Wheel)
        {
            onWheelEvent(static_cast<QWheelEvent *>(event));
        }

        return QGraphicsView::eventFilter(watched, event);
    }

    void zoom(int value)
    {
        if (value < m_zoomMin || value > m_zoomMax)
            return;
        m_currentZoom = value;
        double scaleFactor = value / 100.0;
        scale(scaleFactor / transform().m11(), scaleFactor / transform().m11());
        emit zoomChanged(m_currentZoom);
    }
    void onWheelEvent(QWheelEvent *event)
    {
        int angle = event->angleDelta().y();
        if (angle > 0)
        {
            zoom(m_currentZoom + m_zoomLevel);
        }
        else
        {
            zoom(m_currentZoom - m_zoomLevel);
        }
    }
    void onMouseMoveEvent(QMouseEvent *event)
    {
        QGraphicsView *view = this;
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        emit mouseMoved(mouseEvent->pos());
    }

  private:
    int m_currentZoom = 0;
    int m_zoomLevel = 10;
    int m_zoomMin = 10;
    int m_zoomMax = 400;
};