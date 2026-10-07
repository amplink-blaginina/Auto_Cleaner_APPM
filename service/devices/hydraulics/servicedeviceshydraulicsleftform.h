#ifndef SERVICEDEVICESHYDRAULICSLEFTFORM_H
#define SERVICEDEVICESHYDRAULICSLEFTFORM_H

#include <QWidget>
#include <QLabel>
#include <QSlider>
#include <screenlog.h>

#include <Controllers/cancontroller.h>
#include <Controllers/viewcontroller.h>
#include <can/mycan.h>
#include <interface_button/interfacebutton.h>

namespace Ui {
class ServiceDevicesHydraulicsLeftForm;
}

class ServiceDevicesHydraulicsLeftForm : public QWidget
{
    Q_OBJECT

public:
    explicit ServiceDevicesHydraulicsLeftForm(CanController* can, ViewController* view, QWidget *parent = nullptr);
    ~ServiceDevicesHydraulicsLeftForm();

    void refreshSliders();
    void updateVisual();



private slots:
    void on_horizontalSlider_broomRotateLeft_valueChanged(int value);

    void on_horizontalSlider_broomRotateRight_valueChanged(int value);

    void on_horizontalSlider_fanRotateRight_valueChanged(int value);

    void on_pushButton_dumpFlow_clicked();

    void on_pushButton_broomFlow_clicked();

protected:
    void hideEvent(QHideEvent *event) override;// ушли с экрана - поджим/отжим щётки выключаем

private:
    void updateBroomPress();
    bool _broomPressHeld = false;// поджим или отжим включён кнопкой этого экрана
    Ui::ServiceDevicesHydraulicsLeftForm *ui;
    QWidget* _parent;
    CanController* _can;
    ScreenLog* _logger;
    ViewController *_view;
};

#endif // SERVICEDEVICESHYDRAULICSLEFTFORM_H
