//-----------------------------------------------------------------------------------------------// 
// main function.
//-----------------------------------------------------------------------------------------------// 

#include <QApplication>
#include <MainWindow.qt.h>

//-----------------------------------------------------------------------------------------------// 

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    mpx::MainWindow mpxWindow;
    mpxWindow.show();
    return app.exec();
}

//-----------------------------------------------------------------------------------------------// 