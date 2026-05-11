#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("天地图地图显示系统");
    app.setOrganizationName("天地图地图");

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
