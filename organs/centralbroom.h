#ifndef CENTRALBROOM_H
#define CENTRALBROOM_H

#include "organ.h"
#include "organsenums.h"

#include <QElapsedTimer>
#include <QMap>

class CentralBroom : public Organ
{
    Q_OBJECT
public:
    // коды состояний движка (OrganSequence): шаги поворот, отскок, раскрутка, опускание, плавание
    enum BroomStates
    {
        BroomOff        = 0,
        BroomSlideIn    = 1,
        BroomSlideOut   = 2,
        BroomSlided     = 3,
        BroomBounceIn   = 4,
        BroomBounceOut  = 5,
        BroomBounced    = 6,
        BroomRotateIn   = 7,
        BroomRotateOut  = 8,
        BroomRotated    = 9,
        BroomDownIn     = 10,
        BroomDownOut    = 11,
        BroomDowned     = 12,
        BroomFlowIn     = 13,
        BroomFlowOut    = 14,
        BroomFlowed     = 15,
    };
    Q_ENUM(BroomStates);

    explicit CentralBroom(const MachineIo &machine, MachineContext *context, ViewController *logger, QObject *parent);

    void readSettings() override;

    BroomStates getState() const { return BroomStates(sequence.state()); }
    void setNeedState(BroomStates state_) { sequence.setNeed(state_); }
    QString toString(BroomStates s);

    bool needGoLeft = false; // в какую сторону поворачивать при опускании (true - влево)

    void setDirection(organsEnums::Direction dir);
    void setDirection(organsEnums::Direction dir, bool isPressed);
    void goPressUp(bool state);
    void goPressDown(bool state);
    void stopPress();
    void setPressActive(bool state);
    void setFlowActive(bool state);
    // выбор оператора: плавание (включается при работе органа)
    bool isFlowSelected() const { return _flowSelected; }
    void selectFlow(bool selected) { _flowSelected = selected; }

    void goUpImmediate(bool state);
    void goDownImmediate(bool);

signals:
    void flowCancelRequested();// орган двигают вверх/вниз - плавание надо снять

protected:
    void beforeStep() override;

private:
    void goLeft(bool state);
    void goRight(bool state);
    void goNone();
    void goUp(bool state, bool isPressed);
    void goDown(bool state, bool isPressed);
    void goFlow(bool state);
    void goRotate(int speed_);
    void goNoRotate();
    void goSlide(bool toLeft);

    void printMovement(organsEnums::Direction dir, bool state, bool isPressed);

    // вращение: опускается на поверхность только раскрученной, при подъёме останавливается
    bool shouldSpin() const;
    void updateRotation();
    // высота 0 - верх, 1 - низ. Датчика высоты нет: оцениваем по времени работы клапанов подъёма/опускания,
    // верхний концевик сбрасывает оценку. Времена хода берём не больше реальных - оценка ошибается в безопасную сторону
    void updateHeightEstimate();
    double heightEstimate = 0;
    float lowerTimeSec = 5;
    float raiseTimeSec = 5;
    float spinHeight = 0.8;// ниже - щётка должна крутиться, выше - стоять
    bool spinning = false;
    QElapsedTimer heightClock;

    bool isPressed = false;
    bool isFlowing = false;
    organsEnums::Direction direction = organsEnums::None;

    // таймауты на каждую длительную операцию
    QMap<BroomStates, float> timeouts;
    // скорость вращения щетки под каждый тип смета
    QMap<int, int> speedForSweepType;
    // обороты двигателя под каждый тип смета
    QMap<int, int> rpmForSweepType;
    bool _flowSelected = false;
};

#endif // CENTRALBROOM_H
