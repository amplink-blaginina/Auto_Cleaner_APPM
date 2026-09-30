#ifndef FRONTRAIL_H
#define FRONTRAIL_H

#include "organ.h"
#include "organsenums.h"

#include <QMap>

class FrontRail : public Organ
{
    Q_OBJECT
public:
    // коды состояний движка (OrganSequence): шаги поворот, отскок, опускание, плавание
    enum FrontRailStates
    {
        FrontRailOff        = 0,
        FrontRailSlideIn    = 1,
        FrontRailSlideOut   = 2,
        FrontRailSlided     = 3,
        FrontRailBounceIn   = 4,
        FrontRailBounceOut  = 5,
        FrontRailBounced    = 6,
        FrontRailDownIn     = 7,
        FrontRailDownOut    = 8,
        FrontRailDowned     = 9,
        FrontRailFlowIn     = 10,
        FrontRailFlowOut    = 11,
        FrontRailFlowed     = 12
    };
    Q_ENUM(FrontRailStates)
    explicit FrontRail(const MachineIo &machine, MachineContext *context, ViewController *logger, QObject *parent);

    void readSettings() override;

    FrontRailStates getState() const { return FrontRailStates(sequence.state()); }
    void setNeedState(FrontRailStates state_) { sequence.setNeed(state_); }
    QString toString(FrontRailStates s);

    bool needGoLeft = false; // в какую сторону поворачивать при опускании

    void setDirection(organsEnums::Direction dir);
    void goUp(bool state);// прямое управление клапаном (сервисный экран)
    void goDown(bool state);
    void setFlowActive(bool state);
    // выбор оператора: плавание (включается при работе органа)
    bool isFlowSelected() const { return _flowSelected; }
    void selectFlow(bool selected) { _flowSelected = selected; }

signals:
    void flowCancelRequested();// орган двигают вверх/вниз - плавание надо снять

private:
    void goLeft(bool state);
    void goRight(bool state);
    void goFlow(bool state);
    void printMovement(organsEnums::Direction dir, bool state);

    bool isFlowing = false;
    organsEnums::Direction direction = organsEnums::None;
    // таймауты на каждую длительную операцию
    QMap<FrontRailStates, float> timeouts;
    bool _flowSelected = false;
};

#endif // FRONTRAIL_H
