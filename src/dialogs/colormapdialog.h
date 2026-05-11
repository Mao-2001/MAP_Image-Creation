#ifndef COLORMAPDIALOG_H
#define COLORMAPDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "raster_renderer.h"

class ColorMapDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ColorMapDialog(QWidget *parent = nullptr);
    RasterRenderer::ColorMap getSelectedColorMap() const;

private slots:
    void onItemDoubleClicked(QListWidgetItem *item);
    void onAccepted();

private:
    void populateColorMaps();
    void updatePreview(const RasterRenderer::ColorMap& cm);

    QListWidget *listWidget;
    QLabel *previewLabel;
    QPushButton *okButton;
    QPushButton *cancelButton;
    QVector<RasterRenderer::ColorMap> colorMaps;
    RasterRenderer::ColorMap selectedMap;
};

#endif // COLORMAPDIALOG_H
