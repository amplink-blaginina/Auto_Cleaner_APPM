#ifndef SERVICETOELEMENT_H
#define SERVICETOELEMENT_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>

namespace Ui {
class ServiceTOElement;
}

class ServiceTOElement : public QWidget
{
    Q_OBJECT

public:
    explicit ServiceTOElement(QString humanName_, QString settingsName_, int value_, int valueTO_, int minVal_, int maxVal_, float step_ = 1, QWidget *parent = nullptr);
    ~ServiceTOElement();

    QString humanName;
    QString settingsName;
    int value;
    int valueTO;
    int minVal;
    int maxVal;
    float step;
    QPushButton* elementButton;
    QLabel* elementChoosedLabel;
    QLabel* elementValueLabel;
    QLabel* elementTOValueLabel;

private:
    Ui::ServiceTOElement *ui;
};

#endif // SERVICETOELEMENT_H
