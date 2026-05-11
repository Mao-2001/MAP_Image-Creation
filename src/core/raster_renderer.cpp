#include "raster_renderer.h"
#include <QImage>
#include <QColor>
#include <algorithm>
#include <cmath>

QImage RasterRenderer::renderFrame(const std::vector<double>& data, int width, int height,
                                    const ColorMap& colorMap, double minVal, double maxVal,
                                    double nodata)
{
    QImage frame(width, height, QImage::Format_RGB32);
    frame.fill(Qt::white);

    bool hasNodata = !std::isnan(nodata);
    double range = maxVal - minVal;
    int colorCount = colorMap.colors.size();

    for (int i = 0; i < width * height; ++i) {
        double val = data[i];
        if (std::isnan(val) || std::isinf(val)) continue;
        if (hasNodata && val == nodata) continue;

        double t = range > 0 ? (val - minVal) / range : 0.5;
        t = std::max(0.0, std::min(1.0, t));
        int colorIdx = static_cast<int>(t * (colorCount - 1));
        auto [r, g, b, a] = colorMap.colors[colorIdx];
        frame.setPixelColor(i % width, i / width, QColor::fromRgbF(r, g, b));
    }
    return frame;
}

QImage RasterRenderer::renderFrame(const std::vector<double>& data, int width, int height,
                                    const ColorMap& colorMap)
{
    if (data.empty()) return QImage();

    double minVal = data[0], maxVal = data[0];
    for (double v : data) {
        if (std::isnan(v) || std::isinf(v)) continue;
        if (v < minVal) minVal = v;
        if (v > maxVal) maxVal = v;
    }
    return renderFrame(data, width, height, colorMap, minVal, maxVal, NAN);
}

RasterRenderer::ColorMap RasterRenderer::getDefaultColorMap()
{
    ColorMap cm;
    cm.name = "Default";
    cm.colors = {
        {0.0, 0.0, 1.0, 1.0},
        {0.0, 1.0, 1.0, 1.0},
        {0.0, 1.0, 0.0, 1.0},
        {1.0, 1.0, 0.0, 1.0},
        {1.0, 0.0, 0.0, 1.0}
    };
    return cm;
}

RasterRenderer::ColorMap RasterRenderer::getTerrainColorMap()
{
    ColorMap cm;
    cm.name = "Terrain";
    cm.colors = {
        {0.0, 0.2, 0.5, 1.0},
        {0.0, 0.5, 1.0, 1.0},
        {0.8, 0.7, 0.6, 1.0},
        {0.2, 0.6, 0.2, 1.0},
        {0.1, 0.4, 0.1, 1.0},
        {0.5, 0.5, 0.5, 1.0},
        {1.0, 1.0, 1.0, 1.0}
    };
    return cm;
}

RasterRenderer::ColorMap RasterRenderer::getCoolWarmColorMap()
{
    ColorMap cm;
    cm.name = "Cool-Warm";
    cm.colors = {
        {0.0, 0.0, 1.0, 1.0},
        {1.0, 1.0, 1.0, 1.0},
        {1.0, 0.0, 0.0, 1.0}
    };
    return cm;
}

RasterRenderer::ColorMap RasterRenderer::getViridisColorMap()
{
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

RasterRenderer::ColorMap RasterRenderer::getPlasmaColorMap()
{
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

RasterRenderer::ColorMap RasterRenderer::getSpectralColorMap()
{
    ColorMap cm;
    cm.name = "Spectral";
    cm.colors = {
        {0.619607, 0.003921, 0.258823, 1.0},
        {0.894117, 0.101960, 0.109803, 1.0},
        {0.992156, 0.611764, 0.157647, 1.0},
        {0.996078, 0.878431, 0.364705, 1.0},
        {0.521568, 0.768627, 0.031372, 1.0},
        {0.031372, 0.568627, 0.549019, 1.0},
        {0.031372, 0.286274, 0.549019, 1.0},
        {0.309803, 0.160784, 0.478431, 1.0}
    };
    return cm;
}

RasterRenderer::ColorMap RasterRenderer::getRdBuColorMap()
{
    ColorMap cm;
    cm.name = "RdBu";
    cm.colors = {
        {0.403921, 0.000000, 0.121568, 1.0},
        {0.698039, 0.094117, 0.168627, 1.0},
        {0.939216, 0.400000, 0.345098, 1.0},
        {0.992156, 0.752941, 0.745098, 1.0},
        {0.968627, 0.968627, 0.984313, 1.0},
        {0.776470, 0.858823, 0.937254, 1.0},
        {0.415686, 0.639215, 0.803921, 1.0},
        {0.192156, 0.211764, 0.584313, 1.0},
        {0.019607, 0.047058, 0.321568, 1.0}
    };
    return cm;
}
