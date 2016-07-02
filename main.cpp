
#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    int ret = -1;
    QApplication a(argc, argv);
    MainWindow* mainWindow = MainWindow::NewInstance();

    if(mainWindow)
    {
        mainWindow->show();
        ret = a.exec();

        delete mainWindow;
    }

    return ret;
}
