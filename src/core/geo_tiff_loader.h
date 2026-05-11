#ifndef GEO_TIFF_LOADER_H
#define GEO_TIFF_LOADER_H

#include <string>
#include <vector>
#include <memory>
#include <QImage>
#include "raster_renderer.h"

class GeoTIFFLoader {
public:
    struct GeoInfo {
        double geoTransform[6];
        int width;
        int height;
        double minValue;
        double maxValue;
        std::string projection;
    };

    struct LatLonBounds {
        double west, south, east, north;
    };

    GeoTIFFLoader();
    ~GeoTIFFLoader();

    bool load(const std::string& filename);
    GeoInfo getGeoInfo() const;
    LatLonBounds getWGS84Bounds() const;
    QImage renderToQImage(const RasterRenderer::ColorMap& colorMap) const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

#endif // GEO_TIFF_LOADER_H
