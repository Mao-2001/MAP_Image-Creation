#ifndef GIF_EXPORTER_H
#define GIF_EXPORTER_H

#include <string>
#include <memory>
#include "raster_renderer.h"

class GifExporter {
public:
    GifExporter();
    ~GifExporter();

    bool exportToGif(const std::string& multiBandTiffPath,
                     const RasterRenderer::ColorMap& colorMap,
                     const std::string& outputGifPath,
                     int frameDelay = 100);

    std::string getLastError() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

#endif // GIF_EXPORTER_H
