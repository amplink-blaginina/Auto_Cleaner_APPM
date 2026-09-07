#include "servicegpioserviceintervalleftform.h"
#include "ui_servicegpioserviceintervalleftform.h"

#include <QLabel>

#include "mainwindow.h"

ServiceGPIOServiceIntervalLeftForm::ServiceGPIOServiceIntervalLeftForm(QWidget *parent_) :
    QWidget(parent_),
    ui(new Ui::ServiceGPIOServiceIntervalLeftForm)
{
    ui->setupUi(this);

    parent = parent_;

    choosePdfVboxLayout = new QVBoxLayout(this);

    lessTOValueButton = new QPushButton(this);
    lessTOValueButton->setStyleSheet("QPushButton{\
                                         border-style:none;\
                                         outline: none;\
                                             background-image: url(:/Images/Images/service/other/intervals/buttons/service_engine_sub-background_TO_button_left_off.png);\
                                         }\
                                         QPushButton::pressed\
                                         {\
                                         border-style:none;\
                                         outline: none;\
                                             background-image: url(:/Images/Images/service/other/intervals/buttons/service_engine_sub-background_TO_button_left_on.png);\
                                         }");
    lessTOValueButton->hide();
    //connect(lessTOValueButton, SIGNAL(clicked()), this, SLOT(on_pushButton_lessTOValueButton_clicked()));
    connect(lessTOValueButton,  &QPushButton::clicked, this, &ServiceGPIOServiceIntervalLeftForm::on_pushButton_lessTOValueButton_clicked);

    moreTOValueButton = new QPushButton(this);
    moreTOValueButton->setStyleSheet("QPushButton{\
                                     border-style:none;\
                                     outline: none;\
                                         background-image: url(:/Images/Images/service/other/intervals/buttons/service_engine_sub-background_TO_button_right_off.png);\
                                     }\
                                     QPushButton::pressed\
                                     {\
                                     border-style:none;\
                                     outline: none;\
                                         background-image: url(:/Images/Images/service/other/intervals/buttons/service_engine_sub-background_TO_button_right_on.png);\
                                     }");
    moreTOValueButton->hide();
    //connect(moreTOValueButton, SIGNAL(clicked()), this, SLOT(on_pushButton_moreTOValueButton_clicked()));
    connect(moreTOValueButton,  &QPushButton::clicked, this, &ServiceGPIOServiceIntervalLeftForm::on_pushButton_moreTOValueButton_clicked);
    resetTOValueButton = new QPushButton(this);
    resetTOValueButton->setStyleSheet("QPushButton{\
                                      border-style:none;\
                                      outline: none;\
                                          background-image: url(:/Images/Images/service/other/intervals/buttons/service_button_reset_off.png);\
                                      }\
                                      QPushButton::pressed\
                                      {\
                                      border-style:none;\
                                      outline: none;\
                                          background-image: url(:/Images/Images/service/other/intervals/buttons/service_button_reset_on.png);\
                                      }");
    resetTOValueButton->hide();
    //connect(resetTOValueButton, SIGNAL(clicked()), this, SLOT(on_pushButton_resetTOValueButton_clicked()));
    connect(resetTOValueButton, &QPushButton::clicked, this, &ServiceGPIOServiceIntervalLeftForm::on_pushButton_resetTOValueButton_clicked);
    //TOLabel = new QPushButton(this);
    //TOLabel->setStyleSheet("QPushButton{border-style:none;outline: none;background-image: url(:/Images/Images/service/gpio/intervals/buttons/settings_button_moreLess.png)}");
    //TOLabel->hide();

    TOValueLabel = new QLabel(this);
    TOValueLabel->setFont(QFont("Mont", 16));
    TOValueLabel->setText( QStringLiteral("%1").arg(0, 4, 10, QLatin1Char('0')));
    TOValueLabel->setStyleSheet("color: rgb(238, 238, 236);");

    // добавим картинку с нкопками
    //TOLabel->setGeometry(100,650,228,76);
    //TOLabel->show();
    //TOLabel->raise();

    moreTOValueButton->setGeometry(255,515,49,49);
    moreTOValueButton->show();
    moreTOValueButton->raise();

    TOValueLabel->setGeometry(188,515,70,50);
    TOValueLabel->show();
    TOValueLabel->raise();

    resetTOValueButton->setGeometry(350,515,49,49);
    resetTOValueButton->show();
    resetTOValueButton->raise();

    lessTOValueButton->setGeometry(110,515,49,49);
    lessTOValueButton->show();
    lessTOValueButton->raise();

    choosedTOElement = -1;
    TOValueLabel->hide();

    choosePdfScrollArea = new QScrollArea;
    choosePdfScrollAreaWidget = new QWidget;
    choosePdfScrollAreaWidget->setObjectName("choosePdfScrollAreaWidget");
    choosePdfScrollAreaWidget->setStyleSheet("QWidget#choosePdfScrollAreaWidget{border-style:none;outline: none;}");
    choosePdfScrollArea->setStyleSheet("QScrollArea {background-color: transparent; border-style:none;outline: none;}");

    choosePdfGrid = new QGridLayout();

    //choosePdfScrollArea->setBackgroundRole(QPalette::Dark);
    choosePdfScrollArea->setWidget(choosePdfScrollAreaWidget);
    choosePdfScrollAreaWidget->setLayout(choosePdfGrid);

    QLabel* l = new QLabel();
    l->setFixedSize(50, 50);
    choosePdfVboxLayout->addWidget(l);
    choosePdfVboxLayout->addWidget(choosePdfScrollArea);

    QScroller::grabGesture(choosePdfScrollArea->viewport(), QScroller::LeftMouseButtonGesture);

    fillElements();

    choosePdfScrollArea->setAlignment(Qt::AlignLeft|Qt::AlignTop);
    choosePdfVboxLayout->setAlignment(Qt::AlignLeft|Qt::AlignTop);
    //choosePdfGrid->setAlignment(Qt::AlignHCenter|Qt::AlignTop);
    choosePdfScrollArea->setGeometry(0, 0, 900, 400);
    choosePdfScrollArea->setFixedSize(900, 400);
    choosePdfGrid->setGeometry(QRect(0, 0, 900, 400));
    choosePdfVboxLayout->setGeometry(QRect(0, 0, 900, 400));
}

