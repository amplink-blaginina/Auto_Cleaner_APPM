#ifndef BACKMAGNET_H
#define BACKMAGNET_H

#include "organ.h"

#include <QMap>

class BackMagnet : public Organ
{
    Q_OBJECT
public:
    // коды состояний движка (OrganSequence): шаг «опускание»
    enum BackMagnetStates
    {
        BackMagnetOff        = 0,
        BackMagnetDownIn     = 1,
        BackMagnetDownOut    = 2,
        BackMagnetDowned     = 3
    };
    Q_ENUM(BackMagnetStates)

    explicit BackMagnet(const MachineIo &machine, MachineContext *context, ViewController *logger, QObject *parent);

    void readSettings() override;

    BackMagnetStates getState() const { return BackMagnetStates(sequence.state()); }
    void setNeedState(BackMagnetStates state_) { sequence.setNeed(state_); }
    QString toString(BackMagnetStates s);

    void goOff();
    void goUp();
    void goDown();

private:
    // таймауты на каждую длительную операцию
    QMap<BackMagnetStates, float> timeouts;
};

#endif // BACKMAGNET_H
