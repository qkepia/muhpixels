//-----------------------------------------------------------------------------------------------// 
// MainWindow.cpp
//-----------------------------------------------------------------------------------------------// 
#include <QtWidgets>
#include <GUI/MainWindow.qt.h>
#include <GUI/RawFileMap.qt.h>

using namespace mpx::GUI;

//-----------------------------------------------------------------------------------------------// 

MainWindow::MainWindow()
{
	// Setup image label as the central widget.
	m_pImageLabel = new QLabel;
	m_pImageLabel->setBackgroundRole(QPalette::Base);
	m_pImageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	m_pImageLabel->setScaledContents(true);

	m_pScrollArea = new QScrollArea;
	m_pScrollArea->setBackgroundRole(QPalette::Dark);
	m_pScrollArea->setWidget(m_pImageLabel);
	setCentralWidget(m_pScrollArea);

	createActions();
	createMenus();
	    
	// Add raw file map as a docked widget.	
	QDockWidget* pDock = new QDockWidget(tr("Raw File Map"), this);
	pDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
	addDockWidget(Qt::BottomDockWidgetArea, pDock);
	RawFileMap* pRawFileMap = new RawFileMap();	
	pDock->setWidget(pRawFileMap);

	setWindowTitle(tr("MUH PIXELS"));
	resize(1280, 720);
}

//-----------------------------------------------------------------------------------------------// 

void MainWindow::open()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                    tr("Open File"), QDir::currentPath());
    if (!fileName.isEmpty()) {
        QImage image(fileName);
        if (image.isNull()) {
            QMessageBox::information(this, tr("MUH PIXELS"),
                                     tr("Cannot load %1.").arg(fileName));
            return;
        }
        m_pImageLabel->setPixmap(QPixmap::fromImage(image));
        m_scaleFactor = 1.0;

        m_pFitToWindowAct->setEnabled(true);
        updateActions();

        if (!m_pFitToWindowAct->isChecked())
            m_pImageLabel->adjustSize();
    }
}

//-----------------------------------------------------------------------------------------------// 

void MainWindow::zoomIn()
{
    scaleImage(1.25);
}

//-----------------------------------------------------------------------------------------------// 

void MainWindow::zoomOut()
{
    scaleImage(0.8);
}

//-----------------------------------------------------------------------------------------------// 

void MainWindow::normalSize()
{
    m_pImageLabel->adjustSize();
    m_scaleFactor = 1.0;
}

//-----------------------------------------------------------------------------------------------// 

void MainWindow::fitToWindow()
{
    bool fitToWindow = m_pFitToWindowAct->isChecked();
    m_pScrollArea->setWidgetResizable(fitToWindow);
    if (!fitToWindow) {
        normalSize();
    }
    updateActions();
}

//-----------------------------------------------------------------------------------------------// 

void MainWindow::about()
{
    QMessageBox::about(this, tr("MUH PIXELS"),
		tr("<p>Visualize information about VP9 coded videos.</p>"
		   "<p>At least that's the plan.</p>")
	);
}

//-----------------------------------------------------------------------------------------------// 

void MainWindow::createActions()
{
    m_pOpenAct = new QAction(tr("&Open..."), this);
    m_pOpenAct->setShortcut(tr("Ctrl+O"));
    connect(m_pOpenAct, SIGNAL(triggered()), this, SLOT(open()));

    m_pExitAct = new QAction(tr("E&xit"), this);
    m_pExitAct->setShortcut(tr("Ctrl+Q"));
    connect(m_pExitAct, SIGNAL(triggered()), this, SLOT(close()));

    m_pZoomInAct = new QAction(tr("Zoom &In (25%)"), this);
    m_pZoomInAct->setShortcut(tr("Ctrl++"));
    m_pZoomInAct->setEnabled(false);
    connect(m_pZoomInAct, SIGNAL(triggered()), this, SLOT(zoomIn()));

    m_pZoomOutAct = new QAction(tr("Zoom &Out (25%)"), this);
    m_pZoomOutAct->setShortcut(tr("Ctrl+-"));
    m_pZoomOutAct->setEnabled(false);
    connect(m_pZoomOutAct, SIGNAL(triggered()), this, SLOT(zoomOut()));

    m_pNormalSizeAct = new QAction(tr("&Normal Size"), this);
    m_pNormalSizeAct->setShortcut(tr("Ctrl+S"));
    m_pNormalSizeAct->setEnabled(false);
    connect(m_pNormalSizeAct, SIGNAL(triggered()), this, SLOT(normalSize()));

    m_pFitToWindowAct = new QAction(tr("&Fit to Window"), this);
    m_pFitToWindowAct->setEnabled(false);
    m_pFitToWindowAct->setCheckable(true);
    m_pFitToWindowAct->setShortcut(tr("Ctrl+F"));
    connect(m_pFitToWindowAct, SIGNAL(triggered()), this, SLOT(fitToWindow()));

    m_pAboutAct = new QAction(tr("&About"), this);
    connect(m_pAboutAct, SIGNAL(triggered()), this, SLOT(about()));
}

//-----------------------------------------------------------------------------------------------// 

void MainWindow::createMenus()
{
    m_pFileMenu = new QMenu(tr("&File"), this);
    m_pFileMenu->addAction(m_pOpenAct);
    m_pFileMenu->addSeparator();
    m_pFileMenu->addAction(m_pExitAct);

    m_pViewMenu = new QMenu(tr("&View"), this);
    m_pViewMenu->addAction(m_pZoomInAct);
    m_pViewMenu->addAction(m_pZoomOutAct);
    m_pViewMenu->addAction(m_pNormalSizeAct);
    m_pViewMenu->addSeparator();
    m_pViewMenu->addAction(m_pFitToWindowAct);

    m_pHelpMenu = new QMenu(tr("&Help"), this);
    m_pHelpMenu->addAction(m_pAboutAct);

    menuBar()->addMenu(m_pFileMenu);
    menuBar()->addMenu(m_pViewMenu);
    menuBar()->addMenu(m_pHelpMenu);
}

//-----------------------------------------------------------------------------------------------// 

void MainWindow::updateActions()
{
    m_pZoomInAct->setEnabled(!m_pFitToWindowAct->isChecked());
    m_pZoomOutAct->setEnabled(!m_pFitToWindowAct->isChecked());
    m_pNormalSizeAct->setEnabled(!m_pFitToWindowAct->isChecked());
}

//-----------------------------------------------------------------------------------------------// 

void MainWindow::scaleImage(double factor)
{
    Q_ASSERT(m_pImageLabel->pixmap());
    m_scaleFactor *= factor;
    m_pImageLabel->resize(m_scaleFactor * m_pImageLabel->pixmap()->size());

    adjustScrollBar(m_pScrollArea->horizontalScrollBar(), factor);
    adjustScrollBar(m_pScrollArea->verticalScrollBar(), factor);

    m_pZoomInAct->setEnabled(m_scaleFactor < 3.0);
    m_pZoomOutAct->setEnabled(m_scaleFactor > 0.333);
}


//-----------------------------------------------------------------------------------------------// 

void MainWindow::adjustScrollBar(QScrollBar* scrollBar, double factor)
{
    scrollBar->setValue(int(factor * scrollBar->value()
                            + ((factor - 1) * scrollBar->pageStep()/2)));
}
