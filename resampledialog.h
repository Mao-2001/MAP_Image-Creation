#ifndef RESAMPLEDIALOG_H
#define RESAMPLEDIALOG_H

#include <QDialog>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>

class ResampleDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ResampleDialog(QWidget *parent = nullptr);
    double getResolution() const;
    std::string getResampleMethod() const;

private:
    QDoubleSpinBox *resolutionSpinBox;
    QComboBox *methodCombo;
    QPushButton *okButton;
    QPushButton *cancelButton;
};

#endif // RESAMPLEDIALOG_H
