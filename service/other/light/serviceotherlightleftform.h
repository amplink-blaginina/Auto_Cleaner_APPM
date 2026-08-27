#ifndef SERVICEOTHERLIGHTLEFTFORM_H
#define SERVICEOTHERLIGHTLEFTFORM_H

#include <QWidget>
#include <QLabel>
#include <QSlider>

#include <Controllers/cancontroller.h>
#include <can/mycan.h>
#include <interface_button/interfacebutton.h>

namespace Ui {
class ServiceOtherLightLeftForm;
}

class ServiceOtherLightLeftForm : public QWidget
{
    Q_OBJECT

public:
    explicit ServiceOtherLightLeftForm(CanController* can,QWidget *parent = nullptr);
    ~ServiceOtherLightLeftForm();

    void updateVisual();

    QWidget* parent;

private slots:

    void on_pushButton_lightFRM1_clicked();

    void on_pushButton_lightFRM2_clicked();

    void on_pushButton_lightFRM3_clicked();

private:
    Ui::ServiceOtherLightLeftForm *ui;
    CanController* _can;
};

#endif // SERVICEOTHERLIGHTLEFTFORM_H
