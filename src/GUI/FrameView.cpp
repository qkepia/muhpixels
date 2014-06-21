//-----------------------------------------------------------------------------------------------// 
// FrameView.cpp
//-----------------------------------------------------------------------------------------------// 
#include <QtWidgets>
#include <FrameView.qt.h>
#include <BitStream.h>
#include <FrameBuf.h>
#include <Color.h>
#include <Decode.h>

namespace mpx {

//-----------------------------------------------------------------------------------------------// 

FrameView::FrameView(QWidget* pParent)
	: QGraphicsView(pParent)
{
	m_pGraphicsScene = new QGraphicsScene();
	setScene(m_pGraphicsScene);

#if 1
  	BitStreamInfo bsInfo;
	FrameBuf<RGB8> firstFrame;
	modelBitStream("S:\\my\\data\\knk-vp9.webm", bsInfo, firstFrame);
	QImage image(&firstFrame.data()->r, firstFrame.width(), firstFrame.height(), QImage::Format_RGB888);
	QPixmap pixmap = QPixmap::fromImage(image);
	QGraphicsPixmapItem* pPixmapItem = new QGraphicsPixmapItem(pixmap);
	m_pGraphicsScene->addItem(pPixmapItem);
#endif

	show();
}

//-----------------------------------------------------------------------------------------------// 

} // mpx
