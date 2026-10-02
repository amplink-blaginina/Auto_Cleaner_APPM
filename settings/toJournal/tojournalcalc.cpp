/*
 * Журнал ТО — расчёты
 * Версия: 01, 2026-09-28
 */
#include "tojournalcalc.h"

#include <QtMath>
#include <algorithm>
#include <climits>

namespace ToJ
{

Calc::Calc(const QVector<Procedure>& procedures, const QVector<Record>& records, double startHours)
    : procs(procedures), recs(records), start(startHours)
{
    for (const Procedure& p : procs)
    {
        if (p.interval > 0 && !intervals.contains(p.interval))
            intervals.append(p.interval);
    }
    std::sort(intervals.begin(), intervals.end());
}

int Calc::dueOf(const Work& work, int interval)
{
    if (interval <= 0)
        return kNoValue;
    if (work.lastDone != kNoValue)
    {// следующий кратный интервалу рубеж после выполнения
        return (work.lastDone / interval + 1) * interval;
    }
    // ни разу не выполнялась — первый кратный рубеж не раньше момента создания
    int due = int(qCeil(work.anchorHours / interval - 1e-9)) * interval;
    return qMax(due, interval);
}

QVector<DueWork> Calc::dueWorks() const
{
    QVector<DueWork> out;
    for (const Procedure& p : procs)
    {
        if (p.interval <= 0)
            continue;
        for (const Work& w : p.works)
        {
            DueWork d;
            d.work = w;
            d.interval = p.interval;
            d.procedureName = p.name;
            d.procedureSort = p.sort;
            d.due = dueOf(w, p.interval);
            out.append(d);
        }
    }
    std::stable_sort(out.begin(), out.end(), [](const DueWork& a, const DueWork& b)
    {
        return a.due < b.due;
    });
    return out;
}

int Calc::nextGridPointAfter(int x) const
{
    if (intervals.isEmpty())
        return kNoValue;
    int best = INT_MAX;
    for (int i : intervals)
    {
        int m = (x >= 0) ? (x / i + 1) * i : i;
        best = qMin(best, m);
    }
    return best;
}

int Calc::lastRecordedMilestone() const
{
    int last = kNoValue;
    for (const Record& r : recs)
        last = qMax(last, r.milestone);
    return last;
}

int Calc::firstCandidate() const
{
    int last = lastRecordedMilestone();
    if (last != kNoValue)
        return nextGridPointAfter(last);
    // записей нет — первый рубеж не раньше начала журнала
    return nextGridPointAfter(int(qCeil(start - 1e-9)) - 1);
}

int Calc::plannedMilestone(double hours) const
{
    if (intervals.isEmpty())
        return kNoValue;
    int m = firstCandidate();
    // рубежи, просроченные более чем на 10 м/ч, уже «пропуск» — план переходит на следующий
    while (m + kLateLimit < hours)
        m = nextGridPointAfter(m);
    return m;
}

QVector<int> Calc::milestoneOptions(double hours) const
{
    QVector<int> out;
    int planned = plannedMilestone(hours);
    if (planned == kNoValue)
        return out;
    // пропущенные рубежи (можно записать ТО задним числом), не больше 4 последних
    QVector<int> missed;
    for (int m = firstCandidate(); m < planned; m = nextGridPointAfter(m))
        missed.append(m);
    if (missed.size() > 4)
        missed = missed.mid(missed.size() - 4);
    out += missed;
    out.append(planned);
    int m = planned;
    for (int i = 0; i < 2; ++i)
    {
        m = nextGridPointAfter(m);
        out.append(m);
    }
    return out;
}

QVector<DueWork> Calc::checklist(int milestone) const
{
    QVector<DueWork> out;
    if (milestone == kNoValue)
        return out;
    for (const DueWork& d : dueWorks())
    {
        if (d.due <= milestone)
            out.append(d);
    }
    std::stable_sort(out.begin(), out.end(), [](const DueWork& a, const DueWork& b)
    {
        if (a.work.category != b.work.category)
            return a.work.category < b.work.category;
        if (a.procedureSort != b.procedureSort)
            return a.procedureSort < b.procedureSort;
        return a.work.sort < b.work.sort;
    });
    return out;
}

QVector<Work> Calc::oneTimeWorks() const
{
    QVector<Work> out;
    for (const Procedure& p : procs)
    {
        if (p.interval == 0)
            out += p.works;
    }
    return out;
}

void Calc::countStats(double hours, int& inTime, int& missed) const
{
    inTime = 0;
    missed = 0;
    if (intervals.isEmpty())
    {// без процедур — считаем только записи по рубежам
        for (const Record& r : recs)
        {
            if (r.milestone == kNoValue)
                continue;
            (r.hours <= r.milestone + kLateLimit) ? ++inTime : ++missed;
        }
        return;
    }
    int top = qMax(int(hours), lastRecordedMilestone());
    int guard = 0;
    for (int m = nextGridPointAfter(int(qCeil(start - 1e-9)) - 1); m <= top && guard < 20000; m = nextGridPointAfter(m), ++guard)
    {
        const Record* rec = nullptr;
        for (const Record& r : recs)
        {
            if (r.milestone == m && (rec == nullptr || r.hours < rec->hours))
                rec = &r;   // по рубежу берём самое раннее ТО
        }
        if (rec != nullptr)
            (rec->hours <= m + kLateLimit) ? ++inTime : ++missed;
        else if (m + kLateLimit < hours)
            ++missed;
    }
    // записи по рубежам, которых нет в текущей сетке (интервалы меняли)
    for (const Record& r : recs)
    {
        if (r.milestone == kNoValue || r.milestone < start)
            continue;
        bool onGrid = false;
        for (int i : intervals)
            onGrid = onGrid || (r.milestone % i == 0);
        if (!onGrid)
            (r.hours <= r.milestone + kLateLimit) ? ++inTime : ++missed;
    }
}

double Calc::ratePerDay(const QVector<HoursSample>& samples, double hours, const QDate& today)
{
    // опорная точка — самая ранняя за последние 30 суток, но не сегодняшняя
    const HoursSample* ref = nullptr;
    for (const HoursSample& s : samples)
    {
        qint64 age = s.day.daysTo(today);
        if (age < 1 || age > 30)
            continue;
        if (ref == nullptr || s.day < ref->day)
            ref = &s;
    }
    if (ref == nullptr)
        return -1;
    qint64 days = ref->day.daysTo(today);
    double rate = (hours - ref->hours) / double(days);
    return rate < 0 ? -1 : rate;
}

int Calc::daysTo(double remaining, double rate)
{
    if (rate <= 0.01)
        return kNoValue;
    if (remaining <= 0)
        return 0;
    return int(qCeil(remaining / rate));
}

QString plural(int n, const QString& one, const QString& few, const QString& many)
{
    int a = qAbs(n) % 100;
    int b = a % 10;
    if (a > 10 && a < 20)
        return many;
    if (b == 1)
        return one;
    if (b >= 2 && b <= 4)
        return few;
    return many;
}

} // namespace ToJ
