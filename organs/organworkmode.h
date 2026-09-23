#ifndef ORGANWORKMODE_H
#define ORGANWORKMODE_H
#include <QMetaType>

struct OrganWorkMode
{
    bool startClean = false;

    bool centralBroomLeft = false;
    bool centralBroomRight = false;
    bool centralBroomFlow = false;
    bool centralBroomPress = false;

    bool frontDumpLeft = false;
    bool frontDumpRight = false;
    bool frontDumpFlow = false;

    bool blowerLeft = false;
    bool blowerRight = false;

    bool backMagnet = false;

    int sweepType = 0;
};

Q_DECLARE_METATYPE(OrganWorkMode)

#endif // ORGANWORKMODE_H
