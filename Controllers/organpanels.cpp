#include "organpanels.h"

#include <Controllers/PhysicalButtonManager.h>
#include <Controllers/viewcontroller.h>
#include <machine/machinecontext.h>
#include <organs/blower.h>
#include <organs/centralbroom.h>
#include <organs/frontrail.h>

#include <QLabel>
#include <QPushButton>

namespace {
const QString buttonsPath = "background-image: url(:/Images/Images/main/buttons/configuration_button_";

std::function<bool()> pultButton(PhysicalButtonManager *pult, GPIOInput input){
    return [pult, input]{ return pult->isPressed(input); };
}

// кнопка стороны вне уборки: выбрать сторону или снять выбор
void toggleSide(Organ *organ, bool right, ViewController *view, const QString &name){
    organ->toggleSide(right);
    if (right ? organ->isRightSelected() : organ->isLeftSelected())
        view->addLog(name + (right ? ": выбрана правая сторона" : ": выбрана левая сторона"));
}

// иконка вверх/вниз органа с ручным управлением: доступна только во время уборки
std::function<QString()> vertIcon(QPushButton *down, const QString &path){
    return [down, path]{ return path + (down->isEnabled() ? "off.png);" : "blocked.png);"); };
}
}

//==============================Отвал================================================
DumpButtons::DumpButtons(const OrganWidgets &w, FrontRail *dump, PhysicalButtonManager *pult,
                         MachineContext *context_, ViewController *view_, QObject *parent)
    : OrganButtons("Отвал в движении, ожидайте", dump, context_, view_, parent), _dump(dump)
{
    const QString vert = buttonsPath + "dozerBlade_lift_";
    const QString side = buttonsPath + "dozerBlade_turn_";
    auto move = [dump](organsEnums::Direction dir){ return [dump, dir]{ dump->setDirection(dir); }; };
    auto stop = move(organsEnums::None);

    Button up;
    up.screen = w.up;
    up.physical = pultButton(pult, GPIOInput::IN_DUMP_UP);
    up.onlyWhenEnabled = true;
    up.icon = w.vertIcon;
    up.pressedIcon = vert + "up_off.png);";
    up.onPress = move(organsEnums::Up);
    up.onRelease = stop;
    addButton(up);

    Button down = up;
    down.screen = w.down;
    down.physical = pultButton(pult, GPIOInput::IN_DUMP_DOWN);
    down.pressedIcon = vert + "down_off.png);";
    down.onPress = move(organsEnums::Down);
    addButton(down);

    Button left;
    left.screen = w.left;
    left.physical = pultButton(pult, GPIOInput::IN_DUMP_LEFT);
    left.icon = w.sideIcon;
    left.pressedIcon = side + "left_on.png);";
    left.pressedIconWhileCleaning = false;// во время уборки видно, на какой стороне отвал сейчас
    left.onPress = move(organsEnums::Left);
    left.onRelease = stop;
    left.onSelect = [this, dump]{ toggleSide(dump, false, view, "Отвал"); };
    addButton(left);

    Button right = left;
    right.screen = w.right;
    right.physical = pultButton(pult, GPIOInput::IN_DUMP_RIGHT);
    right.pressedIcon = side + "right_on.png);";
    right.onPress = move(organsEnums::Right);
    right.onSelect = [this, dump]{ toggleSide(dump, true, view, "Отвал"); };
    addButton(right);

    Button flow;// плавание можно выбрать и до уборки, и во время неё
    flow.screen = w.flow;
    flow.blockWhileMoving = false;
    flow.onPress = [this, dump]{ setFlow(!dump->isFlowSelected()); };
    flow.onSelect = flow.onPress;
    addButton(flow);

    addIcon(w.vertIcon, vertIcon(w.down, vert));
    addIcon(w.sideIcon, [this, dump, side]{
        if (!dump->isSideSelected())
            return side + "off.png);";
        // во время уборки - где отвал сейчас: при ручном повороте сторона меняется за серединой хода
        // (выбор оператора переходит за ней сразу, Organ::followActualSide)
        const bool left = context->isCleaning() ? dump->isOnLeft() : dump->isLeftSelected();
        return side + (left ? "left_on.png);" : "right_on.png);");
    });
    addIcon(w.flowIcon, [dump]{
        return buttonsPath + "variable_up_" + (dump->isFlowSelected() ? "on_down_blocked);" : "off_down_blocked);");
    });

    // отвал двигают вверх/вниз вручную - плавание снимаем
    connect(dump, &FrontRail::flowCancelRequested, this, [this, dump]{
        if (!dump->isTransitioning())
            setFlow(false);
    });
}

