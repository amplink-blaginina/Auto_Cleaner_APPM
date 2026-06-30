#include "interfacebutton.h"

#include "mainwindow.h"

InterfaceButton::InterfaceButton(ButtonPressType pressType_, QPushButton* element_, QString offPicture_, QString onPicture_, QObject *parent_) : QObject(parent_)
{
    pressType = pressType_;
    element = element_;
    state = Off;
    enabled = true;
    if (offPicture_ == "")
        offPicture = "border-style:none;outline: none;";
    else
        offPicture = "border-style:none;outline: none;background-image: " + offPicture_;
    if (onPicture_ == "")
        onPicture = "border-style:none;outline: none;";
    else
        onPicture = "border-style:none;outline: none;background-image: " + onPicture_;
    parent = parent_;

    elementEffect = new QGraphicsOpacityEffect(this);
    element->setGraphicsEffect(elementEffect);
    elementEffect->setOpacity(1.0);

    pressedCnt = 0;

    connect(&mainProgressTimer, SIGNAL(timeout()), this, SLOT(mainProgress()));
    mainProgressTimer.start(100);

    //connect(element, SIGNAL(clicked()), this, SLOT(clicked()));
    connect(element, SIGNAL(pressed()), this, SLOT(pressed()));

    updateVisual();
}

void InterfaceButton::setEnabled(bool en)
{
    if (en != enabled)
    {
        enabled = en;
        updateVisual();
    }
}

void InterfaceButton::mainProgress()
{
    if (pressedCnt > 0)
        pressedCnt--;
}

void InterfaceButton::clicked()
{
    if (!enabled)
        return;
    pressedCnt = 0;
    if (pressType == WithFix)
    {
        if (state != On)
            state = On;
        else
            state = Off;
    }
    else if (pressType == WithNoFix)
    {
        state = Off;
    }
    updateVisual();
}

void InterfaceButton::pressed()
{
    if (!enabled)
        return;
    pressedCnt = 10;
//    if (pressType == WithFix)
//    {
//        state = Pressed;
//    }
    if (pressType == WithNoFix)
    {
        state = Pressed;
    }
    updateVisual();
}

void InterfaceButton::updateVisual()
{
    if (state == On || (state == Pressed && pressType == WithNoFix) && element->styleSheet() != onPicture)
        element->setStyleSheet(onPicture);
    else if (state == Off && element->styleSheet() != offPicture)
        element->setStyleSheet(offPicture);
    if (enabled && elementEffect->opacity() != 1.0)
        elementEffect->setOpacity(1.0);
    if (!enabled && elementEffect->opacity() == 1.0)
        elementEffect->setOpacity(0.5);
}
