#include "tiff_utils.h"
#include <gdal_priv.h>
#include <ogr_spatialref.h>
#include <QProcess>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

bool TiffUtils::createMultiBandTiff(const QStringList& inputFiles, const QString& outputFile)
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

TiffUtils::LatLonBounds TiffUtils::computeWGS84Bounds(const double gt[6], int width, int height,
                                                       const char* projRef)
{
    double cornersX[4] = {gt[0], gt[0] + width * gt[1], gt[0], gt[0] + width * gt[1]};
    double cornersY[4] = {gt[3], gt[3], gt[3] + height * gt[5], gt[3] + height * gt[5]};

    OGRSpatialReference srcSRS, wgs84SRS;
    wgs84SRS.importFromEPSG(4326);
    wgs84SRS.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);

    bool geoOk = false;
    if (projRef && strlen(projRef) > 0) {
        srcSRS.importFromWkt(projRef);
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

    LatLonBounds b;
    b.west  = *std::min_element(cornersX, cornersX + 4);
    b.east  = *std::max_element(cornersX, cornersX + 4);
    b.south = *std::min_element(cornersY, cornersY + 4);
    b.north = *std::max_element(cornersY, cornersY + 4);
    return b;
}

bool TiffUtils::runGdalwarp(const QString& input, const QString& output,
                             double resolution, const std::string& method,
                             QString& errorOutput)
{
    QProcess process;
    QString resStr = QString::number(resolution);
    process.setProgram("gdalwarp");
    process.setArguments({"-tr", resStr, resStr,
                          "-r", QString::fromStdString(method),
                          "-t_srs", "EPSG:3857",
                          "-ot", "Float32", input, output});
    process.start();
    process.waitForFinished();

    if (process.exitCode() != 0) {
        errorOutput = QString::fromUtf8(process.readAllStandardError());
        return false;
    }
    return true;
}
