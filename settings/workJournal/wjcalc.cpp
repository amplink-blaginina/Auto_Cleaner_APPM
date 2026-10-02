/*
 * Журнал работы — расчёты без UI и БД (см. wjcalc.h)
 * Версия: 03, 2026-09-30
 * Изменения от 02: «Движение» — по скорости шасси, раньше проверки ДВС надстройки
 * Изменения от 01: работа шасси без системы в итогах и балансе; поправка уровня на LFC
 */
#include "wjcalc.h"
#include <QtMath>
#include <algorithm>

namespace WJ
{

// ============================ типы ============================
int MinuteRec::dominantMode() const
{
    int best = ModeKeyOff, bestSec = 0;
    for (int i = 0; i < ModeCount; ++i)
        if (secMode[i] > bestSec) { bestSec = secMode[i]; best = i; }
    return best;
}

bool Schedule::valid() const
{
    if (count < 1 || count > 3)
        return false;
    for (int i = 0; i < count; ++i)
        if (start[i] < 0 || start[i] >= 1440)
            return false;
    for (int i = 1; i < count; ++i)
        if (offset(i) <= offset(i - 1))
            return false;
    return true;
}

double Totals::fuelPerCleanHour() const
{
    if (!hasValue(fuelCleanL) || secMode[ModeClean] < 60)
        return noValue();
    return fuelCleanL / (secMode[ModeClean] / 3600.0);
}

double Totals::residualL(double tankL) const
{
    if (!hasValue(levelStart) || !hasValue(levelEnd) || !hasValue(fuelL))
        return noValue();
    return (levelStart - levelEnd) / 100.0 * tankL + refuelL - drainL - fuelL - offFuelL;
}

// ============================ смены ============================
ScheduleSet::ScheduleSet()
{
    list.append(Schedule());
}

ScheduleSet::ScheduleSet(const QVector<Schedule>& l) : list(l)
{
    std::sort(list.begin(), list.end(), [](const Schedule& a, const Schedule& b)
    {
        if (!a.fromDay.isValid()) return b.fromDay.isValid();
        if (!b.fromDay.isValid()) return false;
        return a.fromDay < b.fromDay;
    });
    QVector<Schedule> ok;
    for (const Schedule& s : list)
        if (s.valid())
            ok.append(s);
    list = ok;
    if (list.isEmpty() || list.first().fromDay.isValid())
        list.prepend(Schedule());   // до первой записи — график по умолчанию
}

const Schedule& ScheduleSet::forDay(const QDate& day) const
{
    int idx = 0;
    for (int i = 0; i < list.size(); ++i)
        if (!list[i].fromDay.isValid() || list[i].fromDay <= day)
            idx = i;
    return list[idx];
}

QDateTime ScheduleSet::dayBegin(const QDate& day) const
{
    const Schedule& s = forDay(day);
    return QDateTime(day, QTime(0, 0)).addSecs(qint64(s.start[0]) * 60);
}

QDateTime ScheduleSet::dayEnd(const QDate& day) const
{
    return dayBegin(day.addDays(1));
}

QDate ScheduleSet::reportDay(const QDateTime& local) const
{
    QDate d = local.date();
    if (local < dayBegin(d))
        d = d.addDays(-1);
    else if (local >= dayEnd(d))
        d = d.addDays(1);
    return d;
}

QVector<ShiftSlot> ScheduleSet::shiftsOfDay(const QDate& day) const
{
    QVector<ShiftSlot> out;
    const Schedule& s = forDay(day);
    QDateTime b = dayBegin(day);
    QDateTime e = dayEnd(day);
    for (int i = 0; i < s.count; ++i)
    {
        ShiftSlot slot;
        slot.day = day;
        slot.no = i + 1;
        slot.begin = b.addSecs(qint64(s.offset(i)) * 60);
        slot.end = (i + 1 < s.count) ? b.addSecs(qint64(s.offset(i + 1)) * 60) : e;
        if (slot.end > e)
            slot.end = e;
        out.append(slot);
    }
    return out;
}

ShiftSlot ScheduleSet::shiftAt(const QDateTime& local) const
{
    QDate d = reportDay(local);
    const QVector<ShiftSlot> slots = shiftsOfDay(d);
    for (const ShiftSlot& s : slots)
        if (local >= s.begin && local < s.end)
            return s;
    return slots.isEmpty() ? ShiftSlot() : slots.last();
}

ShiftSlot ScheduleSet::nextShift(const ShiftSlot& s) const
{
    QVector<ShiftSlot> slots = shiftsOfDay(s.day);
    if (s.no < slots.size())
        return slots[s.no];
    return shiftsOfDay(s.day.addDays(1)).first();
}

ShiftSlot ScheduleSet::prevShift(const ShiftSlot& s) const
{
    if (s.no > 1)
    {
        QVector<ShiftSlot> slots = shiftsOfDay(s.day);
        if (s.no - 2 < slots.size())
            return slots[s.no - 2];
    }
    return shiftsOfDay(s.day.addDays(-1)).last();
}

// ============================ режим и минута ============================
int modeOf(const Sample& s, const Settings& st)
{
    // Движение определяется скоростью шасси и не зависит от ДВС надстройки (в транспортном режиме он может быть заглушён)
    if (s.cleaning)
        return ModeClean;
    if (hasValue(s.speed) && s.speed > st.speedMove)
        return ModeMove;
    if (!s.ecuOnline)
        return ModeNoLink;
    if (!hasValue(s.rpm) || s.rpm <= st.rpmOn)
        return ModeOff;
    return ModeIdle;
}

static void maxTo(double& dst, double v) { if (hasValue(v) && (!hasValue(dst) || v > dst)) dst = v; }
static void minTo(double& dst, double v) { if (hasValue(v) && (!hasValue(dst) || v < dst)) dst = v; }
static void addTo(double& dst, double v) { if (hasValue(v)) dst = hasValue(dst) ? dst + v : v; }
static void lastTo(double& dst, double v) { if (hasValue(v)) dst = v; }

void MinuteBuilder::reset(qint64 minuteStart)
{
    rec = MinuteRec();
    rec.t = minuteStart;
    count = 0;
    rpmSum = spdSum = hydSum = 0;
    rpmN = spdN = hydN = 0;
}

void MinuteBuilder::add(const Sample& s, const Settings& st, double dtSec)
{
    if (!s.keyOn)
        return;
    const int dt = qMax(1, qRound(dtSec));
    const int m = modeOf(s, st);
    ++count;
    rec.secMode[m] += dt;
    rec.secKey += dt;
    for (int i = 0; i < EqCount; ++i)
        if (s.eq[i])
            rec.secEq[i] += dt;

    if (hasValue(s.rpm)) { rpmSum += s.rpm; ++rpmN; maxTo(rec.rpmMax, s.rpm); }
    if (hasValue(s.speed))
    {
        spdSum += s.speed; ++spdN; maxTo(rec.speedMax, s.speed);
        const double d = s.speed / 3.6 * dt;
        rec.distM += d;
        if (m == ModeClean)
            rec.distCleanM += d;
    }
    if (hasValue(s.fuelRateLh))
    {
        const double l = s.fuelRateLh / 3600.0 * dt;
        addTo(rec.fuelL, l);
        addTo(rec.fuelCleanL, m == ModeClean ? l : 0.0);
    }
    maxTo(rec.coolantMax, s.coolant);
    if (hasValue(s.coolant) && s.coolant > st.coolantOver)
        rec.coolantOverSec += dt;
    if (hasValue(s.rpm) && s.rpm > st.rpmOn)
        minTo(rec.oilPMin, s.oilP);
    if (hasValue(s.hydro))
    {
        hydSum += s.hydro; ++hydN; maxTo(rec.hydroMax, s.hydro);
        if (s.hydro > st.hydroOver)
            rec.hydroOverSec += dt;
    }
    if (!hasValue(rec.engineH0)) rec.engineH0 = s.engineH;
    lastTo(rec.engineH, s.engineH);
    lastTo(rec.ecuH, s.ecuH);
    lastTo(rec.odoKm, s.odoKm);
    lastTo(rec.lfcL, s.lfcL);
    lastTo(rec.fuelLevel, s.fuelLevel);
}

MinuteRec MinuteBuilder::finish()
{
    MinuteRec r = rec;
    if (rpmN) r.rpmAvg = rpmSum / rpmN;
    if (spdN) r.speedAvg = spdSum / spdN;
    if (hydN) r.hydroAvg = hydSum / hydN;
    return r;
}

// ============================ топливо ============================
void LevelFilter::add(qint64 t, double levelPct, bool stationary)
{
    while (!buf.empty() && buf.front().first <= t - window)
        buf.pop_front();
    if (stationary && hasValue(levelPct) && levelPct >= 0 && levelPct <= 100)
        buf.push_back(qMakePair(t, levelPct));
}

double LevelFilter::value() const
{
    if (buf.size() < 10)
        return noValue();
    std::vector<double> v;
    v.reserve(buf.size());
    for (const auto& p : buf)
        v.push_back(p.second);
    std::nth_element(v.begin(), v.begin() + v.size() / 2, v.end());
    return v[v.size() / 2];
}

FuelDetector::Found FuelDetector::finalize(qint64 t, double used)
{
    const double tank = qMax(1.0, st.tankL);
    cur.tEnd = lastChange;
    double usedDuring = 0;
    for (const Pt& p : hist)
        if (p.t >= cur.tStart) { usedDuring = used - p.used; break; }
    if (cur.refuel)
        cur.liters = (cur.levelAfter - cur.levelBefore) / 100.0 * tank + qMax(0.0, usedDuring);
    else
        cur.liters = qMax(0.0, (cur.levelBefore - cur.levelAfter) / 100.0 * tank - qMax(0.0, usedDuring));
    pending = false;
    hist.clear();
    Q_UNUSED(t);
    return cur;
}

QVector<FuelDetector::Found> FuelDetector::close(qint64 t, double used)
{
    QVector<Found> out;
    if (pending)
        out.append(finalize(t, used));
    hist.clear();
    return out;
}

QVector<FuelDetector::Found> FuelDetector::feed(qint64 t, double level, double used)
{
    QVector<Found> out;
    if (!hasValue(used))
        used = hist.empty() ? 0.0 : hist.back().used;
    if (!hasValue(level))
    {
        // уровня нет (движение) — событие закрывается, если уровень не менялся 2 мин
        if (pending && t - lastChange >= 120)
            out.append(finalize(t, used));
        return out;
    }
    const qint64 W = qint64(qMax(1, st.fuelWindowMin)) * 60;
    const double tank = qMax(1.0, st.tankL);

    hist.push_back({t, level, used});
    while (!hist.empty() && hist.front().t < t - W - 300)
        hist.pop_front();

    if (pending)
    {
        if (cur.refuel && level > cur.levelAfter + 0.5) { cur.levelAfter = level; lastChange = t; }
        if (!cur.refuel && level < cur.levelAfter - 0.5) { cur.levelAfter = level; lastChange = t; }
        if (t - lastChange >= 120)
        {
            out.append(finalize(t, used));
            hist.push_back({t, level, used});
        }
        return out;
    }

    // окно последних W секунд
    const Pt* minP = nullptr;
    const Pt* maxP = nullptr;
    for (const Pt& p : hist)
    {
        if (p.t < t - W)
            continue;
        if (!minP || p.level < minP->level) minP = &p;
        if (!maxP || p.level > maxP->level) maxP = &p;
    }
    if (minP && level - minP->level >= st.refuelPct)
    {
        pending = true;
        cur = Found();
        cur.refuel = true;
        cur.tStart = minP->t;
        cur.levelBefore = minP->level;
        cur.levelAfter = level;
        lastChange = t;
    }
    else if (maxP && maxP->level - level >= st.drainPct)
    {
        const double dropL = (maxP->level - level) / 100.0 * tank;
        const double usedSince = used - maxP->used;
        if (usedSince < 0.3 * dropL)
        {
            pending = true;
            cur = Found();
            cur.refuel = false;
            cur.tStart = maxP->t;
            cur.levelBefore = maxP->level;
            cur.levelAfter = level;
            lastChange = t;
        }
    }
    return out;
}

QVector<FuelDetector::Found> FuelDetector::checkKeyOffJump(qint64 tOff, double levelOff, qint64 tOn, double levelOn, double usedOffL) const
{
    QVector<Found> out;
    if (!hasValue(levelOff) || !hasValue(levelOn) || tOff <= 0 || tOn <= tOff)
        return out;
    const double tank = qMax(1.0, st.tankL);
    // изменение уровня без учёта того, что сжёг ДВС шасси, пока система была выключена
    const double diff = levelOn - levelOff + (hasValue(usedOffL) ? usedOffL / tank * 100.0 : 0.0);
    if (diff >= st.refuelPct || -diff >= st.drainPct)
    {
        Found f;
        f.refuel = diff > 0;
        f.tStart = tOff;
        f.tEnd = tOn;
        f.levelBefore = levelOff;
        f.levelAfter = levelOn;
        f.liters = qAbs(diff) / 100.0 * tank;
        f.keyOff = true;
        out.append(f);
    }
    return out;
}

// ============================ итоги ============================
bool isEndMark(const EventRec& e)
{
    return e.text == QString::fromUtf8("снят") || e.text == QString::fromUtf8("восстановлена");
}

Totals aggregate(const QVector<MinuteRec>& minutes, const QVector<EventRec>& events, qint64 begin, qint64 end)
{
    Totals r;
    r.begin = begin;
    r.end = end;
    double rpmW = 0, rpmSumW = 0;
    for (const MinuteRec& m : minutes)
    {
        if (m.t < begin || m.t >= end)
            continue;
        ++r.minutes;
        for (int i = 0; i < ModeCount; ++i)
            r.secMode[i] += m.secMode[i];
        r.secKey += m.secKey;
        for (int i = 0; i < EqCount; ++i)
            r.secEq[i] += m.secEq[i];
        r.distM += m.distM;
        r.distCleanM += m.distCleanM;
        addTo(r.fuelL, m.fuelL);
        addTo(r.fuelCleanL, m.fuelCleanL);
        maxTo(r.speedMax, m.speedMax);
        maxTo(r.rpmMax, m.rpmMax);
        if (hasValue(m.rpmAvg) && m.secKey > 0) { rpmSumW += m.rpmAvg * m.secKey; rpmW += m.secKey; }
        maxTo(r.coolantMax, m.coolantMax);
        minTo(r.oilPMin, m.oilPMin);
        maxTo(r.hydroMax, m.hydroMax);
        r.hydroOverSec += m.hydroOverSec;
        r.coolantOverSec += m.coolantOverSec;
        if (hasValue(m.engineH))
        {
            if (!hasValue(r.engineHStart)) r.engineHStart = hasValue(m.engineH0) ? m.engineH0 : m.engineH;
            r.engineHEnd = m.engineH;
        }
        if (hasValue(m.fuelLevel))
        {
            if (!hasValue(r.levelStart)) r.levelStart = m.fuelLevel;
            r.levelEnd = m.fuelLevel;
        }
        if (m.secKey > 0)
        {
            if (r.firstKeyOn == 0) r.firstKeyOn = m.t;
            r.lastKeyOn = m.t + 60;
        }
        if (m.timeInvalid)
            r.timeInvalid = true;
    }
    if (rpmW > 0)
        r.rpmAvg = rpmSumW / rpmW;
    if (r.secMode[ModeClean] > 0)
        r.speedCleanAvg = r.distCleanM / r.secMode[ModeClean] * 3.6;

    qint64 firstLevelT = 0;
    for (const MinuteRec& m : minutes)
        if (m.t >= begin && m.t < end && hasValue(m.fuelLevel)) { firstLevelT = m.t; break; }
    for (const EventRec& e : events)
    {
        if (e.t < begin || e.t >= end)
            continue;
        // заправка/слив при выключенном ключе обнаружена при включении: уровень на начало интервала —
        // уровень до события, иначе баланс учтёт событие дважды
        if ((e.type == EvRefuel || e.type == EvFuelDrain) && !e.keyOn && firstLevelT > 0 && e.t <= firstLevelT + 180 && hasValue(e.value2))
            r.levelStart = e.value2;
        if (e.type == EvOffRun)
        {
            r.offKm += hasValue(e.value) ? e.value : 0;
            r.offFuelL += hasValue(e.value2) ? e.value2 : 0;
            ++r.offCount;
            if (firstLevelT > 0 && e.t <= firstLevelT + 180 && hasValue(e.value3))
                r.levelStart = e.value3;
        }
        if (e.type == EvRefuel) { r.refuelL += hasValue(e.value) ? e.value : 0; ++r.refuelCount; }
        if (e.type == EvFuelDrain) { r.drainL += hasValue(e.value) ? e.value : 0; ++r.drainCount; }
        if (e.type == EvProgramStart || e.type == EvKeyOn || e.type == EvKeyOff || e.type == EvPauseOn || e.type == EvPauseOff
            || e.type == EvCleanStart || e.type == EvCleanStop || e.type == EvOffRun || isEndMark(e))
            continue;   // служебные и записи о снятии — не считаем
        ++r.events;
        if (e.severity == SevWarn) ++r.warnEvents;
        if (e.severity == SevAlarm) ++r.alarmEvents;
    }
    return r;
}

// ============================ форматирование ============================
QString fmtHM(int seconds)
{
    if (seconds < 0) seconds = 0;
    const int m = (seconds + 30) / 60;
    return QString("%1:%2").arg(m / 60).arg(m % 60, 2, 10, QChar('0'));
}

static QString groupThousands(const QString& intPart)
{
    QString s = intPart;
    bool neg = s.startsWith('-');
    if (neg) s.remove(0, 1);
    for (int i = s.size() - 3; i > 0; i -= 3)
        s.insert(i, QChar(0x00A0));
    return neg ? QChar(0x2212) + s : s;
}

QString fmtNum(double v, int decimals)
{
    if (!hasValue(v))
        return QString::fromUtf8("—");
    if (qAbs(v) < 0.5 * std::pow(10.0, -decimals))
        v = 0.0;   // без «-0,0»
    QString s = QString::number(v, 'f', decimals);
    int dot = s.indexOf('.');
    QString ip = dot < 0 ? s : s.left(dot);
    QString fp = dot < 0 ? QString() : s.mid(dot + 1);
    QString r = groupThousands(ip);
    if (!fp.isEmpty())
        r += ',' + fp;
    return r;
}

QString fmtInt(double v)
{
    return fmtNum(v, 0);
}

QString fmtPct(double part, double whole)
{
    if (!(whole > 0))
        return QString::fromUtf8("—");
    return QString::number(qRound(part / whole * 100.0)) + QString::fromUtf8(" %");
}

QString modeName(int mode)
{
    switch (mode)
    {
    case ModeClean:  return QString::fromUtf8("Уборка");
    case ModeMove:   return QString::fromUtf8("Движение");
    case ModeIdle:   return QString::fromUtf8("Холостой ход");
    case ModeOff:    return QString::fromUtf8("ДВС заглушён");
    case ModeNoLink: return QString::fromUtf8("Нет связи с ЭБУ");
    default:         return QString::fromUtf8("Ключ выключен");
    }
}

QString filterName(int id)
{
    switch (id)
    {
    case FltDrain:    return QString::fromUtf8("Засор сливного фильтра");
    case FltPress1:   return QString::fromUtf8("Засор напорного фильтра 1");
    case FltPress2:   return QString::fromUtf8("Засор напорного фильтра 2");
    case FltPress3:   return QString::fromUtf8("Засор напорного фильтра 3");
    case FltAir:      return QString::fromUtf8("Засор воздушного фильтра");
    case FltOil:      return QString::fromUtf8("Засор масляного фильтра");
    case FltWater:    return QString::fromUtf8("Датчик воды");
    case FltAlarmBtn: return QString::fromUtf8("Аварийная кнопка");
    default:          return QString::fromUtf8("Датчик");
    }
}

static QString dur(double sec)
{
    if (!hasValue(sec))
        return QString();
    const int s = qRound(sec);
    if (s < 60)
        return QString::fromUtf8(" (%1 с)").arg(s);
    return QString::fromUtf8(" (%1 мин)").arg((s + 30) / 60);
}

QString eventText(const EventRec& e)
{
    const bool end = isEndMark(e);
    switch (e.type)
    {
    case EvProgramStart:   return QString::fromUtf8("Запуск программы пульта");
    case EvKeyOn:          return QString::fromUtf8("Ключ включён");
    case EvKeyOff:         return QString::fromUtf8("Ключ выключен");
    case EvPviOff:         return QString::fromUtf8("Выключение кнопкой ПВИ");
    case EvCleanStart:     return QString::fromUtf8("Цикл уборки — старт");
    case EvCleanStop:      return QString::fromUtf8("Цикл уборки — стоп");
    case EvPauseOn:        return QString::fromUtf8("Уборка — пауза");
    case EvPauseOff:       return QString::fromUtf8("Уборка — пауза снята");
    case EvEcuLost:        return end ? QString::fromUtf8("Связь с ЭБУ восстановлена") + dur(e.value) : QString::fromUtf8("Нет связи с ЭБУ");
    case EvBucLost:        return end ? QString::fromUtf8("Связь с БУЦ восстановлена") + dur(e.value) : QString::fromUtf8("Нет связи с БУЦ");
    case EvDtc:
        return QString::fromUtf8("DTC SPN %1 FMI %2").arg(e.code).arg(hasValue(e.value) ? qRound(e.value) : 0)
               + (end ? QString::fromUtf8(" — снят") + dur(e.value2) : QString());
    case EvFilter:         return filterName(int(e.code)) + (end ? QString::fromUtf8(" — снят") + dur(e.value) : QString());
    case EvHydroOverheat:
        return end ? QString::fromUtf8("Перегрев гидросистемы снят, макс. %1 °C").arg(fmtNum(e.value, 0)) + dur(e.value2)
                   : QString::fromUtf8("Перегрев гидросистемы > %1 °C").arg(fmtNum(e.value, 0));
    case EvCoolantOverheat:
        return end ? QString::fromUtf8("Перегрев ОЖ снят, макс. %1 °C").arg(fmtNum(e.value, 0)) + dur(e.value2)
                   : QString::fromUtf8("Перегрев ОЖ > %1 °C").arg(fmtNum(e.value, 0));
    case EvRefuel:
        return QString::fromUtf8("Заправка %1 л (%2 → %3 %)").arg(fmtNum(e.value, 1), fmtNum(e.value2, 0), fmtNum(e.value3, 0))
               + (e.keyOn ? QString() : QString::fromUtf8(", ключ выкл."));
    case EvFuelDrain:
        return QString::fromUtf8("Слив %1 л (%2 → %3 %)").arg(fmtNum(e.value, 1), fmtNum(e.value2, 0), fmtNum(e.value3, 0))
               + (e.keyOn ? QString() : QString::fromUtf8(", ключ выкл."));
    case EvTimeInvalid:    return QString::fromUtf8("Системное время неверно (нет RTC/NTP)");
    case EvOffRun:
        return QString::fromUtf8("Шасси без системы управления: пробег %1 км, топливо %2 л, простой %3")
               .arg(fmtNum(e.value), fmtNum(e.value2), fmtHM(int(e.code)));
    default:               return e.text;
    }
}

} // namespace WJ
