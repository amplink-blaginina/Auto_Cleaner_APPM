#ifndef INTERFACEBUTTON_H
#define INTERFACEBUTTON_H

#include <QObject>
#include <QPushButton>
#include <QTimer>
#include <QGraphicsOpacityEffect>

class InterfaceButton : public QObject
{
    Q_OBJECT
public:
    enum ButtonPressType
    {
        WithNoFix = 0,
        WithFix   = 1
    };

    enum ButtonState
    {
        Off     = 0,
        Pressed = 1,
        On      = 2
    };

    explicit InterfaceButton(ButtonPressType pressType_, QPushButton* element_, QString offPicture_, QString onPicture_, QObject *parent_ = nullptr);

    void updateVisual();
    void setEnabled(bool en);

    QTimer mainProgressTimer;

    quint8 pressedCnt;

    quint8 state;
    ButtonPressType pressType;
    QPushButton* element;
    QGraphicsOpacityEffect* elementEffect;
    QString offPicture;
    QString onPicture;
    QObject *parent;
    bool enabled;

signals:

public slots:
    void clicked();
    void pressed();
    void mainProgress();
};

#endif // INTERFACEBUTTON_H
