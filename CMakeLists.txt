cmake_minimum_required(VERSION 3.16)
project(MAP_v1.3 VERSION 1.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets WebEngineWidgets WebChannel)
find_package(GDAL REQUIRED)

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/dialogs/keydialog.cpp
    src/dialogs/colormapdialog.cpp
    src/dialogs/resampledialog.cpp
    src/core/geo_tiff_loader.cpp
    src/core/shapefile_loader.cpp
    src/core/raster_renderer.cpp
    src/core/tiff_utils.cpp
    src/export/gif_exporter.cpp
)

set(HEADERS
    src/mainwindow.h
    src/dialogs/keydialog.h
    src/dialogs/colormapdialog.h
    src/dialogs/resampledialog.h
    src/core/geo_tiff_loader.h
    src/core/shapefile_loader.h
    src/core/raster_renderer.h
    src/core/tiff_utils.h
    src/export/gif_exporter.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wextra>
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/export
)

target_link_libraries(${PROJECT_NAME}
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::WebChannel
    GDAL::GDAL
)

file(COPY ${CMAKE_SOURCE_DIR}/map.html DESTINATION ${CMAKE_BINARY_DIR})

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(FILES ${CMAKE_SOURCE_DIR}/map.html DESTINATION bin)
