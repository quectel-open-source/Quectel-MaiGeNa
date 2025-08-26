#ifndef __IMAGE_VIEWER_H__
#define __IMAGE_VIEWER_H__

#include <QFrame>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QImage>
#include <QMouseEvent>
#include <QWheelEvent>
#include <qgraphicsview.h>

namespace Ui
{
class ImageViewer;
}

class ImageViewer : public QFrame
{
    Q_OBJECT

  private:
    Ui::ImageViewer *m_ui;

  protected:
    QGraphicsPixmapItem *m_pixitem = nullptr;
    bool m_show_image = true;
    int m_currentZoom = 100;

  public:
    ImageViewer(QWidget *parent = nullptr);
    virtual ~ImageViewer();

  protected:
    QWidget *customWidget();

  public:
    void show_image(const QImage &image);
    void set_show_image_flag(bool flag = true)
    {
        m_show_image = flag;
    }

    QGraphicsView * view();
    QGraphicsScene * scene(){return view()->scene();}

  protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void onWheelEvent(QWheelEvent *event);
    void onMouseMoveEvent(QMouseEvent *event);

  private slots:
    void on_btn_zoom_out_clicked();
    void on_spin_zoom_valueChanged(int value);
    void on_btn_zoom_in_clicked();
    void on_btn_fitscreen_clicked(bool checked);

  private:
    void fitImage();
};

#endif