void DumpButtons::setFlow(bool state){
    if (state == _dump->isFlowSelected())
        return;
    if (context->isCleaning()){
        view->addLog(state ? "Отвал плавание" : "Отвал плавание завершено");
        _dump->setFlowActive(state);
    }
    else if (state)
        view->addLog("Отвал выбрано плавание ");
    _dump->selectFlow(state);
}

//==============================Щётка================================================
BroomButtons::BroomButtons(const OrganWidgets &w, CentralBroom *broom, PhysicalButtonManager *pult,
                           MachineContext *context_, ViewController *view_, QObject *parent)
    : OrganButtons("Щетка в движении, ожидайте", broom, context_, view_, parent), _broom(broom)
{
    const QString vert = buttonsPath + "rotatingBroomsFront_lift_";
    const QString side = buttonsPath + "rotatingBroomsBelow_";
    auto move = [broom](organsEnums::Direction dir){ return [broom, dir]{ broom->setDirection(dir); }; };
    auto stop = move(organsEnums::None);

    Button up;
    up.screen = w.up;
    up.physical = pultButton(pult, GPIOInput::IN_BROOM_UP);
    up.onlyWhenEnabled = true;
    up.icon = w.vertIcon;
    up.pressedIcon = vert + "up_on.png);";
    up.onPress = move(organsEnums::Up);
    up.onRelease = stop;
    addButton(up);

    Button down = up;
    down.screen = w.down;
    down.physical = pultButton(pult, GPIOInput::IN_BROOM_DOWN);
    down.pressedIcon = vert + "down_on.png);";
    down.onPress = move(organsEnums::Down);
    addButton(down);

    Button left;
    left.screen = w.left;
    left.physical = pultButton(pult, GPIOInput::IN_BROOM_LEFT);
    left.icon = w.sideIcon;
    left.pressedIcon = side + "left_on.png);";
    left.pressedIconWhileCleaning = false;// во время уборки видно, на какой стороне щётка сейчас
    left.onPress = move(organsEnums::Left);
    left.onRelease = stop;
    left.onSelect = [this, broom]{ toggleSide(broom, false, view, "Щетка"); };
    addButton(left);

    Button right = left;
    right.screen = w.right;
    right.physical = pultButton(pult, GPIOInput::IN_BROOM_RIGHT);
    right.pressedIcon = side + "right_on.png);";
    right.onPress = move(organsEnums::Right);
    right.onSelect = [this, broom]{ toggleSide(broom, true, view, "Щетка"); };
    addButton(right);

    Button flow;// плавание и прижим можно выбрать и до уборки, и во время неё
    flow.screen = w.flow;
    flow.blockWhileMoving = false;
    flow.onPress = [this, broom]{ setFlow(!broom->isFlowSelected()); };
    flow.onSelect = flow.onPress;
    addButton(flow);

    Button press;
    press.screen = w.press;
    press.blockWhileMoving = false;
    press.onPress = [broom]{ broom->selectPress(!broom->isPressSelected()); };
    press.onSelect = press.onPress;
    addButton(press);

    addIcon(w.vertIcon, vertIcon(w.down, vert));
    addIcon(w.sideIcon, [this, broom, side]{
        if (!broom->isSideSelected())
            return buttonsPath + "rotatingBroomsFront_off.png);";
        // во время уборки - где щётка сейчас: при ручном повороте сторона меняется за серединой хода
        // (выбор оператора переходит за ней сразу, Organ::followActualSide)
        const bool left = context->isCleaning() ? broom->isOnLeft() : broom->isLeftSelected();
        return side + (left ? "left_on.png);" : "right_on.png);");
    });
    addIcon(w.flowIcon, [broom]{
        return buttonsPath + "variable_"
             + (broom->isFlowSelected() ? (broom->isPressSelected() ? "on.png);" : "up_on.png);")
                                        : (broom->isPressSelected() ? "down_on.png);" : "off.png);"));
    });

    // щётку двигают вверх/вниз вручную - плавание снимаем
    connect(broom, &CentralBroom::flowCancelRequested, this, [this, broom]{
        if (!broom->isTransitioning())
            setFlow(false);
    });
}