void ServiceGPIOServiceIntervalLeftForm::fillElements()
{
    for (int i = 0; i < elementsTO.count(); i++)
    {
        elementsTO.at(i)->elementButton->disconnect();
        elementsTO.at(i)->deleteLater();
    }
    elementsTO.clear();
    foreach (QString key, ((MainWindow*)parent)->TONameValues.keys())
    {
        if (((MainWindow*)parent)->TOSourceValues[key] == 1)
            elementsTO.append(new ServiceTOElement(((MainWindow*)parent)->TONameValues[key], key, ((MainWindow*)parent)->TOCurValues["System"] - ((MainWindow*)parent)->TOCurValues[key], ((MainWindow*)parent)->TOValues[key], 0, 1000 * 3600, 3600));
        else
            elementsTO.append(new ServiceTOElement(((MainWindow*)parent)->TONameValues[key], key, ((MainWindow*)parent)->TOCurValues["Engine"] - ((MainWindow*)parent)->TOCurValues[key], ((MainWindow*)parent)->TOValues[key], 0, 1000 * 3600, 3600));
        choosePdfGrid->addWidget(elementsTO.last(), ((elementsTO.size() - 1) / 4), (elementsTO.size() - 1) % 4);
        checkElementTO(elementsTO.size() - 1);
        connect(elementsTO.last()->elementButton, SIGNAL(clicked()), this, SLOT(elementTOClicked()));
    }

    //choosePdfScrollAreaWidget->setGeometry(0, 0, 207 * 4, (((elementsTO.size() - 1) / 4) + 1) * 170);
    choosePdfScrollAreaWidget->setFixedSize(207 * 4, (((elementsTO.size() - 1) / 4) + 1) * 130);
}

void ServiceGPIOServiceIntervalLeftForm::elementTOClicked()
{
    QPushButton* obj = (QPushButton*)sender();
    for (int i = 0; i < elementsTO.size(); i++)
    {
        elementsTO.at(i)->elementChoosedLabel->hide();
        if (elementsTO.at(i)->elementButton == obj)
        {// нашли кнопку которую тыкнули в сетке
            elementsTO.at(i)->elementChoosedLabel->show();
            if (choosedTOElement == i)
            {// отжали
                elementsTO.at(i)->elementChoosedLabel->hide();
                choosedTOElement = -1;
                TOValueLabel->hide();
            }
            else
            {
                choosedTOElement = i;
                TOValueLabel->show();
                TOValueLabel->setText(QStringLiteral("%1").arg(elementsTO.at(i)->valueTO / 3600, 4, 10, QLatin1Char('0')));
            }
        }
    }
}

