#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWebEngineView>
#include <QWebChannel>
#include <QSettings>
#include <QMenuBar>
#include <QAction>
#include <QStringList>
#include <QImage>
#include "geo_tiff_loader.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    Q_INVOKABLE void reloadMap();

private slots:
    void onOpenTiff();
    void onOpenTiffs();
    void onOpenShapefile();
    void onSingleResample();
    void onBatchResample();
    void onExportCurrent();
    void onPlayAnimation();

private:
    QWebEngineView *webView;
    QWebChannel *webChannel;
    QSettings *settings;
    QString tiandituKey;
    QStringList importedTiffFiles;

    QAction *openTiffAction;
    QAction *openTiffsAction;
    QAction *openShapefileAction;
    QAction *singleResampleAction;
    QAction *batchResampleAction;
    QAction *exportGifAction;
    QAction *playAnimationAction;

    enum ExportState { ExportNone, ExportSingleTiff, ExportAnimation };
    ExportState exportState = ExportNone;
    QImage exportImage;                          // ExportSingleTiff 时使用
    QString exportAnimPath;                      // ExportAnimation 时使用
    GeoTIFFLoader::ColorMap exportColorMap;

    void loadKey();
    void saveKey(const QString &key);
    void loadMap();
    void setupMenu();
    void addTiffLayer(const QString& base64Image, const QString& bounds);
    void addShapefileLayer(const QString& geojson, const QString& color);
    bool createMultiBandTiff(const QStringList& inputFiles, const QString& outputFile);
};

#endif // MAINWINDOW_H
