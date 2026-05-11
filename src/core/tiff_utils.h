#ifndef TIFF_UTILS_H
#define TIFF_UTILS_H

#include <QString>
#include <QStringList>
#include <string>

namespace TiffUtils {
    struct LatLonBounds {
        double west, south, east, north;
    };

    bool createMultiBandTiff(const QStringList& inputFiles, const QString& outputFile);
    LatLonBounds computeWGS84Bounds(const double gt[6], int width, int height, const char* projRef);
    bool runGdalwarp(const QString& input, const QString& output,
                     double resolution, const std::string& method,
                     QString& errorOutput);
}

#endif
