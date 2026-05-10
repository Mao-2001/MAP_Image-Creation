#include "colormapdialog.h"
#include <QPainter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidgetItem>
#include <QMessageBox>

ColorMapDialog::ColorMapDialog(QWidget *parent)
    : QDialog(parent), selectedMap(GeoTIFFLoader::getDefaultColorMap())
{
    setWindowTitle("选择 TIFF 色条");
    setFixedSize(400, 500);

    listWidget = new QListWidget(this);
    previewLabel = new QLabel(this);
    previewLabel->setFixedSize(360, 40);
    previewLabel->setStyleSheet("border: 1px solid #ccc;");

    okButton = new QPushButton("确定", this);
    cancelButton = new QPushButton("取消", this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("预览:", this));
    layout->addWidget(previewLabel);
    layout->addWidget(new QLabel("选择色条:", this));
    layout->addWidget(listWidget);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);

    populateColorMaps();
    if (!colorMaps.isEmpty()) {
        updatePreview(colorMaps.first());
    }

    connect(listWidget, &QListWidget::itemDoubleClicked, this, &ColorMapDialog::onItemDoubleClicked);
    connect(okButton, &QPushButton::clicked, this, &ColorMapDialog::onAccepted);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    // 连接当前项变化信号，实时更新预览
    connect(listWidget, &QListWidget::currentItemChanged,
            [this](QListWidgetItem *current, QListWidgetItem*) {
                if (current) {
                    int idx = current->data(Qt::UserRole).toInt();
                    if (idx >= 0 && idx < colorMaps.size()) {
                        updatePreview(colorMaps[idx]);
                    }
                }
            });
}

void ColorMapDialog::populateColorMaps()
{
    colorMaps.clear();
    colorMaps.append(GeoTIFFLoader::getDefaultColorMap());
    colorMaps.append(GeoTIFFLoader::getTerrainColorMap());
    colorMaps.append(GeoTIFFLoader::getCoolWarmColorMap());
    colorMaps.append(GeoTIFFLoader::getViridisColorMap());
    colorMaps.append(GeoTIFFLoader::getPlasmaColorMap());
    colorMaps.append(GeoTIFFLoader::getSpectralColorMap());
    colorMaps.append(GeoTIFFLoader::getRdBuColorMap());

    for (int i = 0; i < colorMaps.size(); ++i) {
        const auto& cm = colorMaps[i];
        QListWidgetItem *item = new QListWidgetItem(QString::fromStdString(cm.name), listWidget);
        item->setData(Qt::UserRole, i);
    }

    listWidget->setCurrentRow(0);
}

void ColorMapDialog::updatePreview(const GeoTIFFLoader::ColorMap& cm)
{
    QPixmap pixmap(previewLabel->size());
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);

    int width = pixmap.width();
    int height = pixmap.height();
    int numColors = cm.colors.size();

    for (int x = 0; x < width; ++x) {
        double t = static_cast<double>(x) / width;
        int idx = qMin(static_cast<int>(t * (numColors - 1)), numColors - 1);
        auto [r, g, b, a] = cm.colors[idx];
        QColor color(
            static_cast<int>(r * 255),
            static_cast<int>(g * 255),
            static_cast<int>(b * 255),
            static_cast<int>(a * 255)
        );
        painter.setPen(color);
        painter.drawLine(x, 0, x, height);
    }

    previewLabel->setPixmap(pixmap);
}

GeoTIFFLoader::ColorMap ColorMapDialog::getSelectedColorMap() const
{
    return selectedMap;
}

void ColorMapDialog::onItemDoubleClicked(QListWidgetItem *item)
{
    int idx = item->data(Qt::UserRole).toInt();
    if (idx >= 0 && idx < colorMaps.size()) {
        selectedMap = colorMaps[idx];
        accept();
    }
}

void ColorMapDialog::onAccepted()
{
    QListWidgetItem *item = listWidget->currentItem();
    if (item) {
        int idx = item->data(Qt::UserRole).toInt();
        if (idx >= 0 && idx < colorMaps.size()) {
            selectedMap = colorMaps[idx];
            accept();
        }
    }
}
