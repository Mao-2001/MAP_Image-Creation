#include "geo_tiff_loader.h"
#include <gdal_priv.h>
#include <cpl_conv.h>
#include <algorithm>
#include <vector>
#include <sstream>
#include <iomanip>
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

        GDALDataType actualType = band->GetRasterDataType();
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

std::vector<uint8_t> GeoTIFFLoader::renderToRGBA(const ColorMap& colorMap) const {
    std::vector<uint8_t> rgba;
    if (pImpl->rasterData.empty()) return rgba;

    int size = pImpl->info.width * pImpl->info.height;
    rgba.resize(size * 4, 0);

    double minV = pImpl->info.minValue;
    double maxV = pImpl->info.maxValue;
    double range = maxV - minV;
    double nodata = pImpl->nodataValue;
    bool hasNodata = !std::isnan(nodata) && !std::isinf(nodata);

    for (int i = 0; i < size; ++i) {
        double val = pImpl->rasterData[i];

        if (std::isnan(val) || std::isinf(val)) continue;
        if (hasNodata && val == nodata) continue;

        double t = range > 0 ? (val - minV) / range : 0.5;
        t = std::max(0.0, std::min(1.0, t));

        int colorIdx = static_cast<int>(t * (colorMap.colors.size() - 1));
        auto [r, g, b, a] = colorMap.colors[colorIdx];

        rgba[i*4] = static_cast<uint8_t>(r * 255);
        rgba[i*4+1] = static_cast<uint8_t>(g * 255);
        rgba[i*4+2] = static_cast<uint8_t>(b * 255);
        rgba[i*4+3] = static_cast<uint8_t>(a * 255);
    }
    return rgba;
}

std::string GeoTIFFLoader::toBase64Image() const {
    auto rgba = renderToRGBA();
    return "";
}

GeoTIFFLoader::ColorMap GeoTIFFLoader::getDefaultColorMap() {
    ColorMap cm;
    cm.name = "Default";
    cm.colors = {
        {0.0, 0.0, 1.0, 1.0},   // Blue
        {0.0, 1.0, 1.0, 1.0},   // Cyan
        {0.0, 1.0, 0.0, 1.0},   // Green
        {1.0, 1.0, 0.0, 1.0},   // Yellow
        {1.0, 0.0, 0.0, 1.0}    // Red
    };
    return cm;
}

GeoTIFFLoader::ColorMap GeoTIFFLoader::getTerrainColorMap() {
    ColorMap cm;
    cm.name = "Terrain";
    cm.colors = {
        {0.0, 0.2, 0.5, 1.0},   // Deep water
        {0.0, 0.5, 1.0, 1.0},   // Shallow water
        {0.8, 0.7, 0.6, 1.0},   // Sand/beach
        {0.2, 0.6, 0.2, 1.0},   // Low vegetation
        {0.1, 0.4, 0.1, 1.0},   // Forest
        {0.5, 0.5, 0.5, 1.0},   // Rock
        {1.0, 1.0, 1.0, 1.0}    // Snow
    };
    return cm;
}

GeoTIFFLoader::ColorMap GeoTIFFLoader::getHeatColorMap() {
    ColorMap cm;
    cm.name = "Heat";
    cm.colors = {
        {0.0, 0.0, 0.0, 0.0},   // Transparent
        {0.2, 0.0, 0.8, 0.8},   // Purple
        {0.4, 0.0, 1.0, 0.9},   // Blue
        {0.6, 1.0, 0.0, 1.0},   // Green
        {0.8, 1.0, 0.0, 1.0},   // Yellow
        {1.0, 0.5, 0.0, 1.0},   // Orange
        {1.0, 0.0, 0.0, 1.0}    // Red
    };
    return cm;
}

GeoTIFFLoader::ColorMap GeoTIFFLoader::getGrayscaleColorMap() {
    ColorMap cm;
    cm.name = "Grayscale";
    cm.colors = {
        {0.0, 0.0, 0.0, 1.0},   // Black
        {1.0, 1.0, 1.0, 1.0}    // White
    };
    return cm;
}

