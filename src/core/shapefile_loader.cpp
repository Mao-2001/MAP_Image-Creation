#include "shapefile_loader.h"
#include <ogrsf_frmts.h>
#include <cpl_conv.h>
#include <sstream>

class ShapefileLoader::Impl {
public:
    GDALDataset* dataset = nullptr;
    OGRLayer* layer = nullptr;
    LayerInfo info;

    Impl() {
        GDALAllRegister();
    }

    ~Impl() {
        if (dataset) GDALClose(dataset);
    }

    bool load(const std::string& filename) {
        dataset = (GDALDataset*)GDALOpenEx(filename.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr);
        if (!dataset) return false;

        layer = dataset->GetLayer(0);
        if (!layer) return false;

        info.name = layer->GetName();
        OGRwkbGeometryType geomType = layer->GetGeomType();
        info.geometryType = OGRGeometryTypeToName(geomType);
        info.featureCount = layer->GetFeatureCount();

        return true;
    }


};

ShapefileLoader::ShapefileLoader() : pImpl(std::make_unique<Impl>()) {}
ShapefileLoader::~ShapefileLoader() = default;

bool ShapefileLoader::load(const std::string& filename) {
    return pImpl->load(filename);
}

ShapefileLoader::LayerInfo ShapefileLoader::getLayerInfo() const {
    return pImpl->info;
}

std::string ShapefileLoader::toGeoJSON(const std::string& color, double opacity) {
    if (!pImpl->layer) return "{}";

    // 重置读取指针
    pImpl->layer->ResetReading();

    std::stringstream ss;
    ss << "{\"type\":\"FeatureCollection\",\"features\":[";

    OGRFeature* feature;
    bool first = true;

    while ((feature = pImpl->layer->GetNextFeature()) != nullptr) {
        OGRGeometry* geom = feature->GetGeometryRef();
        if (geom) {
            char* geomJson = OGR_G_ExportToJson(geom);
            if (geomJson) {
                if (!first) ss << ",";
                first = false;
                ss << "{\"type\":\"Feature\",\"geometry\":"
                    << geomJson
                    << ",\"properties\":{\"color\":\"" << color << "\",\"opacity\":" << opacity << "}}";
                CPLFree(geomJson);
            }
        }
        OGRFeature::DestroyFeature(feature);
    }

    ss << "]}";
    return ss.str();
}
