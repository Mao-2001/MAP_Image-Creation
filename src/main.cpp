#include "mainwindow.h"
#include <QApplication>
#include <QDir>
#include <QCoreApplication>

static void setupGdalDataPaths()
{
    QString appDir = QCoreApplication::applicationDirPath();

    QStringList searchDirs = {
        "gdal_data", "gdal-data", "share/gdal",
        "proj_data",  "proj-data",  "share/proj"
    };

    for (const QString& dir : searchDirs) {
        QDir d(appDir + "/" + dir);
        if (d.exists()) {
            if (dir.contains("proj", Qt::CaseInsensitive))
                qputenv("PROJ_LIB", d.absolutePath().toUtf8());
            else
                qputenv("GDAL_DATA", d.absolutePath().toUtf8());
        }
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("天地图地图显示系统");
    app.setOrganizationName("天地图地图");

    setupGdalDataPaths();

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
