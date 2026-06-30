#pragma once

enum class GPIOInput {
    IN_STARTER,
    IN_RESERVE1,
    IN_FRM_PULT, // обычный
    IN_FRM,
    IN_IGNITION_OFF, // обычный
    IN_OPEN_BUNKER, // обычный
    IN_CLOSE_BUNKER, // обычный
    IN_LIFT_BUNKER, // обычный
    IN_LOWER_BUNKER, // обычный
    IN_PULT_DOWN, // обычный
    IN_RESERVE2,
    IN_DUMP_RIGHT,
    IN_DUMP_LEFT,
    IN_PAUSE_HOME,
    IN_PRESS_DOWN, // обычный
    IN_GABARIT, // обычный
    IN_MAYAK, // обычный
    IN_BLOW_DOWN,
    IN_DUMP_DOWN,
    IN_DUMP_UP,
    IN_BROOM_LEFT,
    IN_BROOM_RIGHT,
    IN_MODE,  // обычный
    IN_BROOM_DOWN,
    IN_BROOM_UP,
    IN_BLOW_UP,
    IN_MODE_LEFT,
    IN_MODE_RIGHT,
    IN_BLOW_LEFT,
    IN_BLOW_RIGHT,
    IN_STARTCLEAN,
    IN_NONE = 255
};

enum class GPIOOutput {
    // выход для кручения стартера
    OUT_STARTER,
    OUT_STARTER_LIGHT,
    OUT_VENTILATION,
    OUT_MAYAK_LIGHT,
    OUT_PVI_TEMP,
};
