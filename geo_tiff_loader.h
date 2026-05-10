#ifndef GEO_TIFF_LOADER_H
#define GEO_TIFF_LOADER_H

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

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

    struct ColorMap {
        std::string name;
        std::vector<std::tuple<double, double, double, double>> colors; // r,g,b,a
    };

    GeoTIFFLoader();
    ~GeoTIFFLoader();

    bool load(const std::string& filename);
    GeoInfo getGeoInfo() const;
    LatLonBounds getWGS84Bounds() const;
    std::vector<uint8_t> renderToRGBA(const ColorMap& colorMap = getDefaultColorMap()) const;
    std::string toBase64Image() const;
    static ColorMap getDefaultColorMap();
    static ColorMap getTerrainColorMap();
    static ColorMap getHeatColorMap();
    static ColorMap getGrayscaleColorMap();
    static ColorMap getCoolWarmColorMap();
    static ColorMap getViridisColorMap();
    static ColorMap getPlasmaColorMap();
    static ColorMap getSpectralColorMap();
    static ColorMap getRdBuColorMap();

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

#endif // GEO_TIFF_LOADER_H
