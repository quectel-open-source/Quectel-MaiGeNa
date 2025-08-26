#pragma once
#pragma execution_character_set("utf-8")

#include <QObject>
#include <QGroupBox>
#include <QPainter>
#include <QStyle>
#include <QVariant>
#include "define.h"

class QtMyGroupBox  : public QGroupBox
{
	Q_OBJECT

public:
	QtMyGroupBox(QWidget *parent);
	~QtMyGroupBox();
public:
	void paintEvent(QPaintEvent *event) override;
	/*生成内侧阴影边框
	painter:输入窗口绘图对象
	nWidget:边框宽度
	pTopLeft:输入窗口左顶点
	pTopRight：输入窗口右顶点
	pBottomLeft：输入窗口左底点
	pButtomRight：输入窗口右底点
	*/
	void getInternalFrame(QPainter& painter,int nWidget,QPoint pTopLeft, QPoint pTopRight, QPoint pBottomLeft, QPoint pButtomRight);
};
