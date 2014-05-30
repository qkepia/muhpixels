#include <QtWidgets>
#include "ImageViewer.qt.h"

ImageViewer::ImageViewer()
{
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

    setWindowTitle(tr("Image Viewer"));
    resize(500, 400);
}

void ImageViewer::open()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                    tr("Open File"), QDir::currentPath());
    if (!fileName.isEmpty()) {
        QImage image(fileName);
        if (image.isNull()) {
            QMessageBox::information(this, tr("Image Viewer"),
                                     tr("Cannot load %1.").arg(fileName));
            return;
        }
        m_pImageLabel->setPixmap(QPixmap::fromImage(image));
        m_scaleFactor = 1.0;

        m_pPrintAct->setEnabled(true);
        m_pFitToWindowAct->setEnabled(true);
        updateActions();

        if (!m_pFitToWindowAct->isChecked())
            m_pImageLabel->adjustSize();
    }
}

void ImageViewer::print()
{
    Q_ASSERT(m_pImageLabel->pixmap());
}

void ImageViewer::zoomIn()
{
    scaleImage(1.25);
}

void ImageViewer::zoomOut()
{
    scaleImage(0.8);
}

void ImageViewer::normalSize()
{
    m_pImageLabel->adjustSize();
    m_scaleFactor = 1.0;
}

void ImageViewer::fitToWindow()
{
    bool fitToWindow = m_pFitToWindowAct->isChecked();
    m_pScrollArea->setWidgetResizable(fitToWindow);
    if (!fitToWindow) {
        normalSize();
    }
    updateActions();
}


void ImageViewer::about()
{
    QMessageBox::about(this, tr("About Image Viewer"),
            tr("<p>The <b>Image Viewer</b> example shows how to combine QLabel "
               "and QScrollArea to display an image. QLabel is typically used "
               "for displaying a text, but it can also display an image. "
               "QScrollArea provides a scrolling view around another widget. "
               "If the child widget exceeds the size of the frame, QScrollArea "
               "automatically provides scroll bars. </p><p>The example "
               "demonstrates how QLabel's ability to scale its contents "
               "(QLabel::scaledContents), and QScrollArea's ability to "
               "automatically resize its contents "
               "(QScrollArea::widgetResizable), can be used to implement "
               "zooming and scaling features. </p><p>In addition the example "
               "shows how to use QPainter to print an image.</p>"));
}

void ImageViewer::createActions()
{
    m_pOpenAct = new QAction(tr("&Open..."), this);
    m_pOpenAct->setShortcut(tr("Ctrl+O"));
    connect(m_pOpenAct, SIGNAL(triggered()), this, SLOT(open()));

    m_pPrintAct = new QAction(tr("&Print..."), this);
    m_pPrintAct->setShortcut(tr("Ctrl+P"));
    m_pPrintAct->setEnabled(false);
    connect(m_pPrintAct, SIGNAL(triggered()), this, SLOT(print()));

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

void ImageViewer::createMenus()
{
    m_pFileMenu = new QMenu(tr("&File"), this);
    m_pFileMenu->addAction(m_pOpenAct);
    m_pFileMenu->addAction(m_pPrintAct);
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

void ImageViewer::updateActions()
{
    m_pZoomInAct->setEnabled(!m_pFitToWindowAct->isChecked());
    m_pZoomOutAct->setEnabled(!m_pFitToWindowAct->isChecked());
    m_pNormalSizeAct->setEnabled(!m_pFitToWindowAct->isChecked());
}

void ImageViewer::scaleImage(double factor)
{
    Q_ASSERT(m_pImageLabel->pixmap());
    m_scaleFactor *= factor;
    m_pImageLabel->resize(m_scaleFactor * m_pImageLabel->pixmap()->size());

    adjustScrollBar(m_pScrollArea->horizontalScrollBar(), factor);
    adjustScrollBar(m_pScrollArea->verticalScrollBar(), factor);

    m_pZoomInAct->setEnabled(m_scaleFactor < 3.0);
    m_pZoomOutAct->setEnabled(m_scaleFactor > 0.333);
}

void ImageViewer::adjustScrollBar(QScrollBar* scrollBar, double factor)
{
    scrollBar->setValue(int(factor * scrollBar->value()
                            + ((factor - 1) * scrollBar->pageStep()/2)));
}
