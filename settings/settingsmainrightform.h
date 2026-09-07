#ifndef SETTINGSMAINRIGHTFORM_H
#define SETTINGSMAINRIGHTFORM_H

#include <QWidget>
//#include <mainwindow.h>

#include <Controllers/cancontroller.h>

#include "../pdf/pdfscroller.h"

#include "settingsform.h"

class MainWindow;

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
    explicit SettingsMainRightForm(MyCan* can, SettingsForm *settingsForm, MainWindow* mainWindow, QWidget *parent);
    ~SettingsMainRightForm();


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
    MainWindow *_mainWindow;
private slots:
    void on_pushButton_1_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_3_clicked();
    void on_pushButton_4_clicked();
    void on_pushButton_exit_clicked();

private:
    Ui::SettingsMainRightForm *ui;
    QWidget* _parent;
    MyCan* _can;
};

#endif // SETTINGSMAINRIGHTFORM_H
