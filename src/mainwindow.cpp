#include "mainwindow.h"
#include "keydialog.h"
#include "raster_renderer.h"
#include "geo_tiff_loader.h"
#include "shapefile_loader.h"
#include "colormapdialog.h"
#include "resampledialog.h"
#include "gif_exporter.h"
#include "tiff_utils.h"
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDir>
#include <QApplication>
#include <QFileDialog>
#include <QColorDialog>
#include <QBuffer>
#include <QImage>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QFileInfo>
#include <gdal_priv.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    settings = new QSettings("天地图地图", "config_v1.3", this);
    webView = new QWebEngineView(this);
    webChannel = new QWebChannel(this);
    webView->page()->setWebChannel(webChannel);
    webChannel->registerObject(QStringLiteral("app"), this);
    setCentralWidget(webView);

    setWindowTitle("MAP v1.3 - 地理信息系统");
    resize(1200, 800);

    setupMenu();
    loadKey();

    if (tiandituKey.isEmpty()) {
        KeyDialog dialog(this);
        if (dialog.exec() == QDialog::Accepted) {
            tiandituKey = dialog.getKey();
            saveKey(tiandituKey);
        } else {
            QMessageBox::critical(this, "错误", "必须输入密钥才能使用地图服务！");
            close();
            return;
        }
    }

    loadMap();
}

MainWindow::~MainWindow()
{
}

void MainWindow::reloadMap()
{
    exportState = ExportNone;
    loadMap();
}

void MainWindow::setupMenu()
{
    QMenuBar *menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    QMenu *fileMenu = menuBar->addMenu("文件");
    openTiffAction = fileMenu->addAction("打开 TIFF");
    openTiffsAction = fileMenu->addAction("打开 TIFFs (批量导入)");
    openShapefileAction = fileMenu->addAction("打开 SHP");
    fileMenu->addSeparator();
    playAnimationAction = fileMenu->addAction("打开多波段 TIFF");
    fileMenu->addSeparator();
    QAction *exitAction = fileMenu->addAction("退出");
    connect(openTiffAction, &QAction::triggered, this, &MainWindow::onOpenTiff);
    connect(openTiffsAction, &QAction::triggered, this, &MainWindow::onOpenTiffs);
    connect(openShapefileAction, &QAction::triggered, this, &MainWindow::onOpenShapefile);
    connect(playAnimationAction, &QAction::triggered, this, &MainWindow::onPlayAnimation);
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);

    QMenu *toolMenu = menuBar->addMenu("工具");
    singleResampleAction = toolMenu->addAction("单文件降采样");
    batchResampleAction = toolMenu->addAction("批量降采样");
    toolMenu->addSeparator();
    exportGifAction = toolMenu->addAction("导出图片 (PNG/GIF)");
    connect(singleResampleAction, &QAction::triggered, this, &MainWindow::onSingleResample);
    connect(batchResampleAction, &QAction::triggered, this, &MainWindow::onBatchResample);
    connect(exportGifAction, &QAction::triggered, this, &MainWindow::onExportCurrent);
}

void MainWindow::loadKey()
{
    tiandituKey = settings->value("tianditu/key", "").toString();
}

void MainWindow::saveKey(const QString &key)
{
    settings->setValue("tianditu/key", key);
    settings->sync();
}

void MainWindow::loadMap()
{
    QFile file(QApplication::applicationDirPath() + "/map.html");
    if (!file.exists()) {
        file.setFileName(QDir::currentPath() + "/map.html");
    }
    if (!file.exists()) {
        QDir dir(QApplication::applicationFilePath());
        dir.cdUp();
        file.setFileName(dir.absolutePath() + "/map.html");
    }

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        QString html = stream.readAll();
        file.close();

        html.replace("{{TIANDITU_KEY}}", tiandituKey);
        webView->setHtml(html);
    } else {
        QMessageBox::critical(this, "错误", "无法加载地图文件！\n请确保map.html在当前目录或程序目录中。");
    }
}

