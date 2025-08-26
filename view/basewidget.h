#pragma once
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>
#include <qapplication.h>
#include <qdebug.h>
#include <qdialog.h>
#include <qevent.h>
#include <qgroupbox.h>
#include <qnamespace.h>
#include <qpainter.h>
#include <qstyle.h>
#include <qtimer.h>
#include <qwidget.h>

inline void paintCard(QWidget *widget);
inline void paintWidget(QWidget *widget);

class BaseCard : public QGroupBox
{
  public:
    explicit BaseCard(QWidget *parent = nullptr) : QGroupBox(parent)
    {
        setStyleSheet("QFrame{border: 0px}"
                      "QListWidget{border: 0px}"
                      "QTableWidget{border: 0px}"
                      "QGroupBox{border: 0px}");

        // setAttribute(Qt::WA_TranslucentBackground);
    }

    void paintEvent(QPaintEvent *event) override
    {
        QGroupBox::paintEvent(event);
        paintCard(this);
    }
};

class BaseWidget : public QWidget
{
  public:
    explicit BaseWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        setStyleSheet("QFrame{border: 0px}"
                      "QListWidget{border: 0px}"
                      "QTableWidget{border: 0px}"
                      "QGroupBox{border: 0px}");

        // setAttribute(Qt::WA_TranslucentBackground);
    }

  protected:
    void paintEvent(QPaintEvent *event) override
    {
        QWidget::paintEvent(event);
        paintWidget(this);
    }
};

/// --------------------
inline void paintCard(QWidget *widget)
{
    QPainter painter(widget);
    painter.setRenderHint(QPainter::Antialiasing, true);

    int nWidget = 12;
    auto rect = widget->rect();
    QPoint pTopLeft = rect.topLeft();
    QPoint pTopRight = rect.topRight();
    QPoint pBottomLeft = rect.bottomLeft();
    QPoint pButtomRight = rect.bottomRight();

    // color1->color2->color3
    QColor color1 = QColor("#254F87");
    QColor color2 = QColor("#102E5F");
    QColor color3 = QColor("#0E1827");
    // top
    QPoint pStart1 = pTopLeft;
    pStart1.setX(pStart1.x() + nWidget);
    QPoint pEnd1(pStart1.x(), pStart1.y() + nWidget);
    QLinearGradient gradient1(pStart1.x(), pStart1.y(), pEnd1.x(), pEnd1.y());
    gradient1.setColorAt(0, color1);
    gradient1.setColorAt(0.3, color2);
    gradient1.setColorAt(1, color3);
    painter.setBrush(gradient1);
    painter.setPen(Qt::transparent);
    painter.drawRect(pStart1.x(), pStart1.y(), rect.width() - 2 * nWidget, nWidget);

    // left
    QPoint pStart2 = pTopLeft;
    pStart2.setY(pStart2.y() + nWidget);
    QPoint pEnd2(pStart2.x() + nWidget, pStart2.y());
    QLinearGradient gradient2(pStart2.x(), pStart2.y(), pEnd2.x(), pEnd2.y());
    gradient2.setColorAt(0, color1);
    gradient2.setColorAt(0.3, color2);
    gradient2.setColorAt(1, color3);
    painter.setBrush(gradient2);
    painter.setPen(Qt::transparent);
    painter.drawRect(pStart2.x(), pStart2.y(), nWidget, rect.height() - 2 * nWidget);

    // bottom
    QPoint pStart3 = pBottomLeft;
    pStart3.setX(pStart3.x() + nWidget);
    QPoint pEnd3(pStart3.x(), pStart3.y() - nWidget);
    QLinearGradient gradient3(pStart3.x(), pStart3.y(), pEnd3.x(), pEnd3.y());
    gradient3.setColorAt(0, color1);
    gradient3.setColorAt(0.3, color2);
    gradient3.setColorAt(1, color3);
    painter.setBrush(gradient3);
    painter.setPen(Qt::transparent);
    painter.drawRect(pEnd3.x(), pEnd3.y(), rect.width() - 2 * nWidget, nWidget);

    // right
    QPoint pStart4 = pTopRight;
    pStart4.setY(pStart4.y() + nWidget);
    QPoint pEnd4(pStart4.x() - nWidget, pStart4.y());
    QLinearGradient gradient4(pStart4.x(), pStart4.y(), pEnd4.x(), pEnd4.y());
    gradient4.setColorAt(0, color1);
    gradient4.setColorAt(0.3, color2);
    gradient4.setColorAt(1, color3);
    painter.setBrush(gradient4);
    painter.setPen(Qt::transparent);
    painter.drawRect(pEnd4.x(), pEnd4.y(), nWidget, rect.height() - 2 * nWidget);

    int x = pTopLeft.x();
    int y = pTopLeft.y();
    QRadialGradient radialGradient(x + nWidget, y + nWidget, nWidget);
    radialGradient.setColorAt(0, color3);
    radialGradient.setColorAt(0.7, color2);
    radialGradient.setColorAt(1, color1);
    painter.setBrush(radialGradient);
    painter.setPen(Qt::transparent);
    QRectF rr(x, y, nWidget * 2, nWidget * 2);
    painter.drawPie(rr, 90 * 16, 90 * 16);

    x = pTopRight.x();
    y = pTopRight.y();
    QRadialGradient radialGradient2(x - nWidget, y + nWidget, nWidget);
    radialGradient2.setColorAt(0, color3);
    radialGradient2.setColorAt(0.7, color2);
    radialGradient2.setColorAt(1, color1);
    painter.setBrush(radialGradient2);
    painter.setPen(Qt::transparent);
    QRectF rr2(x - 2 * nWidget, y, nWidget * 2, nWidget * 2);
    painter.drawPie(rr2, 0 * 16, 90 * 16);

    x = pBottomLeft.x();
    y = pBottomLeft.y();
    QRadialGradient radialGradient3(x + nWidget, y - nWidget, nWidget);
    radialGradient3.setColorAt(0, color3);
    radialGradient3.setColorAt(0.7, color2);
    radialGradient3.setColorAt(1, color1);
    painter.setBrush(radialGradient3);
    painter.setPen(Qt::transparent);
    QRectF rr3(x, y - 2 * nWidget, nWidget * 2, nWidget * 2);
    painter.drawPie(rr3, -90 * 16, -90 * 16);

    x = pButtomRight.x();
    y = pButtomRight.y();
    QRadialGradient radialGradient4(x - nWidget, y - nWidget, nWidget);
    radialGradient4.setColorAt(0, color3);
    radialGradient4.setColorAt(0.7, color2);
    radialGradient4.setColorAt(1, color1);
    painter.setBrush(radialGradient4);
    painter.setPen(Qt::transparent);
    QRectF rr4(x - 2 * nWidget, y - 2 * nWidget, nWidget * 2, nWidget * 2);
    painter.drawPie(rr4, 0 * 16, -90 * 16);

    painter.end();
}

inline void paintWidget(QWidget *widget)
{
    paintCard(widget);
}