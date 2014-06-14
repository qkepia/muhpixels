//-----------------------------------------------------------------------------------------------// 
// RawFileMap.cpp
//-----------------------------------------------------------------------------------------------// 
#include <QtWidgets>
#include <RawFileMap.qt.h>

namespace mpx {

//-----------------------------------------------------------------------------------------------// 

RawFileMap::RawFileMap(QWidget* pParent)
	: QWidget(pParent)
{
	setMinimumHeight(100);
	
}

//-----------------------------------------------------------------------------------------------// 

void RawFileMap::paintEvent(QPaintEvent*)
{
	QPainter painter(this);

	QPen pen(Qt::black);
	pen.setWidth(2);

	QBrush brush(Qt::blue);
    painter.setPen(pen);
    painter.setBrush(brush);

	int margin = 3;
	int barHeight = height() - 2 * margin;
	int barWidth = 5;
	QRect rect(margin, margin, barWidth, barHeight);
	for(int i = 0; i < 20; i++)
	{
		painter.drawRect(rect);
		rect.setLeft(rect.left() + margin + barWidth);
		rect.setRight(rect.right() + margin + barWidth);
	}
}

//-----------------------------------------------------------------------------------------------// 


void RawFileMap::mouseMoveEvent(QMouseEvent* pEvent)
{
	pEvent;
}

//-----------------------------------------------------------------------------------------------// 

void RawFileMap::resizeEvent(QResizeEvent* pEvent)
{
	pEvent;
}

//-----------------------------------------------------------------------------------------------// 

} // mpx

