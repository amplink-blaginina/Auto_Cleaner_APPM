#ifndef FOGOTFORM_H
#define FOGOTFORM_H

#include <QWidget>

namespace Ui {
class FogotForm;
}

class FogotForm : public QWidget
{
    Q_OBJECT

public:
    explicit FogotForm(int secretPassword, QWidget *parent = nullptr);
    ~FogotForm();

private slots:
    void on_pushButton_clicked();

private:
    Ui::FogotForm *ui;
};

#endif // FOGOTFORM_H
