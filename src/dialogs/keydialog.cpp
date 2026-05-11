#include "keydialog.h"
#include <QMessageBox>

KeyDialog::KeyDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("输入天地图密钥");
    setFixedSize(400, 150);

    QLabel *label = new QLabel("请输入天地图API密钥:", this);
    keyEdit = new QLineEdit(this);
    keyEdit->setPlaceholderText("从 http://lbs.tianditu.gov.cn/ 获取");

    okButton = new QPushButton("确定", this);
    cancelButton = new QPushButton("取消", this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(label);
    layout->addWidget(keyEdit);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, this, &KeyDialog::onAccept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(keyEdit, &QLineEdit::returnPressed, this, &KeyDialog::onAccept);
}

QString KeyDialog::getKey() const
{
    return key;
}

void KeyDialog::onAccept()
{
    key = keyEdit->text().trimmed();
    if (key.isEmpty()) {
        QMessageBox::warning(this, "警告", "密钥不能为空！");
        return;
    }
    accept();
}
