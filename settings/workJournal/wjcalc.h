/*
 * Журнал работы — расчёты без UI и БД: смены и отчётные сутки, режим секунды, сборка минуты,
 * заправки и сливы топлива, итоги за интервал, форматирование чисел
 * Версия: 02, 2026-09-29
 * Изменения от 01: checkKeyOffJump с поправкой на расход по LFC
 * Платформа: Qt 5.15 (core). Проверено юнит-тестами tests/tst_workjournal (x86, Qt 5.15.13)
 */
#ifndef WJCALC_H
#define WJCALC_H

#include "wjtypes.h"
#include <QVector>
#include <QList>
#include <deque>

namespace WJ
{

// ---------- смены ----------
class ScheduleSet
{
public:
    ScheduleSet();                                  // по умолчанию 2 смены 08:00 / 20:00
    explicit ScheduleSet(const QVector<Schedule>& list);
    const Schedule& forDay(const QDate& day) const; // график, действующий в отчётные сутки day
    const QVector<Schedule>& all() const { return list; }

    QDateTime dayBegin(const QDate& day) const;     // начало отчётных суток (локальное время)
    QDateTime dayEnd(const QDate& day) const;       // = начало следующих суток
    QDate     reportDay(const QDateTime& local) const;
    QVector<ShiftSlot> shiftsOfDay(const QDate& day) const;
    ShiftSlot shiftAt(const QDateTime& local) const;
    ShiftSlot nextShift(const ShiftSlot& s) const;
    ShiftSlot prevShift(const ShiftSlot& s) const;

private:
    QVector<Schedule> list;   // по возрастанию fromDay; первый действует «всегда до»
};

// ---------- режим секунды ----------
struct Sample
{
    bool   keyOn = true;
    bool   cleaning = false;       // цикл уборки запущен и не на паузе
    bool   ecuOnline = false;      // EEC1 был не позже 5 с назад
    double rpm = noValue();
    double speed = noValue();      // км/ч
    double fuelRateLh = noValue(); // LFE, л/ч
    double coolant = noValue();
    double oilP = noValue();       // кПа
    double hydro = noValue();      // °C
    bool   eq[EqCount] = {};
    double engineH = noValue();
    double ecuH = noValue();
    double odoKm = noValue();
    double lfcL = noValue();
    double fuelLevel = noValue();  // сглаженный уровень, %
};

int modeOf(const Sample& s, const Settings& st);

// Сборка минутной записи из секундных отсчётов
class MinuteBuilder
{
public:
    void   reset(qint64 minuteStart);
    void   add(const Sample& s, const Settings& st, double dtSec = 1.0);
    bool   empty() const { return count == 0; }
    qint64 minuteStart() const { return rec.t; }
    MinuteRec finish();          // вернуть готовую минуту (средние посчитаны)

private:
    MinuteRec rec;
    int    count = 0;
    double rpmSum = 0; int rpmN = 0;
    double spdSum = 0; int spdN = 0;
    double hydSum = 0; int hydN = 0;
};

// ---------- топливо ----------
// Сглаживание уровня: медиана за окно (только отсчёты на стоянке)
class LevelFilter
{
public:
    explicit LevelFilter(int windowSec = 60) : window(windowSec) {}
    void   add(qint64 t, double levelPct, bool stationary);
    double value() const;           // медиана; NaN — мало отсчётов
    void   clear() { buf.clear(); }
private:
    int window;
    std::deque<QPair<qint64, double>> buf;
};

// Обнаружение заправок и сливов по сглаженному уровню и расходу LFE
class FuelDetector
{
public:
    struct Found
    {
        bool   refuel = true;
        qint64 tStart = 0, tEnd = 0;
        double levelBefore = 0, levelAfter = 0;   // %
        double liters = 0;
        bool   keyOff = false;                    // обнаружено сравнением «выключение → включение»
    };

    void configure(const Settings& st) { this->st = st; }
    // t — секунды; level — сглаженный уровень (NaN — нет); fuelUsedL — нарастающий расход по LFE, л
    QVector<Found> feed(qint64 t, double level, double fuelUsedL);
    // Сравнение уровня при включении с уровнем при последнем выключении
    // usedOffL — расход за время простоя системы по LFC шасси (вычитается из падения уровня); NaN — неизвестен
    QVector<Found> checkKeyOffJump(qint64 tOff, double levelOff, qint64 tOn, double levelOn, double usedOffL = 0) const;
    // Выключение ключа: закрыть незавершённое событие и начать историю заново
    QVector<Found> close(qint64 t, double fuelUsedL);
    void reset() { hist.clear(); pending = false; }

private:
    Found finalize(qint64 t, double used);
    struct Pt { qint64 t; double level; double used; };
    Settings st;
    std::deque<Pt> hist;
    bool   pending = false;     // идёт событие (уровень ещё меняется)
    Found  cur;
    qint64 lastChange = 0;
};

// ---------- итоги ----------
bool   isEndMark(const EventRec& e);   // запись о снятии/восстановлении (не считается событием)
Totals aggregate(const QVector<MinuteRec>& minutes, const QVector<EventRec>& events, qint64 begin, qint64 end);

// ---------- форматирование ----------
QString fmtHM(int seconds);                         // «7:40»
QString fmtNum(double v, int decimals = 1);         // «63,1», «—» для NaN
QString fmtInt(double v);                           // «1 420»
QString fmtPct(double part, double whole);          // «76 %» / «—»
QString eventText(const EventRec& e);               // строка для ленты событий
QString modeName(int mode);
QString filterName(int id);

} // namespace WJ

#endif // WJCALC_H
