#ifndef SETTINGSMAINRIGHTFORM_H
#define SETTINGSMAINRIGHTFORM_H

#include <QWidget>

#include "../pdf/pdfscroller.h"

#include "settingsform.h"

namespace Ui {
class SettingsMainRightForm;
}

#define MENU_SIZE 5

class SettingsMainRightForm : public QWidget
{
    Q_OBJECT

    struct MenuElement
    {
        quint8 goLevel;
        QWidget* form;
        QString name;
        QString png;
        QPushButton* button;
    };

public: //= nullptr
    explicit SettingsMainRightForm(QWidget *parent, SettingsForm *settingsForm);
    ~SettingsMainRightForm();

    QWidget* parent;

    void addMenu(QString name_, quint8 id_, quint8 id1_, quint8 goLevel_, QWidget* form_, QString png_);
    void moveMenu(qint8 level);
    void showService();
    struct MenuElement menu[MENU_SIZE + 1][MENU_SIZE];
    quint8 currentLevel;
    qint8 currentElement;
    QPushButton* buttons[MENU_SIZE];

    QWidget* pdfWidget;
    PdfScroller* pdfScroller;
    
    SettingsForm *_settingsForm;
private slots:
    void on_pushButton_1_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_3_clicked();
    void on_pushButton_4_clicked();
    void on_pushButton_exit_clicked();

private:
    Ui::SettingsMainRightForm *ui;
};

#endif // SETTINGSMAINRIGHTFORM_H
