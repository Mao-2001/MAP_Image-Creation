#include "geo_tiff_loader.h"
#include "raster_renderer.h"
#include <gdal_priv.h>
#include <algorithm>
#include <vector>
#include <cmath>
#include <ogr_spatialref.h>

class GeoTIFFLoader::Impl {
public:
    GDALDataset* dataset = nullptr;
    GeoInfo info;
    std::vector<double> rasterData;
    double nodataValue;

    Impl() : nodataValue(0.0) {
        GDALAllRegister();
    }

    ~Impl() {
        if (dataset) GDALClose(dataset);
    }

    bool load(const std::string& filename) {
        dataset = (GDALDataset*)GDALOpen(filename.c_str(), GA_ReadOnly);
        if (!dataset) return false;

        info.width = dataset->GetRasterXSize();
        info.height = dataset->GetRasterYSize();
        dataset->GetGeoTransform(info.geoTransform);

        if (info.geoTransform[0] == 0 && info.geoTransform[1] == 1) {
            info.geoTransform[0] = -180;
            info.geoTransform[1] = 360.0 / info.width;
            info.geoTransform[3] = 90;
            info.geoTransform[5] = -180.0 / info.height;
        }

        info.projection = dataset->GetProjectionRef();

        GDALRasterBand* band = dataset->GetRasterBand(1);
        nodataValue = band->GetNoDataValue();

        rasterData.resize(info.width * info.height);

        if (band->RasterIO(GF_Read, 0, 0, info.width, info.height,
                          rasterData.data(), info.width, info.height, GDT_Float64, 0, 0) != CE_None) {
            return false;
        }

        info.minValue = rasterData[0];
        info.maxValue = rasterData[0];
        for (double v : rasterData) {
            if (std::isnan(v) || std::isinf(v)) continue;
            if (v != nodataValue) {
                if (v < info.minValue) info.minValue = v;
                if (v > info.maxValue) info.maxValue = v;
            }
        }

        return true;
    }

    void computeBoundsInWGS84(double& west, double& south, double& east, double& north) const {
        double ulX = info.geoTransform[0];
        double ulY = info.geoTransform[3];
        double lrX = ulX + info.width * info.geoTransform[1];
        double lrY = ulY + info.height * info.geoTransform[5];

        double cornersX[4] = {ulX, lrX, ulX, lrX};
        double cornersY[4] = {ulY, ulY, lrY, lrY};

        OGRSpatialReference srcSRS, wgs84SRS;
        wgs84SRS.importFromEPSG(4326);
        wgs84SRS.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);

        bool transformOk = false;
        if (!info.projection.empty()) {
            srcSRS.importFromWkt(info.projection.c_str());
            if (!srcSRS.IsSame(&wgs84SRS)) {
                OGRCoordinateTransformation* ct =
                    OGRCreateCoordinateTransformation(&srcSRS, &wgs84SRS);
                if (ct) {
                    transformOk = ct->Transform(4, cornersX, cornersY);
                    OGRCoordinateTransformation::DestroyCT(ct);
                }
            } else {
                transformOk = true;
            }
        }

        if (!transformOk) {
            if (std::abs(ulX) > 180 || std::abs(lrX) > 180 ||
                std::abs(ulY) > 90  || std::abs(lrY) > 90) {
                OGRSpatialReference mercSRS;
                mercSRS.importFromEPSG(3857);
                mercSRS.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
                OGRCoordinateTransformation* ct =
                    OGRCreateCoordinateTransformation(&mercSRS, &wgs84SRS);
                if (ct) {
                    ct->Transform(4, cornersX, cornersY);
                    OGRCoordinateTransformation::DestroyCT(ct);
                    west = *std::min_element(cornersX, cornersX + 4);
                    east = *std::max_element(cornersX, cornersX + 4);
                    south = *std::min_element(cornersY, cornersY + 4);
                    north = *std::max_element(cornersY, cornersY + 4);
                    return;
                }
            }
            west = ulX; east = lrX; south = lrY; north = ulY;
            return;
        }

        west = *std::min_element(cornersX, cornersX + 4);
        east = *std::max_element(cornersX, cornersX + 4);
        south = *std::min_element(cornersY, cornersY + 4);
        north = *std::max_element(cornersY, cornersY + 4);
    }
};

GeoTIFFLoader::GeoTIFFLoader() : pImpl(std::make_unique<Impl>()) {}
GeoTIFFLoader::~GeoTIFFLoader() = default;

bool GeoTIFFLoader::load(const std::string& filename) {
    return pImpl->load(filename);
}

GeoTIFFLoader::GeoInfo GeoTIFFLoader::getGeoInfo() const {
    return pImpl->info;
}

GeoTIFFLoader::LatLonBounds GeoTIFFLoader::getWGS84Bounds() const {
    LatLonBounds b;
    pImpl->computeBoundsInWGS84(b.west, b.south, b.east, b.north);
    return b;
}

QImage GeoTIFFLoader::renderToQImage(const RasterRenderer::ColorMap& colorMap) const {
    return RasterRenderer::renderFrame(pImpl->rasterData, pImpl->info.width, pImpl->info.height,
                                       colorMap, pImpl->info.minValue, pImpl->info.maxValue,
                                       pImpl->nodataValue);
}


