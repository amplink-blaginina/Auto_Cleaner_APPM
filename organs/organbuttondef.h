#ifndef ORGANBUTTONDEF_H
#define ORGANBUTTONDEF_H
#pragma once
#include <QString>
#include "organsenums.h"
#include "gpio_types.hpp"// поправьте под ваш фактический header с GPIOInput

struct OrganButtonDef {
    organsEnums::Direction direction;
    GPIOInput input;
    QString buttonName;
    QString iconName;
    QString activeStyle;
    QString inactiveStyle;   // пусто для Left/Right
    bool dynamicInactive = false;
    QString mode;            // "flow" / "press" для немодальных кнопок
};

enum class OrganButtonAction {
    Direction,
    Flow,
    Press
};

struct OrganButtonDefinition {
    OrganButtonAction action = OrganButtonAction::Direction;
    organsEnums::Direction direction = organsEnums::None;
};

#endif // ORGANBUTTONDEF_H
