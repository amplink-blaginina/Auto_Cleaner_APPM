#include "organbuttons.h"

#include <Controllers/viewcontroller.h>
#include <machine/machinecontext.h>
#include <organs/organ.h>

#include <QLabel>
#include <QPushButton>
#include <QVariant>

OrganButtons::OrganButtons(const QString &busyMessage, Organ *organ, MachineContext *context_, ViewController *view_, QObject *parent)
    : QObject(parent), view(view_), context(context_), _busyMessage(busyMessage), _organ(organ)
{
}

void OrganButtons::addButton(const Button &button){
    ButtonState state;
    state.button = button;
    _buttons.append(state);
}

void OrganButtons::addIcon(QLabel *label, std::function<QString()> style){
    _icons.append({label, std::move(style)});
}

void OrganButtons::update(){
    for (ButtonState &state : _buttons){
        const Button &b = state.button;
        // органа нет в текущем режиме машины - его кнопки (и на пульте) не работают
        const bool now = _organ->isAvailable() && (!b.onlyWhenEnabled || b.screen->isEnabled())
                         && (b.screen->isDown() || (b.physical && b.physical()));
        if (!_initialized){// первый опрос только запоминает состояние: кнопка, зажатая при старте, не срабатывает
            state.pressed = now;
            continue;
        }
        if (now && !state.pressed)
            press(state);
        else if (!now && state.pressed)
            release(state);

        if (state.pressed && state.accepted && !state.held && b.onHeld && context->isCleaning()
            && state.since.elapsed() > b.holdSec * 1000){
            state.held = true;
            b.onHeld();
        }
    }
    _initialized = true;
    refreshIcons();// положение органа меняется и без нажатий
}

bool OrganButtons::showsPressedIcon(const ButtonState &state) const{
    const Button &b = state.button;
    return state.pressed && b.icon && !b.pressedIcon.isEmpty()
           && (b.pressedIconWhileCleaning || !context->isCleaning());
}

void OrganButtons::click(QPushButton *screen){
    if (!_organ->isAvailable())
        return;
    for (ButtonState &state : _buttons)
        if (state.button.screen == screen && !state.pressed)
            press(state);// отпускание увидит следующий опрос
}

void OrganButtons::press(ButtonState &state){
    const Button &b = state.button;
    state.pressed = true;
    state.accepted = false;
    state.held = false;
    b.screen->setProperty("wasDown", true);
    if (showsPressedIcon(state))
        view->setStyle(b.icon, b.pressedIcon);

    if (b.blockWhileMoving && _organ->isTransitioning()){
        view->addLog(_busyMessage);// удержание не засчитываем, пока кнопку не нажмут заново
        return;
    }
    if (context->isCleaning()){
        state.accepted = true;
        state.since.start();
        if (b.onPress)
            b.onPress();
        if (b.holdHint)
            view->addLog(b.holdHint());
    }
    else if (b.onSelect)
        b.onSelect();
}

void OrganButtons::release(ButtonState &state){
    const Button &b = state.button;
    state.pressed = false;
    b.screen->setProperty("wasDown", false);
    refreshIcons();

    if (!state.accepted)
        return;
    state.accepted = false;
    if (b.blockWhileMoving && _organ->isTransitioning())
        return;// орган движет автомат - не мешаем
    if (context->isCleaning() && b.onRelease)
        b.onRelease();
}

void OrganButtons::setWorking(bool working){
    for (const ButtonState &state : _buttons)
        if (state.button.onlyWhenEnabled && state.button.screen->isEnabled() != working)
            state.button.screen->setEnabled(working);
    refreshIcons();
}

void OrganButtons::refreshIcons(){
    for (const auto &icon : _icons){
        bool pressedShown = false;// на иконке сейчас картинка нажатой кнопки
        for (const ButtonState &state : _buttons)
            if (state.button.icon == icon.first && showsPressedIcon(state))
                pressedShown = true;
        if (!pressedShown)
            view->setStyle(icon.first, icon.second());
    }
}

bool OrganButtons::isPressed() const{
    for (const ButtonState &state : _buttons)
        if (state.pressed)
            return true;
    return false;
}
