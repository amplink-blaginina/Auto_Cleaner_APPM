/*
 * Журнал работы — сборщик данных (см. wjcollector.h)
 * Версия: 03, 2026-09-30
 * Изменения от 02: скорость только от шасси (CCVS/EBC2 на speedBus=1), значение программы не используется
 * Изменения от 01: уровень DD и одометр — с шасси (can1), LFE/LFC — сумма обоих ДВС; снимок раз в минуту; при запуске/включении — пробег и расход шасси без системы, заправка/слив с поправкой на LFC
 *
 * Разбор J1939 (PGN, как их выдаёт MyCanJ1939: (id >> 8) & 0xFFFF):
 *   F004 EEC1  обороты  b3..4 × 0.125         FEEE ET1  ОЖ  b0 − 40             FEEF EFL/P1  давление масла b3 × 4 кПа
 *   FEF2 LFE   расход   b0..1 × 0.05 л/ч      FEE9 LFC  общий расход b4..7 × 0.5 л
 *   FEFC DD    уровень топлива b1 × 0.4 %     FEE5 HOURS  моточасы ЭБУ b0..3 × 0.05 ч
 *   FEF1 CCVS  скорость b1..2 / 256 км/ч      FEC1 VDHR  путь b0..3 × 5 м       FEE0 VD  путь b4..7 × 0.125 км
 *   FECA DM1   одиночный кадр: SPN = b2 | b3<<8 | (b4 & 0xE0)<<11, FMI = b4 & 0x1F (многокадровый BAM не разбирается)
 * Значения 0xFB..0xFF (0xFB00.. для 16 бит, 0xFB000000.. для 32 бит) — «нет данных».
 */
#include "wjcollector.h"
#include "wjstore.h"
#include <QThread>
#include <QDateTime>
#include <QDebug>

