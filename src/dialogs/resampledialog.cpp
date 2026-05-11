#include "resampledialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

ResampleDialog::ResampleDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("批量降采样设置");
    setFixedSize(350, 200);

    resolutionSpinBox = new QDoubleSpinBox(this);
    resolutionSpinBox->setRange(1, 100000);
    resolutionSpinBox->setValue(3000);
    resolutionSpinBox->setSuffix(" 米");
    resolutionSpinBox->setDecimals(0);

    methodCombo = new QComboBox(this);
    methodCombo->addItem("bilinear (双线性)");
    methodCombo->addItem("near (最邻近)");
    methodCombo->addItem("cubic (三次卷积)");
    methodCombo->addItem("average (平均)");
    methodCombo->setCurrentIndex(0);

    okButton = new QPushButton("确定", this);
    cancelButton = new QPushButton("取消", this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("目标分辨率 (每个像素代表的地面距离):", this));
    layout->addWidget(resolutionSpinBox);
    layout->addSpacing(10);
    layout->addWidget(new QLabel("重采样方法:", this));
    layout->addWidget(methodCombo);
    layout->addSpacing(10);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

double ResampleDialog::getResolution() const
{
    return resolutionSpinBox->value();
}

std::string ResampleDialog::getResampleMethod() const
{
    switch (methodCombo->currentIndex()) {
        case 0: return "bilinear";
        case 1: return "near";
        case 2: return "cubic";
        case 3: return "average";
        default: return "bilinear";
    }
}
