/*
 * Журнал работы — сборка данных отчётов «за смену» и «за период» из хранилища (для экрана и PDF)
 * Версия: 01, 2026-09-29
 * Изменения: первая версия
 */
#ifndef WJREPORT_H
#define WJREPORT_H

#include "wjtypes.h"
#include "wjcalc.h"
#include "wjstore.h"

namespace WJ
{

enum PeriodKind { PerDay = 0, PerWeek = 1, PerMonth = 2, PerCustom = 3 };

struct PeriodSel
{
    PeriodKind kind = PerWeek;
    QDate from, to;                        // отчётные сутки включительно

    static PeriodSel day(const QDate& d);
    static PeriodSel week(const QDate& last);          // 7 суток, заканчивая last
    static PeriodSel month(const QDate& anyDay);
    static PeriodSel custom(const QDate& a, const QDate& b);
    PeriodSel shifted(int dir) const;                  // ◀ ▶ для СУТКИ / 7 ДНЕЙ / МЕСЯЦ
    int  days() const { return int(from.daysTo(to)) + 1; }
    QString title() const;                             // «28.09.2026» / «23.09 — 29.09.2026» / «Сентябрь 2026»
    static const int kMaxDays = 92;
};

struct ShiftReport
{
    ShiftSlot          slot;
    bool               open = false;       // смена ещё идёт
    QDateTime          dataAt;             // «данные на»
    Settings           st;
    Totals             tot;
    QVector<MinuteRec> minutes;
    QVector<EventRec>  events;
    QVector<CycleRec>  cycles;
};

struct PeriodRow
{
    QString label;                         // «23.09 ср» / «1 · 08:00–20:00»
    QDate   day;
    int     shiftNo = 0;                   // 0 — строка за сутки
    Totals  tot;
};

struct PeriodReport
{
    PeriodSel          sel;
    bool               byShift = false;    // строки по сменам (СУТКИ)
    Settings           st;
    QVector<PeriodRow> rows;
    Totals             total;
    QVector<EventRec>  fuelEvents;         // заправки и сливы
    int                daysWithWork = 0;
    QDateTime          begin, end;
};

class ReportBuilder
{
public:
    explicit ReportBuilder(Store& store);
    const ScheduleSet& schedules() const { return sched; }
    ShiftReport  shift(const ShiftSlot& slot, const QDateTime& now);
    PeriodReport period(const PeriodSel& sel);

private:
    Store&      store;
    ScheduleSet sched;
    Settings    st;
};

QString dayLabel(const QDate& d);          // «23.09 ср»
QString shiftLabel(const ShiftSlot& s);    // «1 · 08:00–20:00»

} // namespace WJ

#endif // WJREPORT_H
