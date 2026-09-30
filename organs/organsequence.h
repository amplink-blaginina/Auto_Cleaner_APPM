#ifndef ORGANSEQUENCE_H
#define ORGANSEQUENCE_H

#include <QElapsedTimer>
#include <QString>
#include <QVector>
#include <functional>

class ViewController;

// Общий движок органа. Орган описывается шагами (повернуть, опустить, раскрутить...). У каждого шага своё
// движение вперёд (к рабочему положению) и назад (к домашнему), поэтому опускание и подъём могут идти
// разными путями. Движение - одна или несколько фаз: что включить, чем подтверждается окончание
// (датчик или время) и сколько ждать. Датчик не сработал за тайм-аут - тревога, орган останавливается,
// последовательность идёт дальше.
//
// Коды состояний совпадают с enum органов: 0 - дома; шаг i (с нуля): 3i+1 - движение назад,
// 3i+2 - движение вперёд, 3i+3 - шаг выполнен (орган стоит).
class OrganSequence
{
public:
    struct Phase {
        QString name;                      // для журнала: "Щетка: подъём - достигнут датчик"
        QString log;                       // запись в журнал в начале фазы
        std::function<void()> start;       // что включить (перед этим движок останавливает орган)
        std::function<bool()> sensor;      // датчик окончания; без датчика фаза заканчивается по времени
        std::function<float()> timeoutSec; // сколько ждать, с; 0 или нет - фаза мгновенная
        std::function<bool()> skip;        // пропустить фазу (проверяется в её начале)
    };
    struct Step {
        QVector<Phase> out;         // вперёд, к рабочему положению
        QVector<Phase> in;          // назад, к домашнему; пусто - возврат мгновенный
        std::function<void()> done; // орган встал в конце шага (с любой стороны)
    };

    OrganSequence(const QString &organName, ViewController *logger);

    void addStep(const Step &step);
    void setHalt(std::function<void()> halt) { _halt = std::move(halt); }// остановить все движения органа
    void setHome(std::function<void()> home) { _home = std::move(home); }// орган встал в домашнее положение
    void setOnChange(std::function<void(int)> onChange) { _onChange = std::move(onChange); }

    int state() const { return _state; }
    int topState() const { return 3 * _steps.size(); }// всё выполнено - рабочее положение
    static bool isStable(int code) { return code % 3 == 0; }

    // куда хотим (выбор оператора) и куда можно (идёт ли уборка); идём к меньшему из двух
    void setNeed(int code) { _need = code; }
    int need() const { return _need; }
    void setAble(int code) { _able = code; }
    int able() const { return _able; }
    int targetState() const { return 3 * targetPosition(); }
    bool isTransitioning() const { return _state != targetState(); }
    bool isAlarmed() const { return _alarmed; }// датчик не сработал; сбрасывается при следующем выходе из дома

    // провести орган через положение (например, опустить обдув до низа, чтобы сменить сторону) и выполнить then
    void passThrough(int stableCode, std::function<void()> then);
    void cancelPassThrough();
    bool isPassingThrough() const { return _viaPosition >= 0; }

    void enter(int code);  // встать в состояние и выполнить его действия
    void assume(int stableCode);// считать, что орган в этом положении, ничего не включая
    void tick();           // вызывать по таймеру органа

private:
    int positionOf(int code) const;
    int targetPosition() const;
    const QVector<Phase> &currentMotion() const;
    float timeoutOf(const Phase &phase) const;
    void beginMotion(int step, bool forward);
    void advancePhase();
    bool phaseFinished();
    void enterStable(int position);
    void notify();

    QString _name;
    ViewController *_logger;
    QVector<Step> _steps;
    std::function<void()> _halt;
    std::function<void()> _home;
    std::function<void(int)> _onChange;

    int _state = 0;
    int _phase = -1;
    int _need = 0;
    int _able = 0;
    bool _alarmed = false;
    int _viaPosition = -1;
    std::function<void()> _viaThen;
    QElapsedTimer _clock;
};

#endif // ORGANSEQUENCE_H
