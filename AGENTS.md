# AGENTS.md

## Project: MAP_v1.3 - 地理信息系统 (GIS)

A desktop GIS application built with Qt6 and Leaflet.js, integrating China's Tianditu (天地图) map service with powerful GeoTIFF and Shapefile visualization capabilities.

## Tech Stack

- **Backend**: C++17, Qt6 (Widgets, WebEngineWidgets, WebChannel)
- **Frontend**: Leaflet.js (v1.9.4), Tianditu WMTS API
- **GIS Library**: GDAL 3.12.3 (for GeoTIFF and Shapefile handling)
- **Build**: CMake (>=3.16)

## Key Features

### 1. Base Map Display
- Tianditu vector basemap with annotation overlay (EPSG:3857 Web Mercator)
- Map centered on Xi'an (34.34, 108.94) as China's geographic center
- Custom right-click context menu (only "Reload")
- Interactive zoom and pan controls

### 2. GeoTIFF Loading & Visualization
- Load GeoTIFF files via **File → Open TIFF**
- **Color Map Selection**: 7 scientific color schemes available:
  - **Default**: Blue→Cyan→Green→Yellow→Red
  - **Terrain**: Blue→Brown→Green→Gray→White (for DEM/elevation data)
  - **Cool-Warm**: Blue→White→Red (for positive/negative values)
  - **Viridis**: Modern scientific colormap (colorblind-friendly)
  - **Plasma**: Modern scientific colormap (purple→yellow)
  - **Spectral**: Rainbow spectrum (for remote sensing)
  - **RdBu**: Red→White→Blue (for meteorology/anomalies)
- Real-time preview in color map selection dialog
- Automatic min/max value calculation with NoData handling
- CRS-aware bounds transformation (supports EPSG:4326 and EPSG:3857)
- Debug PNG output to `/tmp/debug_tiff.png`

### 3. Shapefile Loading & Visualization
- Load Shapefile files via **File → Open SHP**
- Color picker dialog for custom display colors
- Uses GDAL's OGR to convert SHP to GeoJSON
- Supports Polygon, LineString, and Point geometries

### 4. Single File Downsampling
- **Tool → 单文件降采样**: Resample a single TIFF to target resolution
  - User-configurable resolution and resampling method (bilinear/nearest/cubic/average)
  - Output in EPSG:3857 (Web Mercator) for pixel-perfect alignment with Leaflet
  - Auto-generated output filename: `original_{resolution}.tif`

### 5. Batch TIFF Import & Downsampling
- **File → Open TIFFs (批量导入)**: Select multiple TIFF files at once
- **Tool → 批量降采样**: Resample all imported TIFFs to target resolution via `gdalwarp -tr`
  - User-configurable resolution (e.g., 3000m/pixel)
  - Multiple resampling methods: bilinear, nearest, cubic, average
  - Output as multi-band single TIFF in EPSG:3857 for animation

### 6. Multi-band TIFF Playback
- **File → 打开多波段 TIFF**: Open multi-band TIFF and display as animation on map
  - 10fps playback using JavaScript `setInterval` + `L.imageOverlay.setUrl()`
  - CRS detection with automatic 3857→4326 bounds transformation
  - NoData regions rendered as white

### 7. Export Current View
- **Tool → 导出图片 (PNG/GIF)**: Export current display based on state:
  - After loading single TIFF → export as **PNG** with current colormap
  - After playing animation → export as **GIF** with current colormap
  - NoData regions rendered as white
  - Uses ImageMagick `magick` (or `convert`) for GIF generation

### 8. API Key Management
- First launch prompts for Tianditu API key via `KeyDialog`
- Key persisted via `QSettings` (no re-entry needed on subsequent launches)

## Build Instructions

### Prerequisites
- Qt6 (with Widgets, WebEngineWidgets, WebChannel components)
- GDAL >= 3.0 with development headers
- CMake >= 3.16
- C++17 compatible compiler
- ImageMagick (for GIF export)

### Build Steps (Linux)
```bash
cd /home/mao/Documents/Map
rm -rf build
mkdir -p build && cd build
cmake ..
make
```

### Build (Windows)
```bash
# Prerequisites: Visual Studio 2022, Qt 6.8 (msvc2022_64), OSGeo4W GDAL, ImageMagick
# Edit paths in package_windows.bat first, then run:
package_windows.bat
```

### Run
```bash
cd /home/mao/Documents/Map/build
./MAP_v1.0
```

## Usage Guide

