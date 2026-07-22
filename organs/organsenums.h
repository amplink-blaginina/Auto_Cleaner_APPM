#ifndef ORGANSENUMS_H
#define ORGANSENUMS_H

class organsEnums
{
public:
    enum Direction {
        None = 0,
        Up = 1,
        Down = 2,
        Left = 3,
        Right = 4,
    };

    enum Organ{
        BroomOrgan,
        BlowerOrgan,
        DumpOrgan
    };

    organsEnums();
};

#endif // ORGANSENUMS_H
