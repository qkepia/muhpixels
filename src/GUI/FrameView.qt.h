//-----------------------------------------------------------------------------------------------// 
// FrameView.qt.h
//-----------------------------------------------------------------------------------------------// 
#ifndef MPX_GUI_FRAME_VIEW_H
#define MPX_GUI_FRAME_VIEW_H

#include <QGraphicsView>

QT_BEGIN_NAMESPACE
class QGraphicsScene;
QT_END_NAMESPACE

namespace mpx { namespace GUI {

//-----------------------------------------------------------------------------------------------// 
// Analysing view of a single frame
//-----------------------------------------------------------------------------------------------// 
class FrameView : public QGraphicsView
{
    Q_OBJECT

public:
	FrameView(QWidget* pParent = nullptr);

private:
	QGraphicsScene* m_pGraphicsScene;
};

}} // mpx::GUI

//-----------------------------------------------------------------------------------------------// 

#endif
