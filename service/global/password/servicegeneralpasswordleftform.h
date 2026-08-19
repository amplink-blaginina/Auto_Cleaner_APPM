#ifndef SERVICEGENERALPASSWORDLEFTFORM_H
#define SERVICEGENERALPASSWORDLEFTFORM_H

#include <QWidget>
#include <QDateTime>
#include <QLabel>

#include <interface_button/interfacebutton.h>

namespace Ui {
class ServiceGeneralPasswordLeftForm;
}

class ServiceGeneralPasswordLeftForm : public QWidget
{
    Q_OBJECT

public:
    explicit ServiceGeneralPasswordLeftForm(QWidget *parent = nullptr);
    ~ServiceGeneralPasswordLeftForm();

    void stopWrongPassword();
    void goStep(quint8 step);

    quint8 currentStep;
    QTimer wrongPasswordTimer;
    QDateTime wrongPasswordTime;
    bool wrongPassword;
    QString stringpasswrd;
    QList<QLabel*> labels;
    int passwrd;
    QString passwordVariable;

    QWidget* parent;

    void resetPassword();
private slots:
    void wrongPasswordFlash();
    void on_pushButton_1_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_3_clicked();
    void on_pushButton_4_clicked();
    void on_pushButton_5_clicked();
    void on_pushButton_6_clicked();
    void on_pushButton_7_clicked();
    void on_pushButton_8_clicked();
    void on_pushButton_9_clicked();
    void on_pushButton_0_clicked();
    void on_pushButton_backspace_clicked();
    void on_pushButton_save_clicked();

private:
    Ui::ServiceGeneralPasswordLeftForm *ui;
};

#endif // SERVICEGENERALPASSWORDLEFTFORM_H