void MainWindow::onOpenTiff()
{
    QString fileName = QFileDialog::getOpenFileName(this, "打开 TIFF 文件", "", "TIFF Files (*.tif *.tiff *.TIF *.TIFF)");
    if (fileName.isEmpty()) return;

    GeoTIFFLoader loader;
    if (!loader.load(fileName.toStdString())) {
        QMessageBox::critical(this, "错误", "无法加载 TIFF 文件！");
        return;
    }

    auto geoInfo = loader.getGeoInfo();
    auto latlon = loader.getWGS84Bounds();

    ColorMapDialog dialog(this);
    RasterRenderer::ColorMap selectedMap = RasterRenderer::getDefaultColorMap();

    if (dialog.exec() == QDialog::Accepted) {
        selectedMap = dialog.getSelectedColorMap();
    }

    QImage image = loader.renderToQImage(selectedMap);

    if (image.isNull()) {
        QMessageBox::critical(this, "错误", "渲染 TIFF 数据失败！");
        return;
    }

    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");

    image.save("/tmp/debug_tiff.png");

    QString base64 = QString(ba.toBase64());

    QString js = QString("addTiffImage(\"data:image/png;base64,%1\", [[%2, %3], [%4, %5]]);")
        .arg(base64)
        .arg(latlon.south, 0, 'f', 6).arg(latlon.west, 0, 'f', 6)
        .arg(latlon.north, 0, 'f', 6).arg(latlon.east, 0, 'f', 6);
    webView->page()->runJavaScript(js);

    QMessageBox::information(this, "成功", QString("TIFF 文件已加载！\n尺寸: %1x%2\n色条: %3\n范围: %4, %5, %6, %7\n最小值: %8, 最大值: %9\n调试图片: /tmp/debug_tiff.png")
        .arg(geoInfo.width).arg(geoInfo.height)
        .arg(QString::fromStdString(selectedMap.name))
        .arg(latlon.west, 0, 'f', 4).arg(latlon.south, 0, 'f', 4).arg(latlon.east, 0, 'f', 4).arg(latlon.north, 0, 'f', 4)
        .arg(geoInfo.minValue).arg(geoInfo.maxValue));

    exportState = ExportSingleTiff;
    exportImage = image;
    exportColorMap = selectedMap;
}

void MainWindow::onOpenShapefile()
{
    QString fileName = QFileDialog::getOpenFileName(this, "打开 SHP 文件", "", "Shapefile Files (*.shp *.SHP)");
    if (fileName.isEmpty()) return;

    QColor color = QColorDialog::getColor(Qt::blue, this, "选择 SHP 显示颜色");
    if (!color.isValid()) return;

    ShapefileLoader loader;
    if (!loader.load(fileName.toStdString())) {
        QMessageBox::critical(this, "错误", "无法加载 SHP 文件！");
        return;
    }

    auto layerInfo = loader.getLayerInfo();
    std::string geojsonStr = loader.toGeoJSON(color.name().toStdString(), 0.6);
    QString geojson = QString::fromStdString(geojsonStr);

    // 使用 QJsonDocument 验证和格式化 JSON
    QJsonDocument doc = QJsonDocument::fromJson(geojson.toUtf8());
    if (doc.isNull()) {
        QMessageBox::critical(this, "错误", "GeoJSON 格式错误！");
        return;
    }

    // 使用紧凑格式，避免换行符问题
    QString compactJson = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    QString js = QString("addShapefileLayer(%1);").arg(compactJson);
    webView->page()->runJavaScript(js, [](const QVariant& result) {
        qDebug() << "addShapefileLayer result:" << result;
    });

    QMessageBox::information(this, "成功", QString("SHP 文件已加载！\n图层: %1\n要素数量: %2")
        .arg(QString::fromStdString(layerInfo.name))
        .arg(layerInfo.featureCount));
}

void MainWindow::onOpenTiffs()
{
    QStringList files = QFileDialog::getOpenFileNames(this, "批量导入 TIFF 文件", "",
        "TIFF Files (*.tif *.tiff *.TIF *.TIFF)");
    if (files.isEmpty()) return;

    importedTiffFiles = files;
    QString orderInfo = "已导入 " + QString::number(files.size()) + " 个文件\n选择顺序即后续波段顺序:\n";
    for (int i = 0; i < files.size(); ++i) {
        QFileInfo fi(files[i]);
        orderInfo += QString("  %1: %2\n").arg(i + 1).arg(fi.fileName());
        if (i >= 20) { orderInfo += "  ...(更多)"; break; }
    }
    QMessageBox::information(this, "导入成功", orderInfo);
}

