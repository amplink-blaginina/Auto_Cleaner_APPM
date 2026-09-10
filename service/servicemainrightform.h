#ifndef SERVICEMAINRIGHTFORM_H
#define SERVICEMAINRIGHTFORM_H

#include <QWidget>
#include <QPushButton>
#include <qsettings.h>

#include "../pdf/pdfscroller.h"

namespace Ui {
class ServiceMainRightForm;
}

#define SERVICE_MENU_SIZE 5
class MainWindow;
class ServiceMainRightForm : public QWidget
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

public:
    explicit ServiceMainRightForm(MainWindow* mainWindow, QWidget *parent = nullptr);
    ~ServiceMainRightForm();

    QWidget* parent;

    void addMenu(QString name_, quint8 id_, quint8 id1_, quint8 goLevel_, QWidget* form_, QString png_);
    void moveMenu(qint8 level);
    void showService();
    struct MenuElement menu[SERVICE_MENU_SIZE + 1][SERVICE_MENU_SIZE];
    quint8 currentLevel;
    qint8 currentElement;
    quint8 oldLevel;
    qint8 oldCurrentElement;
    quint8 oldCurrentLevel;
    QPushButton* buttons[SERVICE_MENU_SIZE];

    QWidget* pdfWidget;
    PdfScroller* pdfScroller;
signals:
    void Send_Pass_2_pass_form(int);
    void Send_SecretPass_2_pass_form(int);
private slots:
    void passwordDiagOk(int pass);
    void on_pushButton_1_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_3_clicked();
    void on_pushButton_4_clicked();
    void on_pushButton_exit_clicked();

private:
    Ui::ServiceMainRightForm *ui;
    QSettings *_settings;
    MainWindow *_mainWindow;
};

#endif // SERVICEMAINRIGHTFORM_H
