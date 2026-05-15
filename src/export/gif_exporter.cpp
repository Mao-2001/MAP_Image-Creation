#include "gif_exporter.h"
#include "raster_renderer.h"
#include <gdal_priv.h>
#include <QProcess>
#include <QImage>
#include <QTemporaryDir>
#include <QCoreApplication>
#include <QFile>
#include <cmath>
#include <vector>

class GifExporter::Impl {
public:
    std::string lastError;

    bool exportToGif(const std::string& multiBandTiffPath,
                     const RasterRenderer::ColorMap& colorMap,
                     const std::string& outputGifPath,
                     int frameDelay)
    {
        GDALAllRegister();

        GDALDataset* dataset = (GDALDataset*)GDALOpen(multiBandTiffPath.c_str(), GA_ReadOnly);
        if (!dataset) {
            lastError = "无法打开 TIFF 文件: " + multiBandTiffPath;
            return false;
        }

        int width = dataset->GetRasterXSize();
        int height = dataset->GetRasterYSize();
        int numBands = dataset->GetRasterCount();

        if (numBands < 2) {
            lastError = "需要至少 2 个波段，实际只有 " + std::to_string(numBands);
            GDALClose(dataset);
            return false;
        }

        QTemporaryDir tmpDir;
        if (!tmpDir.isValid()) {
            lastError = "无法创建临时目录";
            GDALClose(dataset);
            return false;
        }
        QString frameDir = tmpDir.path() + "/";

        QStringList frameFiles;

        for (int bandIdx = 0; bandIdx < numBands; ++bandIdx) {
            GDALRasterBand* band = dataset->GetRasterBand(bandIdx + 1);
            if (!band) {
                lastError = "无法获取波段 " + std::to_string(bandIdx + 1);
                continue;
            }

            std::vector<double> data(width * height);
            if (band->RasterIO(GF_Read, 0, 0, width, height,
                               data.data(), width, height, GDT_Float64, 0, 0) != CE_None) {
                lastError = "读取波段 " + std::to_string(bandIdx + 1) + " 失败";
                continue;
            }

            double nodata = band->GetNoDataValue();
            double minVal = data[0], maxVal = data[0];
            for (double v : data) {
                if (std::isnan(v) || std::isinf(v)) continue;
                if (!std::isnan(nodata) && v == nodata) continue;
                if (v < minVal) minVal = v;
                if (v > maxVal) maxVal = v;
            }

            QImage frame = RasterRenderer::renderFrame(data, width, height, colorMap,
                                                        minVal, maxVal, nodata);

            QString pngPath = frameDir + QString("frame_%1.png").arg(bandIdx, 3, 10, QChar('0'));
            if (frame.save(pngPath, "PNG")) {
                frameFiles << pngPath;
            } else {
                lastError = "保存 PNG 帧 " + std::to_string(bandIdx + 1) + " 失败";
            }
        }

        GDALClose(dataset);

        if (frameFiles.isEmpty()) {
            if (lastError.empty()) lastError = "没有生成任何有效帧";
            return false;
        }

        QProcess process;
        QStringList args;
        args << "-delay" << QString::number(frameDelay / 10)
             << "-loop" << "0";
        for (const QString& f : frameFiles) {
            args << f;
        }
        args << QString::fromStdString(outputGifPath);

        // 检查 ImageMagick 是否可用（优先程序目录，再查 PATH）
        QString appDir = QCoreApplication::applicationDirPath();
        auto findProgram = [&](const QString& name) -> QString {
            QStringList candidates = {appDir + "/" + name + ".exe", appDir + "/" + name, name};
            for (const QString& exe : candidates) {
                QProcess chk;
                chk.setProgram(exe);
                chk.setArguments({"--version"});
                chk.start();
                if (chk.waitForFinished(5000) && chk.exitCode() == 0)
                    return exe;
            }
            return {};
        };

        QString program = findProgram("magick");
        if (program.isEmpty()) program = findProgram("convert");

        if (program.isEmpty()) {
            lastError = "ImageMagick (magick/convert) 未找到，无法生成 GIF";
            for (const QString& f : frameFiles) QFile::remove(f);
            return false;
        }

        process.setProgram(program);
        process.setArguments(args);
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert("MAGICK_CODER_MODULE_PATH", appDir + "/modules/coders/");
        env.insert("MAGICK_CONFIGURE_PATH", appDir);
        process.setProcessEnvironment(env);
        process.start();
        if (!process.waitForStarted(10000)) {
            lastError = "无法启动 " + program.toStdString() + ": " + process.errorString().toStdString();
            for (const QString& f : frameFiles) QFile::remove(f);
            return false;
        }
        if (!process.waitForFinished(120000)) {
            process.kill();
            lastError = "ImageMagick 执行超时（>2分钟）";
            for (const QString& f : frameFiles) QFile::remove(f);
            return false;
        }

        for (const QString& f : frameFiles) {
            QFile::remove(f);
        }

        if (process.exitCode() != 0) {
            lastError = "ImageMagick (" + program.toStdString() + ") 失败:\n";
            lastError += process.readAllStandardError().toStdString();
            return false;
        }

        if (!QFile::exists(QString::fromStdString(outputGifPath))) {
            lastError = "GIF 文件未生成，请确保路径可写: " + outputGifPath;
            return false;
        }

        return true;
    }
};

GifExporter::GifExporter() : pImpl(std::make_unique<Impl>()) {}
GifExporter::~GifExporter() = default;

bool GifExporter::exportToGif(const std::string& multiBandTiffPath,
                               const RasterRenderer::ColorMap& colorMap,
                               const std::string& outputGifPath,
                               int frameDelay)
{
    return pImpl->exportToGif(multiBandTiffPath, colorMap, outputGifPath, frameDelay);
}

std::string GifExporter::getLastError() const {
    return pImpl->lastError;
}