### Getting Started
1. Run `./MAP_v1.0`
2. On first launch, enter your Tianditu API key (get from http://lbs.tianditu.gov.cn/)
3. The map will load centered on Xi'an, China

### Loading GeoTIFF
1. **File → Open TIFF** (or `Ctrl+T`)
2. Select a GeoTIFF file (`.tif`, `.tiff`)
3. Choose a color map from the dialog
4. The TIFF overlay appears on the map

### Loading Shapefile
1. **File → Open SHP** (or `Ctrl+S`)
2. Select a Shapefile (`.shp`)
3. Choose display color from picker
4. The vector layer appears on the map

### Single Downsample
1. **Tool → 单文件降采样**: Select TIFF → set resolution/method → save EPSG:3857 output
2. Open the downsampled file for pixel-perfect alignment with basemap

### Batch Import and Animation
1. **File → Open TIFFs (批量导入)**: Select multiple TIFF files
2. **Tool → 批量降采样**: Set target resolution → resamples all TIFFs via `gdalwarp` → saves as multi-band TIFF
3. **File → 打开多波段 TIFF**: Open multi-band TIFF → plays 10fps animation on map
4. **Tool → 导出图片**: Export current animation as GIF or current TIFF as PNG

## File Structure

| File | Description |
|------|-------------|
| `main.cpp` | Application entry point |
| `mainwindow.h/cpp` | Main window, menu bar, all business logic |
| `keydialog.h/cpp` | Dialog for entering Tianditu API key |
| `geo_tiff_loader.h/cpp` | GeoTIFF reading, color mapping, min/max, CRS-aware bounds |
| `shapefile_loader.h/cpp` | Shapefile reading, GeoJSON conversion |
| `colormapdialog.h/cpp` | Color map selection dialog with preview |
| `resampledialog.h/cpp` | Resolution/method input dialog for downsampling |
| `gif_exporter.h/cpp` | Multi-band TIFF reading, color rendering, GIF generation |
| `map.html` | Leaflet.js map with QWebChannel bridge, animation, context menu |
| `CMakeLists.txt` | CMake build configuration |
| `package_windows.bat` | Windows packaging script |

## Dependencies

### System Libraries (Auto-detected)
- **Qt6**: `/usr/lib/cmake/Qt6/`
- **GDAL**: `/usr/lib/cmake/gdal/GDALConfig.cmake` (v3.12.3)

### Header Files
- Qt6: `/usr/include/qt6/`
- GDAL: `/usr/include/gdal.h`, `/usr/include/gdal/`

## Technical Notes

### GeoTIFF Processing
- Uses GDAL's `RasterIO` with `GDT_Float64` for type-safe reading
- Automatic NoData value detection and handling
- Calculates actual min/max from data (not metadata)
- Supports Int16, Float32, Float64, and other raster types

### Coordinate System & Projection
- Basemap: Tianditu `vec_w`/`cva_w` with `TILEMATRIXSET=w` (EPSG:3857 Web Mercator)
- Source TIFFs: typically EPSG:4326 (WGS84 geographic)
- `getWGS84Bounds()` transforms TIFF corners to EPSG:4326 lat/lon for Leaflet
- GDAL 3.x axis order issue handled via `SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER)`
- Missing CRS detection: if geoTransform values exceed valid lat/lon range, assume EPSG:3857

### Color Mapping Algorithm
```
t = (value - minValue) / (maxValue - minValue)
t = clamp(t, 0, 1)
colorIndex = t * (colorCount - 1)
color = colormap[colorIndex]
```

### JavaScript Communication
- Uses `QWebEngineView::runJavaScript()` to call Leaflet functions
- Base64 encoding for image data transmission
- QWebChannel bridge for C++ ↔ JS communication (context menu reload, etc.)

### Downsampling
- Uses `gdalwarp -tr X X -r method -t_srs EPSG:3857 -ot Float32`
- Output resolution in meters (EPSG:3857 units)
- For single file: no meter→degree conversion needed

### GIF Export
- Each TIFF band → Float64 data → colormap → RGB32 QImage → PNG frame
- ImageMagick `magick -delay D -loop 0 frames... output.gif`
- NoData regions rendered as white (QImage::Format_RGB32, fill(Qt::white))

## Known Issues

1. **Large TIFF files**: Memory usage increases with image size
2. **L.imageOverlay distortion**: 4326-grid TIFF on 3857 map shows slight pixel stretching (use downsampled 3857 file for perfect alignment)
3. **WebGL rendering**: Leaflet image overlay may have rendering quirks in some Qt WebEngine versions

## Testing Data

Example data paths:
```
# NPP GeoTIFF files
/home/mao/Documents/tif/drive-download-20260507T030938Z-3-001/NPP_2001_China.tif
/home/mao/Documents/tif/drive-download-20260507T030938Z-3-001/NPP_2021_China.tif

# Shapefile (China boundary)
/home/mao/Documents/tif/GEE_exports/Export_China_mian_boundary.shp
```

## Version History

- **v1.0**: Initial release with GeoTIFF/SHP support and color mapping
- **v1.1**: Added batch TIFF import, GDAL-based downsampling, multi-band TIFF creation, and GIF animation export via ImageMagick. Removed SHP clipping feature.
- **v1.2**: Added single file downsampling with configurable resolution. ColorMapDialog simplified (removed Heat/Grayscale). All downsampling outputs EPSG:3857.
- **v1.3**: Fixed GDAL 3.x axis order bug (SetAxisMappingStrategy). Added CRS auto-detection fallback for files without projection metadata. Map right-click context menu → only Reload. Export adapts to current state (PNG for single TIFF, GIF for animation). Added QWebChannel bridge for C++↔JS communication.
