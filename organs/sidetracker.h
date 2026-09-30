#ifndef SIDETRACKER_H
#define SIDETRACKER_H

#include <QElapsedTimer>
#include <QtGlobal>

// Положение органа по горизонтали: 0 - левый упор, 1 - правый. Датчика положения нет: оцениваем по времени
// работы клапанов поворота, концевики сторон уточняют оценку. Середина (0.5) - граница левой и правой стороны.
class SideTracker
{
public:
    void setTravelSec(float sec) { _travelSec = sec; }// ход от упора до упора
    void update(bool movingLeft, bool movingRight, bool atLeft, bool atRight){
        const double dt = _clock.isValid() ? _clock.restart() / 1000.0 : 0;
        if (!_clock.isValid())
            _clock.start();
        if (atLeft)
            _position = 0;
        else if (atRight)
            _position = 1;
        else if (movingLeft != movingRight){
            const double delta = _travelSec > 0 ? dt / _travelSec : 1;
            _position = qBound(0.0, _position + (movingRight ? delta : -delta), 1.0);
        }
    }
    double position() const { return _position; }
    bool isLeft() const { return _position < 0.5; }

private:
    double _position = 1;// исходное положение - у правого упора (так орган возвращается домой)
    float _travelSec = 5;
    QElapsedTimer _clock;
};

#endif // SIDETRACKER_H
