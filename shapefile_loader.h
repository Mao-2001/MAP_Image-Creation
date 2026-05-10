#ifndef SHAPEFILE_LOADER_H
#define SHAPEFILE_LOADER_H

#include <string>
#include <vector>
#include <memory>

class ShapefileLoader {
public:
    struct Feature {
        std::string type; // "Polygon", "LineString", "Point"
        std::vector<std::vector<std::pair<double, double>>> coordinates; // 支持 MultiPolygon
        std::string properties;
    };

    struct LayerInfo {
        std::string name;
        std::string geometryType;
        int featureCount;
        std::vector<Feature> features;
    };

    ShapefileLoader();
    ~ShapefileLoader();

    bool load(const std::string& filename);
    LayerInfo getLayerInfo() const;
    std::string toGeoJSON(const std::string& color = "#3388ff", double opacity = 0.6);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

#endif // SHAPEFILE_LOADER_H
