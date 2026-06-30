#ifndef PASSWORD_FORM_H
#define PASSWORD_FORM_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QDateTime>

namespace Ui {
class Password_Form;
}

class Password_Form : public QWidget
{
    Q_OBJECT

public:
    explicit Password_Form(QWidget *parent_ = 0, bool showFogotPassword_ = true, bool backKind_ = false);
    ~Password_Form();

    void stopWrongPassword();
    void setTitle(QString title_);
    QTimer wrongPasswordTimer;
    QDateTime wrongPasswordTime;
    QString title;
    QString stringpasswrd;

    QLabel* labelText;
    QList<QLabel*> labels;
    QWidget* parent;
    bool showFogotPassword;
public slots:
    void Recieve_pass_name(int);
    void Recieve_secret_pass_name(int);
signals:
    void Result_pass(bool);
    void Send_correct(int);
private slots:
    void wrongPasswordFlash();

    void on_pushButton_0_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_pushButton_4_clicked();

    void on_pushButton_5_clicked();

    void on_pushButton_6_clicked();

    void on_pushButton_7_clicked();

    void on_pushButton_8_clicked();

    void on_pushButton_9_clicked();

    void on_pushButton_clean_clicked();

    void on_pushButton_back_clicked();

    void on_pushButton_ok_clicked();

    void on_pushButton_1_clicked();

    void on_pushButton_0_pressed();

    void on_pushButton_1_pressed();

    void on_pushButton_2_pressed();

    void on_pushButton_3_pressed();

    void on_pushButton_4_pressed();

    void on_pushButton_5_pressed();

    void on_pushButton_6_pressed();

    void on_pushButton_7_pressed();

    void on_pushButton_8_pressed();

    void on_pushButton_9_pressed();

    void on_pushButton_clean_pressed();

    void on_pushButton_forgot_clicked();


private:
    Ui::Password_Form *ui;
};

#endif // PASSWORD_FORM_H
