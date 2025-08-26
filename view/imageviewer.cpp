#include "imageviewer.h"
#include "ui_imageviewer.h"

ImageViewer::ImageViewer(QWidget *parent) : QFrame(parent), m_ui(new Ui::ImageViewer)
{
    m_ui->setupUi(this);
    m_ui->graphics_view->setScene(new QGraphicsScene);
    m_ui->graphics_view->setMouseTracking(true);
    m_ui->graphics_view->viewport()->installEventFilter(this);
    // m_ui->graphics_view->installEventFilter(this);
    m_ui->graphics_view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    // m_ui->graphics_view->setBackgroundBrush(Qt::gray);
	// 设置场景范围
	// m_ui->graphics_view->setSceneRect(INT_MIN / 2, INT_MIN / 2, INT_MAX, INT_MAX);
	// 反锯齿
	m_ui->graphics_view->setRenderHints(QPainter::Antialiasing);
}

ImageViewer::~ImageViewer()
{
    delete m_ui;
}

QWidget *ImageViewer::customWidget()
{
    if (m_ui != nullptr)
    {
        return m_ui->custom_actions_widget;
    }
    return nullptr;
}

QGraphicsView *ImageViewer::view()
{
    return m_ui->graphics_view;
}
void ImageViewer::show_image(const QImage &image)
{
    if (m_show_image)
    {
        if (m_pixitem == nullptr)
        {
            m_pixitem = new QGraphicsPixmapItem();
            m_ui->graphics_view->scene()->addItem(m_pixitem);
        }
        // m_ui->graphics_view->resetTransform();
        m_ui->graphics_view->scene()->setSceneRect(QRectF(0, 0, image.width(), image.height()));
        m_pixitem->setPixmap(QPixmap::fromImage(image));
        fitImage();
    }
}

void ImageViewer::fitImage()
{
    // if (m_pixitem == nullptr)
    //     return;
    // m_ui->graphics_view->fitInView(m_pixitem, Qt::KeepAspectRatio);
    double scaleFactor = m_ui->graphics_view->transform().m11(); // 或m22，因为m11和m22在KeepAspectRatio模式下相等
    int zoomValue = static_cast<int>(scaleFactor * 100);
    m_ui->spin_zoom->setValue(zoomValue);
}

void ImageViewer::on_btn_zoom_out_clicked()
{
    m_ui->spin_zoom->setValue(m_currentZoom - 10);
}

void ImageViewer::on_btn_zoom_in_clicked()
{
    m_ui->spin_zoom->setValue(m_currentZoom + 10);
}

void ImageViewer::on_spin_zoom_valueChanged(int value)
{
    // if (m_pixitem == nullptr)
    //     return;
    if (value < 10 || value > 400)
        return;
    m_currentZoom = value;
    double scaleFactor = value / 100.0;
    double currentScaleFactor = m_ui->graphics_view->transform().m11();
    m_ui->graphics_view->scale(scaleFactor / currentScaleFactor, scaleFactor / currentScaleFactor);
}

void ImageViewer::on_btn_fitscreen_clicked(bool checked)
{
    fitImage();
}

bool ImageViewer::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseMove)
    {
        onMouseMoveEvent(static_cast<QMouseEvent *>(event));
    }
    else if (event->type() == QEvent::Wheel)
    {
        onWheelEvent(static_cast<QWheelEvent *>(event));
    }

    return QWidget::eventFilter(watched, event);
}

void ImageViewer::onMouseMoveEvent(QMouseEvent *event)
{
    QGraphicsView *view = m_ui->graphics_view;
    QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
    const QPoint mousePos = mouseEvent->pos();
    const QPointF scenePos = view->mapToScene(mouseEvent->pos());

    // 获取像素值
    QColor color;
    if (m_pixitem != nullptr)
    {
        QImage image = m_pixitem->pixmap().toImage();
        if (m_pixitem->boundingRect().contains(mousePos))
        {
            color = image.pixelColor(mousePos);
        }
    }

    // 更新坐标
    m_ui->lb_xy_val->setText(QString::asprintf("%d, %d", mousePos.x(), mousePos.y()));

    if (color.isValid())
    {
        int r, g, b;
        color.getRgb(&r, &g, &b);
        m_ui->lb_rgb_val->setText(QString::asprintf("%d, %d, %d", r, g, b));
    }
}
void ImageViewer::onWheelEvent(QWheelEvent *event)
{
    int angle = event->angleDelta().y();
    if (angle > 0)
    {
        on_btn_zoom_in_clicked();
    }
    else
    {
        on_btn_zoom_out_clicked();
    }
}