GeoTIFFLoader::ColorMap GeoTIFFLoader::getCoolWarmColorMap() {
    ColorMap cm;
    cm.name = "Cool-Warm";
    cm.colors = {
        {0.0, 0.0, 1.0, 1.0},   // Cool (Blue)
        {1.0, 1.0, 1.0, 1.0},   // White
        {1.0, 0.0, 0.0, 1.0}    // Warm (Red)
    };
    return cm;
}

GeoTIFFLoader::ColorMap GeoTIFFLoader::getViridisColorMap() {
    ColorMap cm;
    cm.name = "Viridis";
    cm.colors = {
        {0.267004, 0.004874, 0.329415, 1.0},
        {0.282679, 0.140651, 0.457781, 1.0},
        {0.253935, 0.265254, 0.529983, 1.0},
        {0.206756, 0.371758, 0.553117, 1.0},
        {0.163625, 0.471133, 0.558148, 1.0},
        {0.127568, 0.566949, 0.550556, 1.0},
        {0.134692, 0.658636, 0.517649, 1.0},
        {0.267398, 0.748886, 0.440476, 1.0},
        {0.440997, 0.831159, 0.337293, 1.0},
        {0.644024, 0.878438, 0.206551, 1.0},
        {0.847943, 0.873719, 0.150204, 1.0},
        {0.993248, 0.906157, 0.143936, 1.0}
    };
    return cm;
}

GeoTIFFLoader::ColorMap GeoTIFFLoader::getPlasmaColorMap() {
    ColorMap cm;
    cm.name = "Plasma";
    cm.colors = {
        {0.050383, 0.029803, 0.527975, 1.0},
        {0.220965, 0.103014, 0.616513, 1.0},
        {0.359831, 0.148154, 0.683126, 1.0},
        {0.504347, 0.156547, 0.732154, 1.0},
        {0.649924, 0.133640, 0.743275, 1.0},
        {0.763548, 0.086945, 0.724543, 1.0},
        {0.855891, 0.146131, 0.676595, 1.0},
        {0.916574, 0.242242, 0.572138, 1.0},
        {0.950106, 0.375939, 0.428084, 1.0},
        {0.954117, 0.521690, 0.272651, 1.0},
        {0.940015, 0.675995, 0.138613, 1.0},
        {0.991438, 0.856553, 0.126800, 1.0}
    };
    return cm;
}

GeoTIFFLoader::ColorMap GeoTIFFLoader::getSpectralColorMap() {
    ColorMap cm;
    cm.name = "Spectral";
    cm.colors = {
        {0.619607, 0.003921, 0.258823, 1.0},  // Red
        {0.894117, 0.101960, 0.109803, 1.0},  // Orange-Red
        {0.992156, 0.611764, 0.157647, 1.0},  // Orange
        {0.996078, 0.878431, 0.364705, 1.0},  // Yellow
        {0.521568, 0.768627, 0.031372, 1.0},  // Green
        {0.031372, 0.568627, 0.549019, 1.0},  // Teal
        {0.031372, 0.286274, 0.549019, 1.0},  // Blue
        {0.309803, 0.160784, 0.478431, 1.0}   // Purple
    };
    return cm;
}

GeoTIFFLoader::ColorMap GeoTIFFLoader::getRdBuColorMap() {
    ColorMap cm;
    cm.name = "RdBu";
    cm.colors = {
        {0.403921, 0.000000, 0.121568, 1.0},  // Dark Red
        {0.698039, 0.094117, 0.168627, 1.0},  // Red
        {0.939216, 0.400000, 0.345098, 1.0},  // Light Red
        {0.992156, 0.752941, 0.745098, 1.0},  // Pink-White
        {0.968627, 0.968627, 0.984313, 1.0},  // White
        {0.776470, 0.858823, 0.937254, 1.0},  // Light Blue
        {0.415686, 0.639215, 0.803921, 1.0},  // Blue
        {0.192156, 0.211764, 0.584313, 1.0},  // Dark Blue
        {0.019607, 0.047058, 0.321568, 1.0}   // Very Dark Blue
    };
    return cm;
}
