/*
 * Журнал работы — сборка данных отчётов (см. wjreport.h)
 * Версия: 01, 2026-09-29
 * Изменения: первая версия
 */
#include "wjreport.h"

namespace WJ
{

static const char* kWeekDays[] = {"", "пн", "вт", "ср", "чт", "пт", "сб", "вс"};
static const char* kMonths[] = {"", "Январь", "Февраль", "Март", "Апрель", "Май", "Июнь", "Июль", "Август",
                                "Сентябрь", "Октябрь", "Ноябрь", "Декабрь"};

PeriodSel PeriodSel::day(const QDate& d)          { PeriodSel s; s.kind = PerDay; s.from = s.to = d; return s; }
PeriodSel PeriodSel::week(const QDate& last)      { PeriodSel s; s.kind = PerWeek; s.to = last; s.from = last.addDays(-6); return s; }
PeriodSel PeriodSel::month(const QDate& d)
{
    PeriodSel s;
    s.kind = PerMonth;
    s.from = QDate(d.year(), d.month(), 1);
    s.to = s.from.addMonths(1).addDays(-1);
    return s;
}
PeriodSel PeriodSel::custom(const QDate& a, const QDate& b)
{
    PeriodSel s;
    s.kind = PerCustom;
    s.from = a;
    s.to = b < a ? a : b;
    if (s.days() > kMaxDays)
        s.to = s.from.addDays(kMaxDays - 1);
    return s;
}

PeriodSel PeriodSel::shifted(int dir) const
{
    switch (kind)
    {
    case PerDay:   return day(from.addDays(dir));
    case PerWeek:  return week(to.addDays(7 * dir));
    case PerMonth: return month(from.addMonths(dir));
    default:       return *this;
    }
}

QString PeriodSel::title() const
{
    switch (kind)
    {
    case PerDay:   return from.toString("dd.MM.yyyy") + " " + QString::fromUtf8(kWeekDays[from.dayOfWeek()]);
    case PerMonth: return QString::fromUtf8(kMonths[from.month()]) + " " + QString::number(from.year());
    default:       return from.toString("dd.MM") + QString::fromUtf8(" — ") + to.toString("dd.MM.yyyy");
    }
}

QString dayLabel(const QDate& d)
{
    return d.toString("dd.MM") + " " + QString::fromUtf8(kWeekDays[d.dayOfWeek()]);
}

QString shiftLabel(const ShiftSlot& s)
{
    return QString::fromUtf8("%1 · %2–%3").arg(s.no).arg(s.begin.toString("HH:mm"), s.end.toString("HH:mm"));
}

ReportBuilder::ReportBuilder(Store& s) : store(s)
{
    sched = store.schedules();
    st = store.settings();
}

ShiftReport ReportBuilder::shift(const ShiftSlot& slot, const QDateTime& now)
{
    ShiftReport r;
    r.slot = slot;
    r.st = st;
    r.open = now >= slot.begin && now < slot.end;
    r.dataAt = r.open ? now : slot.end;
    const qint64 b = slot.begin.toSecsSinceEpoch();
    const qint64 e = slot.end.toSecsSinceEpoch();
    r.minutes = store.minutes(b, e);
    r.events = store.events(b, e);
    r.cycles = store.cycles(b, e);
    r.tot = aggregate(r.minutes, r.events, b, e);
    return r;
}

PeriodReport ReportBuilder::period(const PeriodSel& sel)
{
    PeriodReport r;
    r.sel = sel;
    r.st = st;
    r.byShift = sel.kind == PerDay;
    r.begin = sched.dayBegin(sel.from);
    r.end = sched.dayEnd(sel.to);
    const qint64 b = r.begin.toSecsSinceEpoch();
    const qint64 e = r.end.toSecsSinceEpoch();
    const QVector<MinuteRec> mins = store.minutes(b, e);
    const QVector<EventRec> evs = store.events(b, e);

    if (r.byShift)
    {
        for (const ShiftSlot& s : sched.shiftsOfDay(sel.from))
        {
            PeriodRow row;
            row.label = shiftLabel(s);
            row.day = s.day;
            row.shiftNo = s.no;
            row.tot = aggregate(mins, evs, s.begin.toSecsSinceEpoch(), s.end.toSecsSinceEpoch());
            r.rows.append(row);
        }
    }
    else
    {
        for (QDate d = sel.from; d <= sel.to; d = d.addDays(1))
        {
            PeriodRow row;
            row.label = dayLabel(d);
            row.day = d;
            row.tot = aggregate(mins, evs, sched.dayBegin(d).toSecsSinceEpoch(), sched.dayEnd(d).toSecsSinceEpoch());
            r.rows.append(row);
        }
    }
    for (const PeriodRow& row : r.rows)
        if (row.tot.secKey > 0)
            ++r.daysWithWork;
    r.total = aggregate(mins, evs, b, e);
    for (const EventRec& ev : evs)
        if (ev.type == EvRefuel || ev.type == EvFuelDrain)
            r.fuelEvents.append(ev);
    return r;
}

} // namespace WJ
