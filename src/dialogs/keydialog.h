#ifndef KEYDIALOG_H
#define KEYDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>

class KeyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit KeyDialog(QWidget *parent = nullptr);
    QString getKey() const;

private slots:
    void onAccept();

private:
    QLineEdit *keyEdit;
    QPushButton *okButton;
    QPushButton *cancelButton;
    QString key;
};

#endif // KEYDIALOG_H