void BroomButtons::setFlow(bool state){
    if (state == _broom->isFlowSelected())
        return;
    if (context->isCleaning())
        _broom->setFlowActive(state);
    _broom->selectFlow(state);
}

//==============================Обдув================================================
BlowerButtons::BlowerButtons(const OrganWidgets &w, Blower *blower, PhysicalButtonManager *pult,
                             MachineContext *context_, ViewController *view_, QObject *parent)
    : OrganButtons("Обдув в движении, ожидайте", blower, context_, view_, parent)
{
    // обдув вручную не двигаем: включённый обдув не может стоять в промежуточном положении.
    // Во время уборки кнопки срабатывают после удержания, дальше всё делает автомат
    const QString vert = buttonsPath + "purgeUnit_lift_";
    const QString side = buttonsPath + "purgeUnit_turn_";
    auto stop = [blower]{ blower->goNone(); };

    Button up;
    up.screen = w.up;
    up.physical = pultButton(pult, GPIOInput::IN_BLOW_UP);
    up.onlyWhenEnabled = true;
    up.icon = w.vertIcon;
    up.pressedIcon = vert + "up_on.png);";
    up.onRelease = stop;
    up.holdHint = []{ return QString("Удерживайте кнопку вверх для остановки обдува и подъёма"); };
    up.onHeld = [blower]{ blower->holdUp(); };
    addButton(up);

    Button down = up;
    down.screen = w.down;
    down.physical = pultButton(pult, GPIOInput::IN_BLOW_DOWN);
    down.pressedIcon = vert + "down_on.png);";
    down.holdHint = []{ return QString("Удерживайте кнопку вниз для опускания и запуска обдува"); };
    down.onHeld = [blower]{ blower->holdDown(); };
    addButton(down);

    Button left;
    left.screen = w.left;
    left.physical = pultButton(pult, GPIOInput::IN_BLOW_LEFT);
    left.icon = w.sideIcon;
    left.pressedIcon = side + "left_on.png);";
    left.onRelease = stop;
    left.holdHint = [blower]{
        return QString(blower->isRotating() ? "Удерживайте кнопку влево для смены направления обдува"
                                            : "Удерживайте кнопку влево для запуска обдува");
    };
    left.onHeld = [blower]{ blower->holdSide(false); };
    left.onSelect = [this, blower]{ toggleSide(blower, false, view, "Обдув"); };
    addButton(left);

    Button right = left;
    right.screen = w.right;
    right.physical = pultButton(pult, GPIOInput::IN_BLOW_RIGHT);
    right.pressedIcon = side + "right_on.png);";
    right.holdHint = [blower]{
        return QString(blower->isRotating() ? "Удерживайте кнопку вправо для смены направления обдува"
                                            : "Удерживайте кнопку вправо для запуска обдува");
    };
    right.onHeld = [blower]{ blower->holdSide(true); };
    right.onSelect = [this, blower]{ toggleSide(blower, true, view, "Обдув"); };
    addButton(right);

    // положение, к которому идёт автомат: иконка меняется сразу, как принята команда
    addIcon(w.vertIcon, [w, blower, vert]{
        if (!w.down->isEnabled())
            return vert + "blocked.png);";
        return vert + (blower->targetState() == Blower::BlowerOff ? "up_on.png);" : "down_on.png);");
    });
    // во время уборки - сторона, на которую обдув переходит, не дожидаясь реальной смены
    addIcon(w.sideIcon, [this, blower, side]{
        if (!blower->isSideSelected())
            return side + "off.png);";
        const bool right = context->isCleaning() ? blower->targetRight() : blower->isRightSelected();
        return side + (right ? "right_on.png);" : "left_on.png);");
    });
}
