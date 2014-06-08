//-----------------------------------------------------------------------------------------------// 
// MainWindow.qt.h
//-----------------------------------------------------------------------------------------------// 
#ifndef MPX_GUI_MAIN_WINDOW_QT_H
#define MPX_GUI_MAIN_WINDOW_QT_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QAction;
class QLabel;
class QMenu;
class QScrollArea;
class QScrollBar;
QT_END_NAMESPACE

namespace mpx { namespace GUI {

//-----------------------------------------------------------------------------------------------// 
// Main application window.
//-----------------------------------------------------------------------------------------------// 
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

private slots:
    void open();
    void zoomIn();
    void zoomOut();
    void normalSize();
    void fitToWindow();
    void about();

private:
    void createActions();
    void createMenus();
    void updateActions();
    void scaleImage(double factor);
    void adjustScrollBar(QScrollBar* scrollBar, double factor);

    QLabel* m_pImageLabel;
    QScrollArea* m_pScrollArea;
    double m_scaleFactor;

    QAction* m_pOpenAct;
    QAction* m_pExitAct;
    QAction* m_pZoomInAct;
    QAction* m_pZoomOutAct;
    QAction* m_pNormalSizeAct;
    QAction* m_pFitToWindowAct;
    QAction* m_pAboutAct;

    QMenu* m_pFileMenu;
    QMenu* m_pViewMenu;
    QMenu* m_pHelpMenu;
};

}} // mpx::GUI

//-----------------------------------------------------------------------------------------------// 

#endif