void MainWindow::onSingleResample()
{
    QString input = QFileDialog::getOpenFileName(this, "选择 TIFF 文件", "",
        "TIFF Files (*.tif *.tiff *.TIF *.TIFF)");
    if (input.isEmpty()) return;

    ResampleDialog resampleDialog(this);
    if (resampleDialog.exec() != QDialog::Accepted) return;

    double resolution = resampleDialog.getResolution();
    std::string method = resampleDialog.getResampleMethod();

    QFileInfo fi(input);
    QString defaultName = fi.absolutePath() + "/" + fi.completeBaseName()
        + "_" + QString::number(resolution, 'f', 0) + ".tif";

    QString output = QFileDialog::getSaveFileName(this, "保存降采样 TIFF", defaultName,
        "TIFF Files (*.tif *.tiff)");
    if (output.isEmpty()) return;

    QString errorMsg;
    if (TiffUtils::runGdalwarp(input, output, resolution, method, errorMsg)) {
        QMessageBox::information(this, "成功",
            QString("降采样完成！\n文件: %1\n分辨率: %2m\n方法: %3\n坐标: EPSG:3857")
                .arg(output).arg(resolution).arg(QString::fromStdString(method)));
    } else {
        QMessageBox::critical(this, "错误",
            QString("降采样失败:\n%1").arg(errorMsg));
    }
}

void MainWindow::onBatchResample()
{
    if (importedTiffFiles.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先通过 文件 → 打开 TIFFs 导入文件");
        return;
    }

    ResampleDialog resampleDialog(this);
    if (resampleDialog.exec() != QDialog::Accepted) return;

    double resolution = resampleDialog.getResolution();
    std::string method = resampleDialog.getResampleMethod();

    QString outputFile = QFileDialog::getSaveFileName(this, "保存多波段 TIFF", "merged.tif",
        "TIFF Files (*.tif *.tiff)");
    if (outputFile.isEmpty()) return;

    QTemporaryDir tmpDir;
    if (!tmpDir.isValid()) {
        QMessageBox::critical(this, "错误", "无法创建临时目录");
        return;
    }
    QString resampleDir = tmpDir.path() + "/";

    QStringList resampledFiles;
    int total = importedTiffFiles.size();
    bool error = false;

    for (int i = 0; i < total; ++i) {
        QString input = importedTiffFiles[i];
        QString output = resampleDir + QString("resampled_%1.tif").arg(i);

        QString errorMsg;
        if (!TiffUtils::runGdalwarp(input, output, resolution, method, errorMsg)) {
            QMessageBox::critical(this, "错误",
                QString("降采样失败: %1\n%2").arg(input).arg(errorMsg));
            error = true;
            break;
        }
        resampledFiles << output;
    }

    if (!error) {
        error = !TiffUtils::createMultiBandTiff(resampledFiles, outputFile);
        if (!error) {
            QString orderInfo = "波段顺序按文件选择顺序排列:\n";
            for (int i = 0; i < importedTiffFiles.size(); ++i) {
                QFileInfo fi(importedTiffFiles[i]);
                orderInfo += QString("  波段%1: %2\n").arg(i + 1).arg(fi.fileName());
            }
            QMessageBox::information(this, "成功",
                QString("降采样完成！共 %1 个波段\n保存至: %2\n\n%3")
                    .arg(resampledFiles.size()).arg(outputFile).arg(orderInfo));
        } else {
            QMessageBox::critical(this, "错误", "合并多波段 TIFF 失败！\n各文件尺寸不一致或读取错误。");
        }
    }

    for (const QString& f : resampledFiles) QFile::remove(f);
}

