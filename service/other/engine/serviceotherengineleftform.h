#ifndef SERVICEOTHERENGINELEFTFORM_H
#define SERVICEOTHERENGINELEFTFORM_H

#include <QWidget>
#include <QLabel>
#include <QSlider>

#include <can/mycan.h>
#include <interface_button/interfacebutton.h>
namespace Ui {
class ServiceOtherEngineLeftForm;
}

class MainWindow;
class ServiceOtherEngineLeftForm : public QWidget
{
    Q_OBJECT

public:
    explicit ServiceOtherEngineLeftForm(MainWindow* mainWindow, QWidget *parent = nullptr);
    ~ServiceOtherEngineLeftForm();

    void updateVisual();

    quint16 rpm_need;

    QWidget* parent;
    
    MainWindow *_mainWindow;
    bool starterBtnStatus();
protected:
    void showEvent(QShowEvent *event) override;// при входе в окно - предупреждение о неисправной кнопке стартера
private slots:

    void on_pushButton_preroll_clicked();
    void on_pushButton_starter_pressed();
    void on_pushButton_starter_released();

    void on_pushButton_starterPreroll_clicked();
    void on_pushButton_ignition_clicked();

    void on_pushButton_lessRPM_clicked();
    void on_pushButton_moreRPM_clicked();

private:
    Ui::ServiceOtherEngineLeftForm *ui;
};

#endif // SERVICEOTHERENGINELEFTFORM_H
