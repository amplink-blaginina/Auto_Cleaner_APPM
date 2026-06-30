#ifndef SERVICEGLOBALDATETIMELEFTFORM_H
#define SERVICEGLOBALDATETIMELEFTFORM_H

#include <QWidget>
#include <QDateTime>

#include "interface_button/interfacebutton.h"

namespace Ui {
class ServiceGlobalDateTimeLeftForm;
}

class ServiceGlobalDateTimeLeftForm : public QWidget
{
    Q_OBJECT

public:
    explicit ServiceGlobalDateTimeLeftForm(QWidget *parent = nullptr);
    ~ServiceGlobalDateTimeLeftForm();

    void actualTime();
    void showTime();

    QWidget* parent;

    QDateTime curDT;
    bool changed;

private slots:

    void on_pushButton_hour_more_clicked();

    void on_pushButton_hour_less_clicked();

    void on_pushButton_minute_more_clicked();

    void on_pushButton_minute_less_clicked();

    void on_pushButton_second_more_clicked();

    void on_pushButton_second_less_clicked();

    void on_pushButton_day_more_clicked();

    void on_pushButton_day_less_clicked();

    void on_pushButton_month_more_clicked();

    void on_pushButton_month_less_clicked();

    void on_pushButton_year_more_clicked();

    void on_pushButton_year_less_clicked();

private:
    Ui::ServiceGlobalDateTimeLeftForm *ui;
};

#endif // SERVICEGLOBALDATETIMELEFTFORM_H