void ServiceGPIOServiceIntervalLeftForm::on_pushButton_lessTOValueButton_clicked()
{
    if (choosedTOElement >= 0)
    {
        quint32 value = elementsTO.at(choosedTOElement)->valueTO;
        quint32 step = elementsTO.at(choosedTOElement)->step;
        quint32 minVal = elementsTO.at(choosedTOElement)->minVal;
        quint32 maxVal = elementsTO.at(choosedTOElement)->maxVal;
        value -= step;
        if (value < minVal)
            value = minVal;
        elementsTO.at(choosedTOElement)->valueTO = value;
        elementsTO.at(choosedTOElement)->elementTOValueLabel->setText( QStringLiteral("%1").arg(value / 3600, 4, 10, QLatin1Char('0')));
        TOValueLabel->setText( QStringLiteral("%1").arg(value / 3600, 4, 10, QLatin1Char('0')));

        checkElementTO(choosedTOElement);
    }
}

void ServiceGPIOServiceIntervalLeftForm::on_pushButton_moreTOValueButton_clicked()
{
    if (choosedTOElement >= 0)
    {
        quint32 value = elementsTO.at(choosedTOElement)->valueTO;
        quint32 step = elementsTO.at(choosedTOElement)->step;
        quint32 minVal = elementsTO.at(choosedTOElement)->minVal;
        quint32 maxVal = elementsTO.at(choosedTOElement)->maxVal;
        value += step;
        if (value > maxVal)
            value = maxVal;
        elementsTO.at(choosedTOElement)->valueTO = value;
        elementsTO.at(choosedTOElement)->elementTOValueLabel->setText( QStringLiteral("%1").arg(value / 3600, 4, 10, QLatin1Char('0')));
        TOValueLabel->setText( QStringLiteral("%1").arg(value / 3600, 4, 10, QLatin1Char('0')));

        checkElementTO(choosedTOElement);
    }
}

void ServiceGPIOServiceIntervalLeftForm::updateElementTO(int element)
{
    if (((MainWindow*)parent)->TOSourceValues[elementsTO.at(element)->settingsName] == 1)
        elementsTO.at(element)->value = ((MainWindow*)parent)->TOCurValues["System"] - ((MainWindow*)parent)->TOCurValues[elementsTO.at(element)->settingsName];
    else
        elementsTO.at(element)->value = ((MainWindow*)parent)->TOCurValues["Engine"] - ((MainWindow*)parent)->TOCurValues[elementsTO.at(element)->settingsName];
    elementsTO.at(element)->elementValueLabel->setText(QStringLiteral("%1").arg(elementsTO.at(element)->value / 3600, 4, 10, QLatin1Char('0')));
}

void ServiceGPIOServiceIntervalLeftForm::checkElementTO(int element)
{
    if (elementsTO.at(element)->valueTO < elementsTO.at(element)->value)
    {
        elementsTO.at(element)->elementValueLabel->setStyleSheet("color:red;");
    }
    else
    {
        elementsTO.at(element)->elementValueLabel->setStyleSheet("color: rgb(238, 238, 236);");
    }
}

void ServiceGPIOServiceIntervalLeftForm::on_pushButton_resetTOValueButton_clicked()
{
    if (choosedTOElement >= 0)
    {
        elementsTO.at(choosedTOElement)->value = 0;
        elementsTO.at(choosedTOElement)->elementValueLabel->setText(QStringLiteral("%1").arg(0, 4, 10, QLatin1Char('0')));
        // занулялось - не понял почему
        //TOValueLabel->setText( QStringLiteral("%1").arg(0, 4, 10, QLatin1Char('0')));

        checkElementTO(choosedTOElement);
    }
}

void ServiceGPIOServiceIntervalLeftForm::saveIntervals()
{
    for (int i = 0; i < elementsTO.count(); i++)
    {
        ((MainWindow*)parent)->settings->setValue("TO/" + elementsTO.at(i)->settingsName, elementsTO.at(i)->valueTO);
        if (((MainWindow*)parent)->TOSourceValues[elementsTO.at(i)->settingsName] == 1)
            ((MainWindow*)parent)->settings->setValue("TOCur/" + elementsTO.at(i)->settingsName, ((MainWindow*)parent)->TOCurValues["System"] - elementsTO.at(i)->value);
        else
            ((MainWindow*)parent)->settings->setValue("TOCur/" + elementsTO.at(i)->settingsName, ((MainWindow*)parent)->TOCurValues["Engine"] - elementsTO.at(i)->value);
    }

    ((MainWindow*)parent)->settings->sync();
    system("sync");
}

ServiceGPIOServiceIntervalLeftForm::~ServiceGPIOServiceIntervalLeftForm()
{
    delete ui;
}
