#ifndef RASTER_RENDERER_H
#define RASTER_RENDERER_H

#include <string>
#include <vector>
#include <tuple>

class QImage;

class RasterRenderer {
public:
    struct ColorMap {
        std::string name;
        std::vector<std::tuple<double, double, double, double>> colors;
    };

    static QImage renderFrame(const std::vector<double>& data, int width, int height,
                              const ColorMap& colorMap, double minVal, double maxVal,
                              double nodata);

    static QImage renderFrame(const std::vector<double>& data, int width, int height,
                              const ColorMap& colorMap);

    static ColorMap getDefaultColorMap();
    static ColorMap getTerrainColorMap();
    static ColorMap getCoolWarmColorMap();
    static ColorMap getViridisColorMap();
    static ColorMap getPlasmaColorMap();
    static ColorMap getSpectralColorMap();
    static ColorMap getRdBuColorMap();
};

#endif