void MainWindow::onPlayAnimation()
{
    QString tiffFile = QFileDialog::getOpenFileName(this, "选择多波段 TIFF 文件", "",
        "TIFF Files (*.tif *.tiff)");
    if (tiffFile.isEmpty()) return;

    ColorMapDialog colorDialog(this);
    if (colorDialog.exec() != QDialog::Accepted) return;
    auto selectedMap = colorDialog.getSelectedColorMap();

    GDALAllRegister();

    GDALDataset* dataset = (GDALDataset*)GDALOpen(tiffFile.toStdString().c_str(), GA_ReadOnly);
    if (!dataset) {
        QMessageBox::critical(this, "错误", "无法打开文件！");
        return;
    }

    int width = dataset->GetRasterXSize();
    int height = dataset->GetRasterYSize();
    int numBands = dataset->GetRasterCount();

    if (numBands < 2) {
        QMessageBox::critical(this, "错误", "至少需要 2 个波段！");
        GDALClose(dataset);
        return;
    }

    double gt[6];
    dataset->GetGeoTransform(gt);

    const char* projStr = dataset->GetProjectionRef();
    auto bounds = TiffUtils::computeWGS84Bounds(gt, width, height, projStr);

    QString js = "playAnimation([";

    for (int b = 0; b < numBands; ++b) {
        GDALRasterBand* band = dataset->GetRasterBand(b + 1);
        if (!band) continue;

        std::vector<double> data(width * height);
        if (band->RasterIO(GF_Read, 0, 0, width, height,
                           data.data(), width, height, GDT_Float64, 0, 0) != CE_None) continue;

        double nodata = band->GetNoDataValue();
        double minVal = data[0], maxVal = data[0];
        for (double v : data) {
            if (std::isnan(v) || std::isinf(v)) continue;
            if (!std::isnan(nodata) && v == nodata) continue;
            if (v < minVal) minVal = v;
            if (v > maxVal) maxVal = v;
        }

        QImage frame = RasterRenderer::renderFrame(data, width, height, selectedMap,
                                                    minVal, maxVal, nodata);

        QByteArray ba;
        QBuffer buf(&ba);
        buf.open(QIODevice::WriteOnly);
        frame.save(&buf, "PNG");

        if (b > 0) js += ",";
        js += "\"data:image/png;base64," + QString::fromLatin1(ba.toBase64()) + "\"";
    }

    GDALClose(dataset);

    js += QString("],[[%1,%2],[%3,%4]],10);")
        .arg(bounds.south, 0, 'f', 6).arg(bounds.west, 0, 'f', 6)
        .arg(bounds.north, 0, 'f', 6).arg(bounds.east, 0, 'f', 6);

    webView->page()->runJavaScript(js);

    QMessageBox::information(this, "成功",
        QString("动画已加载！共 %1 帧，按 10fps 播放").arg(numBands));

    exportState = ExportAnimation;
    exportAnimPath = tiffFile;
    exportColorMap = selectedMap;
}

void MainWindow::onExportCurrent()
{
    if (exportState == ExportNone) {
        QMessageBox::warning(this, "提示", "没有可导出的内容。\n请先打开 TIFF 或播放动画。");
        return;
    }

    if (exportState == ExportSingleTiff) {
        QString fileName = QFileDialog::getSaveFileName(this, "导出 PNG", "export.png",
            "PNG Files (*.png)");
        if (fileName.isEmpty()) return;

        if (exportImage.save(fileName, "PNG")) {
            QMessageBox::information(this, "成功",
                QString("PNG 导出成功！\n文件: %1\n色条: %2")
                    .arg(fileName).arg(QString::fromStdString(exportColorMap.name)));
        } else {
            QMessageBox::critical(this, "错误", "PNG 导出失败！");
        }
        return;
    }

    if (exportState == ExportAnimation) {
        QString outputFile = QFileDialog::getSaveFileName(this, "导出 GIF", "animation.gif",
            "GIF Files (*.gif)");
        if (outputFile.isEmpty()) return;

        GifExporter exporter;
        if (exporter.exportToGif(exportAnimPath.toStdString(), exportColorMap,
                                  outputFile.toStdString())) {
            QMessageBox::information(this, "成功", QString("GIF 导出成功！\n文件: %1").arg(outputFile));
        } else {
            QMessageBox::critical(this, "错误",
                QString("GIF 导出失败！\n原因:\n%1").arg(QString::fromStdString(exporter.getLastError())));
        }
    }
}

