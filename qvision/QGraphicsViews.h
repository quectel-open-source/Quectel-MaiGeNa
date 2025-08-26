#pragma once
#pragma execution_character_set("utf-8")

#include <QGraphicsView>
#include<QDateTime>
#include <qmutex.h>
#include <QScrollBar>
#include "BaseItem.h"
#include "commonZack.h"
class QLabel;
class QMouseEvent;
class QGraphicsScenes;
class ImageItem;

class QGraphicsViews : public QGraphicsView
{
	Q_OBJECT

public:
	QGraphicsViews(QWidget* parent = nullptr);
	void SetToFit(qreal val);
	void ClearObj();
	QGraphicsScenes* scene;
	DrawItem* drawItem;	

    void needPt();
    QImage getImage();

public slots:
	void DispImage(QImage Image); //显示图片
	void DispPoint(QVector<QPointF>& list, QColor color = QColor(0, 255, 0));//显示点集合
    void AddItems(BaseItem* item_one); //添加ROI
	void SetType(bool InfoFlg, bool GrayValueFlg);//设定显示信息

protected:
	virtual void mousePressEvent(QMouseEvent* event) override;
	virtual void mouseReleaseEvent(QMouseEvent* event) override;
	virtual void mouseDoubleClickEvent(QMouseEvent* event) override;
	virtual void mouseMoveEvent(QMouseEvent* event) override;
	virtual void wheelEvent(QWheelEvent* event) override;
	virtual void resizeEvent(QResizeEvent* event) override;
	virtual void drawBackground(QPainter* painter, const QRectF& rect) override;
	virtual void paintEvent(QPaintEvent* event) override;
	virtual void contextMenuEvent(QContextMenuEvent* event) override;   //右键菜单

private:
	void OnSelectImage();       //选择打开图片
	void OnSaveImage();       //选择保存图片
	void OnZoomInImage();
	void OnZoomOutImage();
	void OnFitImage();
	void horFilp();             //水平翻转
	void verFilp();             //垂直翻转
	void clockwise();            //顺时针旋转
	void anticlockwise();       //逆时针旋转
	void ZoomFrame(double value);
	void GetFit();
	QScrollBar* pHbar;
	QScrollBar* pVbar;
	double ZoomValue = 1;
	double ZoomFit = 1;
	double PixX = 0;
	double PixY = 0;
	QPixmap image;
	ImageItem* item;	
	QWidget* fWidget;	
	QLabel* GrayValue;
	int clickcount = 0;
	QImage Image = QImage();           //显示的图片
	QMutex qmutex;

    QImage m_image;
    QGraphicsTextItem* m_textItem;

public slots:
	void slot_RGBValue(QString InfoVal);
	bool eventFilter(QObject *obj, QEvent *event);//事件过滤
private:
	int MRSize = 8; //拖拽区域大小
	QColor color1 = QColor(30, 30, 30);           //颜色1
	QColor color2 = QColor(50, 50, 50, 255);      //颜色2
	QPixmap bgPix = QPixmap(36, 36);              //背景图片
	void drawBg();
signals:
    void emit_pt(int, int, QPointF);
//    void emit_itemRemove();
};
