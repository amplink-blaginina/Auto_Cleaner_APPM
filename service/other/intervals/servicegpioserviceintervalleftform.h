#ifndef SERVICEGPIOSERVICEINTERVALLEFTFORM_H
#define SERVICEGPIOSERVICEINTERVALLEFTFORM_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QScroller>


#include "servicetoelement.h"
class MainWindow;
namespace Ui {
class ServiceGPIOServiceIntervalLeftForm;
}

class ServiceGPIOServiceIntervalLeftForm : public QWidget
{
    Q_OBJECT

public:
    explicit ServiceGPIOServiceIntervalLeftForm(MainWindow* mainWindow, QWidget *parent = nullptr);
    ~ServiceGPIOServiceIntervalLeftForm();

    void checkElementTO(int element);
    void updateElementTO(int element);
    void fillElements();
    void saveIntervals();

    QWidget* parent;

    QVBoxLayout * choosePdfVboxLayout;
    QScrollArea* choosePdfScrollArea;
    QWidget* choosePdfScrollAreaWidget;
    QGridLayout* choosePdfGrid;

    QList<ServiceTOElement*> elementsTO;
    QPushButton* lessTOValueButton;
    QPushButton* moreTOValueButton;
    QPushButton* resetTOValueButton;
    QPushButton* TOLabel;
    QLabel* TOValueLabel;
    int choosedTOElement;

public slots:
    void on_pushButton_lessTOValueButton_clicked();
    void on_pushButton_moreTOValueButton_clicked();
    void on_pushButton_resetTOValueButton_clicked();
    void elementTOClicked();

private:
    Ui::ServiceGPIOServiceIntervalLeftForm *ui;
    MainWindow *_mainWindow;
};

#endif // SERVICEGPIOSERVICEINTERVALLEFTFORM_H
