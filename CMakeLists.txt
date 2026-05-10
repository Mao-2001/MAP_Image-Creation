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
    main.cpp
    mainwindow.cpp
    keydialog.cpp
    geo_tiff_loader.cpp
    shapefile_loader.cpp
    colormapdialog.cpp
    resampledialog.cpp
    gif_exporter.cpp
)

set(HEADERS
    mainwindow.h
    keydialog.h
    geo_tiff_loader.h
    shapefile_loader.h
    colormapdialog.h
    resampledialog.h
    gif_exporter.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME}
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::WebChannel
    GDAL::GDAL
)

file(COPY ${CMAKE_SOURCE_DIR}/map.html DESTINATION ${CMAKE_BINARY_DIR})

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(FILES ${CMAKE_SOURCE_DIR}/map.html DESTINATION bin)
