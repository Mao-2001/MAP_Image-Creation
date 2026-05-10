#include "mainwindow.h"
#include "keydialog.h"
#include "geo_tiff_loader.h"
#include "shapefile_loader.h"
#include "colormapdialog.h"
#include "resampledialog.h"
#include "gif_exporter.h"
#include <QVBoxLayout>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDir>
#include <QApplication>
#include <QFileDialog>
#include <QColorDialog>
#include <QBuffer>
#include <QImage>
#include <QProcess>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QFileInfo>
#include <gdal_priv.h>
#include <ogr_spatialref.h>

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
    connect(playAnimationAction, &QAction::triggered, this, &MainWindow::onPlayAnimation);
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
    GeoTIFFLoader::ColorMap selectedMap = GeoTIFFLoader::getDefaultColorMap();

    if (dialog.exec() == QDialog::Accepted) {
        selectedMap = dialog.getSelectedColorMap();
    }

    auto rgba = loader.renderToRGBA(selectedMap);

    if (rgba.empty()) {
        QMessageBox::critical(this, "错误", "渲染 TIFF 数据失败！");
        return;
    }

    QImage image(geoInfo.width, geoInfo.height, QImage::Format_ARGB32);
    for (int i = 0; i < rgba.size() / 4; ++i) {
        image.setPixelColor(i % geoInfo.width, i / geoInfo.width,
                          QColor(rgba[i*4], rgba[i*4+1], rgba[i*4+2], rgba[i*4+3]));
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

    QString resStr = QString::number(resolution);

    QProcess process;
    process.setProgram("gdalwarp");
    process.setArguments({"-tr", resStr, resStr,
                          "-r", QString::fromStdString(method),
                          "-t_srs", "EPSG:3857",
                          "-ot", "Float32",
                          input, output});
    process.start();
    process.waitForFinished();

    if (process.exitCode() == 0) {
        QMessageBox::information(this, "成功",
            QString("降采样完成！\n文件: %1\n分辨率: %2m\n方法: %3\n坐标: EPSG:3857")
                .arg(output).arg(resolution).arg(QString::fromStdString(method)));
    } else {
        QMessageBox::critical(this, "错误",
            QString("降采样失败:\n%1").arg(QString::fromUtf8(process.readAllStandardError())));
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
        QString resStr = QString::number(resolution);

        QProcess process;
        process.setProgram("gdalwarp");
        process.setArguments({"-tr", resStr, resStr,
                              "-r", QString::fromStdString(method),
                              "-t_srs", "EPSG:3857",
                              "-ot", "Float32", input, output});
        process.start();
        process.waitForFinished();

        if (process.exitCode() != 0) {
            QMessageBox::critical(this, "错误",
                QString("降采样失败: %1\n%2").arg(input, QString::fromUtf8(process.readAllStandardError())));
            error = true;
            break;
        }
        resampledFiles << output;
    }

    if (!error) {
        error = !createMultiBandTiff(resampledFiles, outputFile);
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

bool MainWindow::createMultiBandTiff(const QStringList& inputFiles, const QString& outputFile)
{
    if (inputFiles.isEmpty()) return false;

    GDALAllRegister();

    GDALDataset* firstDS = (GDALDataset*)GDALOpen(inputFiles[0].toStdString().c_str(), GA_ReadOnly);
    if (!firstDS) return false;

    int width = firstDS->GetRasterXSize();
    int height = firstDS->GetRasterYSize();
    double gt[6];
    firstDS->GetGeoTransform(gt);
    const char* projRef = firstDS->GetProjectionRef();
    GDALClose(firstDS);

    GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("GTiff");
    if (!driver) return false;

    GDALDataset* dstDS = driver->Create(outputFile.toStdString().c_str(),
                                         width, height, inputFiles.size(), GDT_Float32, nullptr);
    if (!dstDS) return false;

    dstDS->SetGeoTransform(gt);
    if (projRef && strlen(projRef) > 0) {
        dstDS->SetProjection(projRef);
    }

    bool ok = true;
    for (int i = 0; i < inputFiles.size(); ++i) {
        GDALDataset* srcDS = (GDALDataset*)GDALOpen(inputFiles[i].toStdString().c_str(), GA_ReadOnly);
        if (!srcDS) { ok = false; break; }

        if (srcDS->GetRasterXSize() != width || srcDS->GetRasterYSize() != height) {
            GDALClose(srcDS);
            ok = false;
            break;
        }

        GDALRasterBand* srcBand = srcDS->GetRasterBand(1);
        GDALRasterBand* dstBand = dstDS->GetRasterBand(i + 1);

        std::vector<float> buf(width * height);
        if (srcBand->RasterIO(GF_Read, 0, 0, width, height,
                              buf.data(), width, height, GDT_Float32, 0, 0) != CE_None) {
            ok = false;
            GDALClose(srcDS);
            break;
        }
        if (dstBand->RasterIO(GF_Write, 0, 0, width, height,
                              buf.data(), width, height, GDT_Float32, 0, 0) != CE_None) {
            ok = false;
            GDALClose(srcDS);
            break;
        }

        int hasNoData = 0;
        double nodata = srcBand->GetNoDataValue(&hasNoData);
        if (hasNoData) dstBand->SetNoDataValue(nodata);

        GDALClose(srcDS);
    }

    GDALClose(dstDS);
    return ok;
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

    double cornersX[4] = {gt[0], gt[0] + width * gt[1], gt[0], gt[0] + width * gt[1]};
    double cornersY[4] = {gt[3], gt[3], gt[3] + height * gt[5], gt[3] + height * gt[5]};

    OGRSpatialReference srcSRS, wgs84SRS;
    wgs84SRS.importFromEPSG(4326);
    wgs84SRS.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
    const char* projStr = dataset->GetProjectionRef();
    bool geoOk = false;
    if (projStr && strlen(projStr) > 0) {
        srcSRS.importFromWkt(projStr);
        if (!srcSRS.IsSame(&wgs84SRS)) {
            OGRCoordinateTransformation* ct =
                OGRCreateCoordinateTransformation(&srcSRS, &wgs84SRS);
            if (ct) {
                geoOk = ct->Transform(4, cornersX, cornersY);
                OGRCoordinateTransformation::DestroyCT(ct);
            }
        } else {
            geoOk = true;
        }
    }
    if (!geoOk) {
        double ulX = gt[0], ulY = gt[3];
        double lrX = gt[0] + width * gt[1], lrY = gt[3] + height * gt[5];

        // 值超出经纬度范围 → 可能是投影坐标系（如 3857），尝试变换
        if (std::abs(ulX) > 180 || std::abs(lrX) > 180 ||
            std::abs(ulY) > 90  || std::abs(lrY) > 90) {
            OGRSpatialReference mercSRS;
            mercSRS.importFromEPSG(3857);
            mercSRS.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
            OGRCoordinateTransformation* ct =
                OGRCreateCoordinateTransformation(&mercSRS, &wgs84SRS);
            if (ct) {
                cornersX[0] = ulX; cornersY[0] = ulY;
                cornersX[1] = lrX; cornersY[1] = ulY;
                cornersX[2] = ulX; cornersY[2] = lrY;
                cornersX[3] = lrX; cornersY[3] = lrY;
                ct->Transform(4, cornersX, cornersY);
                OGRCoordinateTransformation::DestroyCT(ct);
            }
        } else {
            cornersX[0] = ulX; cornersY[0] = ulY;
            cornersX[1] = lrX; cornersY[1] = ulY;
            cornersX[2] = ulX; cornersY[2] = lrY;
            cornersX[3] = lrX; cornersY[3] = lrY;
        }
    }

    double west  = *std::min_element(cornersX, cornersX + 4);
    double east  = *std::max_element(cornersX, cornersX + 4);
    double south = *std::min_element(cornersY, cornersY + 4);
    double north = *std::max_element(cornersY, cornersY + 4);

    QString js = "playAnimation([";

    for (int b = 0; b < numBands; ++b) {
        GDALRasterBand* band = dataset->GetRasterBand(b + 1);
        if (!band) continue;

        std::vector<double> data(width * height);
        if (band->RasterIO(GF_Read, 0, 0, width, height,
                           data.data(), width, height, GDT_Float64, 0, 0) != CE_None) continue;

        double nodata = band->GetNoDataValue();
        bool hasNodata = !std::isnan(nodata);

        double minVal = data[0], maxVal = data[0];
        for (double v : data) {
            if (std::isnan(v) || std::isinf(v)) continue;
            if (hasNodata && v == nodata) continue;
            if (v < minVal) minVal = v;
            if (v > maxVal) maxVal = v;
        }

        QImage frame(width, height, QImage::Format_RGB32);
        frame.fill(Qt::white);

        double range = maxVal - minVal;
        int colorCount = selectedMap.colors.size();
        for (int i = 0; i < width * height; ++i) {
            double val = data[i];
            if (std::isnan(val) || std::isinf(val)) continue;
            if (hasNodata && val == nodata) continue;
            double t = range > 0 ? (val - minVal) / range : 0.5;
            t = std::max(0.0, std::min(1.0, t));
            int ci = static_cast<int>(t * (colorCount - 1));
            auto [r, g, b_, a] = selectedMap.colors[ci];
            frame.setPixelColor(i % width, i / width, QColor::fromRgbF(r, g, b_));
        }

        QByteArray ba;
        QBuffer buf(&ba);
        buf.open(QIODevice::WriteOnly);
        frame.save(&buf, "PNG");

        if (b > 0) js += ",";
        js += "\"data:image/png;base64," + QString::fromLatin1(ba.toBase64()) + "\"";
    }

    GDALClose(dataset);

    js += QString("],[[%1,%2],[%3,%4]],10);")
        .arg(south, 0, 'f', 6).arg(west, 0, 'f', 6)
        .arg(north, 0, 'f', 6).arg(east, 0, 'f', 6);

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

void MainWindow::addTiffLayer(const QString& base64Image, const QString& bounds)
{
    QString js = QString("addTiffImage('%1', %2);").arg(base64Image).arg(bounds);
    webView->page()->runJavaScript(js);
}

void MainWindow::addShapefileLayer(const QString& geojson, const QString& color)
{
    QString js = QString("addShapefileLayer('%1');").arg(geojson);
    webView->page()->runJavaScript(js);
}
