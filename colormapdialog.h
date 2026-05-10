#ifndef COLORMAPDIALOG_H
#define COLORMAPDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "geo_tiff_loader.h"

class ColorMapDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ColorMapDialog(QWidget *parent = nullptr);
    GeoTIFFLoader::ColorMap getSelectedColorMap() const;

private slots:
    void onItemDoubleClicked(QListWidgetItem *item);
    void onAccepted();

private:
    void populateColorMaps();
    void updatePreview(const GeoTIFFLoader::ColorMap& cm);

    QListWidget *listWidget;
    QLabel *previewLabel;
    QPushButton *okButton;
    QPushButton *cancelButton;
    QVector<GeoTIFFLoader::ColorMap> colorMaps;
    GeoTIFFLoader::ColorMap selectedMap;
};

#endif // COLORMAPDIALOG_H
