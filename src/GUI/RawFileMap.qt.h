//-----------------------------------------------------------------------------------------------// 
// RawFileMap.qt.h
//-----------------------------------------------------------------------------------------------// 
#ifndef MPX_GUI_RAW_FILE_MAP_H
#define MPX_GUI_RAW_FILE_MAP_H

#include <QWidget>

namespace mpx {

//-----------------------------------------------------------------------------------------------// 
// Representation of the raw file data.
//-----------------------------------------------------------------------------------------------// 
class RawFileMap : public QWidget
{
    Q_OBJECT

public:
	RawFileMap(QWidget* pParent = nullptr);

    void paintEvent(QPaintEvent* pEvent) override;
    void mouseMoveEvent(QMouseEvent* pEvent) override;
    void resizeEvent(QResizeEvent* pEvent) override;

private:
	
};

//-----------------------------------------------------------------------------------------------// 

} // mpx

//-----------------------------------------------------------------------------------------------// 

#endif