namespace WJ
{

static const QString kEnd = QString::fromUtf8("снят");
static const QString kRestored = QString::fromUtf8("восстановлена");

// ============================ Writer ============================
bool Writer::init()
{
    store = new Store;
    if (!store->open(path, "wj_writer"))
    {
        qDebug() << "WORK JOURNAL: БД не открыта:" << store->lastError();
        return false;
    }
    qDebug() << "WORK JOURNAL: БД" << path;
    return true;
}

Writer::~Writer()
{
    delete store;
}

// ============================ Collector ============================
Collector::Collector(const QString& dbPath, QObject* parent) : QObject(parent), path(dbPath)
{
    clock.start();
    thread = new QThread;
    thread->setObjectName("wj_writer");
    writer = new Writer(path);
    writer->moveToThread(thread);
    thread->start(QThread::LowPriority);

    bool okInit = false;
    Settings s;
    QString last;
    Writer* w = writer;
    QMetaObject::invokeMethod(writer, [w, &okInit, &s, &last]()
    {
        okInit = w->init();
        if (okInit)
        {
            s = w->store->settings();
            last = w->store->meta("snap");
        }
    }, Qt::BlockingQueuedConnection);
    started = okInit;
    st = s;
    fuel.configure(st);
    const QStringList p = last.split(';');
    if (p.size() == 5)
    {
        auto num = [](const QString& x) { bool ok = false; double v = x.toDouble(&ok); return ok ? v : noValue(); };
        snap.t = p[0].toLongLong();
        snap.level = num(p[1]);
        snap.odo = num(p[2]);
        snap.lfc = num(p[3]);
        snap.lfcAtLevel = num(p[4]);
    }
}

Collector::~Collector()
{
    flush(true);
    if (writer)
    {
        Writer* w = writer;
        QMetaObject::invokeMethod(writer, [w]() { delete w->store; w->store = nullptr; }, Qt::BlockingQueuedConnection);
    }
    thread->quit();
    thread->wait(3000);
    delete writer;
    delete thread;
}

void Collector::reloadSettings()
{
    Settings s;
    Writer* w = writer;
    QMetaObject::invokeMethod(writer, [w, &s]() { if (w->store) s = w->store->settings(); }, Qt::BlockingQueuedConnection);
    st = s;
    fuel.configure(st);
}

double Collector::fresh(const Val& x, qint64 maxAgeMs) const
{
    return clock.elapsed() - x.ms <= maxAgeMs ? x.v : noValue();
}

static quint32 u16(const QByteArray& d, int i) { return quint8(d[i]) | (quint32(quint8(d[i + 1])) << 8); }
static quint32 u32(const QByteArray& d, int i)
{
    return quint8(d[i]) | (quint32(quint8(d[i + 1])) << 8) | (quint32(quint8(d[i + 2])) << 16) | (quint32(quint8(d[i + 3])) << 24);
}

void Collector::onEngineFrame(quint32 pgn, quint8 sa, QByteArray data)
{
    Q_UNUSED(sa);
    parse(pgn, data, 0);
}

void Collector::onChassisFrame(quint32 pgn, quint8 sa, QByteArray data)
{
    Q_UNUSED(sa);
    parse(pgn, data, 1);
}

void Collector::parse(quint32 pgn, const QByteArray& d, int bus)
{
    if (d.size() < 8)
        return;
    const qint64 ms = clock.elapsed();
    auto set = [ms](Val& v, double x) { v.v = x; v.ms = ms; };
    switch (pgn & 0xFFFF)
    {
    case 0xF004:
        if (bus != 0) break;
        eec1Ms = ms;
        if (u16(d, 3) < 0xFB00) set(rpm, u16(d, 3) * 0.125);
        break;
    case 0xFEEE:
        if (bus == 0 && quint8(d[0]) < 0xFB) set(coolant, quint8(d[0]) - 40.0);
        break;
    case 0xFEEF:
        if (bus == 0 && quint8(d[3]) < 0xFB) set(oilP, quint8(d[3]) * 4.0);
        break;
    case 0xFEE5:
        if (bus == 0 && u32(d, 0) < 0xFB000000u) set(ecuH, u32(d, 0) * 0.05);
        break;
    case 0xFEF2:
        if (u16(d, 0) < 0xFB00) set(rate[bus & 1], u16(d, 0) * 0.05);
        break;
    case 0xFEE9:
        if (u32(d, 4) < 0xFB000000u) set(lfcV[bus & 1], u32(d, 4) * 0.5);
        break;
    case 0xFEFC:
        if (bus == levelBus && quint8(d[1]) < 0xFB) set(level, quint8(d[1]) * 0.4);
        break;
    case 0xFEF1:   // скорость — только с шины шасси (ДВС надстройки тоже шлёт CCVS, но это не скорость машины)
        if (bus == speedBus && u16(d, 1) < 0xFB00) set(ccvs, u16(d, 1) / 256.0);
        break;
    case 0xFEBF:   // EBC2, скорость передней оси — её же читает программа (CurrentState::vehicleSpeed)
        if (bus == speedBus && u16(d, 0) < 0xFB00) set(ebc2, u16(d, 0) / 256.0);
        break;
    case 0xFEC1:
        if (bus == odoBus && u32(d, 0) < 0xFB000000u) set(odo, u32(d, 0) * 0.005);
        break;
    case 0xFEE0:
        if (bus == odoBus && !hasValue(fresh(odo, 60000)) && u32(d, 4) < 0xFB000000u) set(odo, u32(d, 4) * 0.125);
        break;
    case 0xFECA:
    {
        if (bus != 0) break;
        const quint32 spn = quint8(d[2]) | (quint32(quint8(d[3])) << 8) | (quint32(quint8(d[4]) & 0xE0) << 11);
        const quint32 fmi = quint8(d[4]) & 0x1F;
        if (spn != 0 && spn != 0x7FFFF)
            dtcSeen.insert((spn << 5) | fmi, ms);
        break;
    }
    default:
        break;
    }
}

void Collector::push(EventRec e)
{
    if (e.t == 0)
        e.t = lastNow;
    if (e.mode < 0)
        e.mode = lastMode;
    pending.append(e);
}

void Collector::edge(bool& prev, bool now, int onType, int offType, int sev)
{
    if (now == prev)
        return;
    prev = now;
    EventRec e;
    e.type = now ? onType : offType;
    e.severity = sev;
    push(e);
}

void Collector::onKeyOff(bool byPvi)
{
    EventRec e;
    e.type = EvKeyOff;
    e.keyOn = false;
    push(e);
    cycleEnd = lastNow;
    cycleReason = byPvi ? QString::fromUtf8("ПВИ") : QString::fromUtf8("ключ");
    if (!gapPending && !gapLevelPending)
        saveSnap();                    // иначе снимок ещё не сверен — оставить прежний
    gapPending = true;                 // при следующем включении — сравнить со снимком
    gapLevelPending = false;
}

void Collector::tick(const Inputs& in)
{
    if (!started)
        return;
    const qint64 now = in.now;
    const qint64 ms = clock.elapsed();
    lastNow = now;
    const bool timeBad = QDateTime::fromSecsSinceEpoch(now).date().year() < 2026;

    // ---- граница минуты ----
    const qint64 mk = now - ((now % 60) + 60) % 60;
    if (builder.minuteStart() != mk)
    {
        writeMinute();
        builder.reset(mk);
    }

    // ---- отсчёт секунды ----
    Sample s;
    s.keyOn = in.keyOn;
    s.cleaning = in.cleanRunning && !in.paused;
    s.ecuOnline = ms - eec1Ms <= 5000;
    s.rpm = fresh(rpm);
    // скорость машины — только от шасси (CCVS или EBC2 на speedBus); нет кадров 3 с — скорости нет
    // (значение программы CurrentState::vehicleSpeed при пропаже шасси остаётся последним — его не берём)
    const double spdCcvs = fresh(ccvs, 3000);
    s.speed = hasValue(spdCcvs) ? spdCcvs : fresh(ebc2, 3000);
    {
        double r = noValue();
        for (int b = 0; b < 2; ++b)
            if (rateMask & (1 << b))
            {
                const double v = fresh(rate[b]);
                if (hasValue(v)) r = hasValue(r) ? r + v : v;
            }
        s.fuelRateLh = r;
    }
    s.coolant = fresh(coolant);
    s.oilP = fresh(oilP);
    s.hydro = in.hydro;
    for (int i = 0; i < EqCount; ++i)
        s.eq[i] = in.eq[i];
    s.engineH = in.engineH;
    s.ecuH = fresh(ecuH, 60000);
    s.odoKm = fresh(odo, 60000);
    s.lfcL = lfcSum();
    if (hasValue(s.odoKm)) odoHeld = s.odoKm;
    if (hasValue(s.lfcL)) lfcHeld = s.lfcL;
    const bool stationary = !hasValue(s.speed) || s.speed <= st.speedMove;
    levelFilter.add(now, fresh(level), stationary && in.keyOn);
    const double lvlNow = levelFilter.value();
    if (hasValue(lvlNow))
    {
        levelHeld = lvlNow;            // в движении — последний уровень на стоянке
        lfcAtLevel = lfcSum();
    }
    s.fuelLevel = levelHeld;
    builder.add(s, st);
    if (in.keyOn && hasValue(s.fuelRateLh))
        fuelUsed += s.fuelRateLh / 3600.0;

    // ---- события ----
    if (first)
    {
        first = false;
        EventRec e;
        e.type = EvProgramStart;
        e.keyOn = in.keyOn;
        push(e);
        keyPrev = in.keyOn;
        cleanPrev = in.cleanRunning;
        pausePrev = in.cleanRunning && in.paused;
        bucPrev = in.bucKnown;
        for (int i = 0; i < FltCount; ++i)
            fltPrev[i] = false;
        cycleEnd = now;
        cycleReason = QString::fromUtf8("нет данных");
        if (in.keyOn)
        {
            Writer* w = writer;
            QMetaObject::invokeMethod(writer, [w, now]() { if (w->store) w->cycleId = w->store->beginCycle(now); }, Qt::QueuedConnection);
        }
        lastPurge = now - 86400 + 600;   // первая очистка — через 10 мин после запуска
        gapSince = now;
    }
    if (timeBad && !timeInvalidLogged)
    {
        timeInvalidLogged = true;
        EventRec e;
        e.type = EvTimeInvalid;
        e.severity = SevWarn;
        push(e);
    }

    // ключ
    if (in.keyOn != keyPrev)
    {
        keyPrev = in.keyOn;
        if (in.keyOn)
        {
            EventRec e;
            e.type = EvKeyOn;
            push(e);
            gapPending = true;
            gapSince = now;
            cycleEnd = now;
            cycleReason = QString::fromUtf8("нет данных");
            Writer* w = writer;
            QMetaObject::invokeMethod(writer, [w, now]() { if (w->store) w->cycleId = w->store->beginCycle(now); }, Qt::QueuedConnection);
        }
        else
            onKeyOff(pviDone);
    }
    if (in.keyOn)
    {
        cycleEnd = now;
        if (!pviDone)
            cycleReason = QString::fromUtf8("нет данных");
    }

    // ПВИ (удержание >= 2 с при включённом ключе)
    if (in.keyOn && in.pvi && in.bucKnown)
    {
        if (++pviSec >= 2 && !pviDone)
        {
            pviDone = true;
            EventRec e;
            e.type = EvPviOff;
            push(e);
            cycleEnd = now;
            cycleReason = QString::fromUtf8("ПВИ");
            flush(false);
        }
    }
    else
    {
        pviSec = 0;
        pviDone = false;
    }

    // уборка и пауза
    edge(cleanPrev, in.cleanRunning, EvCleanStart, EvCleanStop, SevInfo);
    edge(pausePrev, in.cleanRunning && in.paused, EvPauseOn, EvPauseOff, SevInfo);

    // связь с ЭБУ
    const bool ecuDown = in.keyOn && !s.ecuOnline && ms > 15000;
    if (ecuDown && !ecuLost)
    {
        ecuLost = true;
        ecuLostT = now;
        EventRec e;
        e.type = EvEcuLost;
        e.severity = SevAlarm;
        push(e);
    }
    else if (!ecuDown && ecuLost)
    {
        ecuLost = false;
        EventRec e;
        e.type = EvEcuLost;
        e.value = double(now - ecuLostT);
        e.text = kRestored;
        push(e);
    }

    // связь с БУЦ (была конфигурация — пропала)
    if (bucPrev && !in.bucKnown && !bucLost)
    {
        bucLost = true;
        bucLostT = now;
        EventRec e;
        e.type = EvBucLost;
        e.severity = SevAlarm;
        push(e);
    }
    else if (bucLost && in.bucKnown)
    {
        bucLost = false;
        EventRec e;
        e.type = EvBucLost;
        e.value = double(now - bucLostT);
        e.text = kRestored;
        push(e);
    }
    bucPrev = in.bucKnown;

    // датчики БУЦ
    if (in.bucKnown)
        for (int i = 0; i < FltCount; ++i)
        {
            const bool v = in.filters[i];
            if (v && !fltPrev[i])
            {
                fltT[i] = now;
                EventRec e;
                e.type = EvFilter;
                e.code = i;
                e.severity = i == FltAlarmBtn ? SevAlarm : SevWarn;
                push(e);
            }
            else if (!v && fltPrev[i])
            {
                EventRec e;
                e.type = EvFilter;
                e.code = i;
                e.value = double(now - fltT[i]);
                e.text = kEnd;
                push(e);
            }
            fltPrev[i] = v;
        }

    // перегревы
    auto overheat = [this, now](bool hot, bool& state, qint64& t0, double& peak, double v, int type, double thr)
    {
        if (hot)
        {
            if (!state)
            {
                state = true;
                t0 = now;
                peak = v;
                EventRec e;
                e.type = type;
                e.severity = SevWarn;
                e.value = thr;
                push(e);
            }
            if (!hasValue(peak) || v > peak)
                peak = v;
        }
        else if (state)
        {
            state = false;
            EventRec e;
            e.type = type;
            e.value = peak;
            e.value2 = double(now - t0);
            e.text = kEnd;
            push(e);
        }
    };
    overheat(hasValue(s.hydro) && s.hydro > st.hydroOver, hydroHot, hydroHotT, hydroPeak, s.hydro, EvHydroOverheat, st.hydroOver);
    overheat(hasValue(s.coolant) && s.coolant > st.coolantOver, coolHot, coolHotT, coolPeak, s.coolant, EvCoolantOverheat, st.coolantOver);

    // DTC (DM1)
    for (auto it = dtcSeen.begin(); it != dtcSeen.end();)
    {
        const quint32 key = it.key();
        const bool recent = ms - it.value() <= 5000;
        if (recent && !dtcActive.contains(key))
        {
            dtcActive.insert(key, now);
            EventRec e;
            e.type = EvDtc;
            e.code = key >> 5;
            e.value = key & 0x1F;
            e.severity = SevWarn;
            push(e);
        }
        if (!recent)
        {
            if (dtcActive.contains(key))
            {
                EventRec e;
                e.type = EvDtc;
                e.code = key >> 5;
                e.value = key & 0x1F;
                e.value2 = double(now - dtcActive.value(key));
                e.text = kEnd;
                push(e);
                dtcActive.remove(key);
            }
            it = dtcSeen.erase(it);
        }
        else
            ++it;
    }

    // топливо: заправки и сливы — раз в 10 с по сглаженному уровню
    lastMode = in.keyOn ? modeOf(s, st) : ModeKeyOff;
    QVector<FuelDetector::Found> found;
    if (!in.keyOn && keyPrevFuel)
        found = fuel.close(now, fuelUsed);           // ключ выключен — событие не переносится через стоянку
    keyPrevFuel = in.keyOn;
    if (in.keyOn && now - lastFuelFeed >= 10)
    {
        lastFuelFeed = now;
        const double lvl = levelFilter.value();
        found += fuel.feed(now, lvl, fuelUsed);
    }
    if (in.keyOn && (gapPending || gapLevelPending))
        checkGap(now);
    {
        for (const FuelDetector::Found& f : found)
        {
            EventRec e;
            e.type = f.refuel ? EvRefuel : EvFuelDrain;
            e.severity = f.refuel ? SevInfo : SevAlarm;
            e.t = f.keyOff ? f.tEnd : f.tStart;
            e.value = f.liters;
            e.value2 = f.levelBefore;
            e.value3 = f.levelAfter;
            e.code = f.keyOff ? 1 : 0;
            e.keyOn = !f.keyOff;
            e.mode = f.keyOff ? ModeKeyOff : lastMode;
            push(e);
        }
    }

    // очистка старых данных — раз в сутки
    if (now - lastPurge >= 86400)
    {
        lastPurge = now;
        const int keep = st.keepDays;
        Writer* w = writer;
        QMetaObject::invokeMethod(writer, [w, keep, now]()
        {
            if (w->store)
            {
                const int n = w->store->purge(keep, now);
                if (n > 0)
                    qDebug() << "WORK JOURNAL: удалено старых записей" << n;
            }
        }, Qt::QueuedConnection);
    }
}

double Collector::lfcSum() const
{
    double r = noValue();
    for (int b = 0; b < 2; ++b)
        if (rateMask & (1 << b))
        {
            const double v = fresh(lfcV[b], 60000);
            if (!hasValue(v))
                return noValue();      // сумма имеет смысл, только если есть все выбранные счётчики
            r = hasValue(r) ? r + v : v;
        }
    return r;
}

void Collector::saveSnap()
{
    // уровень — последний сглаженный на стоянке; одометр и LFC — последние известные
    if (!hasValue(levelHeld) && !hasValue(odoHeld) && !hasValue(lfcHeld))
        return;
    snap.t = lastNow;
    snap.level = levelHeld;
    snap.odo = odoHeld;
    snap.lfc = lfcHeld;
    snap.lfcAtLevel = lfcAtLevel;
    auto f = [](double v) { return hasValue(v) ? QString::number(v, 'f', 3) : QString("nan"); };
    const QString v = QString("%1;%2;%3;%4;%5").arg(snap.t).arg(f(snap.level), f(snap.odo), f(snap.lfc), f(snap.lfcAtLevel));
    Writer* w = writer;
    QMetaObject::invokeMethod(writer, [w, v]() { if (w->store) w->store->setMeta("snap", v); }, Qt::QueuedConnection);
}

void Collector::checkGap(qint64 now)
{
    // 1) одометр и LFC шасси (ждём до 60 с): пробег и расход, пока система была выключена
    // 2) уровень на стоянке (ждём до 30 мин): заправка/слив с поправкой на расход по LFC между замерами уровня
    if (snap.t <= 0 || now <= snap.t)
    {
        gapPending = gapLevelPending = false;
        return;
    }
    const qint64 waited = now - gapSince;
    const double lfcNow = lfcSum();
    if (gapPending)
    {
        const double odoNow = fresh(odo, 60000);
        if ((!hasValue(odoNow) || !hasValue(lfcNow)) && waited < 60)
            return;
        gapPending = false;
        gapLevelPending = true;
        const double dOdo = hasValue(odoNow) && hasValue(snap.odo) ? odoNow - snap.odo : noValue();
        const double dLfc = hasValue(lfcNow) && hasValue(snap.lfc) ? lfcNow - snap.lfc : noValue();
        if ((hasValue(dOdo) && dOdo >= 0.2) || (hasValue(dLfc) && dLfc >= 0.5))
        {
            EventRec e;
            e.type = EvOffRun;
            e.t = now;
            e.keyOn = false;
            e.mode = ModeKeyOff;
            e.value = hasValue(dOdo) ? qMax(0.0, dOdo) : noValue();
            e.value2 = hasValue(dLfc) ? qMax(0.0, dLfc) : noValue();
            e.value3 = snap.level;
            e.code = now - snap.t;
            push(e);
            qDebug() << "WORK JOURNAL: работа шасси без системы, км" << e.value << "л" << e.value2;
        }
    }
    if (!gapLevelPending)
        return;
    const double lvlNow = levelFilter.value();
    if (!hasValue(lvlNow))
    {
        if (waited >= 1800)
            gapLevelPending = false;        // за 30 мин ни разу не стояли — сравнить уровень нельзя
        return;
    }
    gapLevelPending = false;
    // расход между последним замером уровня до выключения и текущим — по LFC (идёт и при выключенной системе)
    const double used = hasValue(lfcNow) && hasValue(snap.lfcAtLevel) ? qMax(0.0, lfcNow - snap.lfcAtLevel) : 0.0;
    for (const FuelDetector::Found& f : fuel.checkKeyOffJump(snap.t, snap.level, now, lvlNow, used))
    {
        EventRec e;
        e.type = f.refuel ? EvRefuel : EvFuelDrain;
        e.severity = f.refuel ? SevInfo : SevAlarm;
        e.t = now;
        e.value = f.liters;
        e.value2 = f.levelBefore;
        e.value3 = f.levelAfter;
        e.code = 1;
        e.keyOn = false;
        e.mode = ModeKeyOff;
        push(e);
    }
}

void Collector::writeMinute()
{
    if (builder.empty() && pending.isEmpty())
        return;
    const bool hasMinute = !builder.empty();
    if (hasMinute && keyPrev && !gapPending && !gapLevelPending)
        saveSnap();                    // снимок раз в минуту (питание может пропасть без выключения ключа)
    MinuteRec m = builder.finish();
    m.timeInvalid = QDateTime::fromSecsSinceEpoch(m.t).date().year() < 2026;
    QVector<EventRec> ev = pending;
    pending.clear();
    const qint64 ce = cycleEnd;
    const QString cr = cycleReason;
    Writer* w = writer;
    QMetaObject::invokeMethod(writer, [w, m, ev, ce, cr, hasMinute]()
    {
        if (!w->store)
            return;
        bool ok = hasMinute ? w->store->writeMinute(m, ev, w->cycleId, ce, cr)
                            : (w->store->writeEvents(ev) && w->store->updateCycle(w->cycleId, ce, cr));
        if (!ok)
            qDebug() << "WORK JOURNAL: ошибка записи:" << w->store->lastError();
    }, Qt::QueuedConnection);
}

void Collector::flush(bool blocking)
{
    if (!started)
        return;
    const bool hasMinute = !builder.empty();
    MinuteRec m = builder.finish();          // минута остаётся в сборке: при следующей записи заменится полной
    m.timeInvalid = QDateTime::fromSecsSinceEpoch(m.t).date().year() < 2026;
    QVector<EventRec> ev = pending;
    pending.clear();
    const qint64 ce = cycleEnd;
    const QString cr = cycleReason;
    Writer* w = writer;
    QMetaObject::invokeMethod(writer, [w, m, ev, ce, cr, hasMinute]()
    {
        if (!w->store)
            return;
        if (hasMinute)
            w->store->writeMinute(m, ev, w->cycleId, ce, cr);
        else
        {
            w->store->writeEvents(ev);
            w->store->updateCycle(w->cycleId, ce, cr);
        }
    }, blocking ? Qt::BlockingQueuedConnection : Qt::QueuedConnection);
}

} // namespace WJ
