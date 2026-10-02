/*
 * Журнал работы — сборщик данных: кадры J1939 (ЭБУ ДВС на j1939Deivce, шасси на canDeivce),
 * состояние пульта раз в секунду, сборка минуты, события, заправки/сливы; запись — в отдельном потоке
 * Версия: 03, 2026-09-30
 * Изменения от 02: speedBus; источники по шинам описаны
 * Изменения от 01: источники по шинам (уровень и одометр — шасси, расход — сумма ДВС); снимок уровень/одометр/LFC
 * Зависимости: Qt 5.15 core + sql; wjcalc, wjstore
 */
#ifndef WJCOLLECTOR_H
#define WJCOLLECTOR_H

#include "wjtypes.h"
#include "wjcalc.h"
#include <QObject>
#include <QByteArray>
#include <QElapsedTimer>
#include <QHash>
#include <QPointer>

class QThread;

namespace WJ
{

class Writer;
class Store;

// Состояние пульта за секунду (заполняет Hooks::tick из MainWindow)
struct Inputs
{
    qint64 now = 0;               // секунды UTC
    bool   bucKnown = false;      // БУЦ сконфигурирован — его входы достоверны
    bool   keyOn = true;          // IN1 (при !bucKnown — считается включённым: пульт работает от ключа)
    bool   pvi = false;           // IN3 — кнопка ПВИ
    bool   cleanRunning = false;  // MainWindow::startClean
    bool   paused = false;        // MainWindow::pauseActive
    double hydro = noValue();     // °C
    bool   eq[EqCount] = {};
    bool   filters[FltCount] = {false, false, false, false, false, false, false, false};
    double engineH = noValue();   // моточасы программы
};

class Collector : public QObject
{
    Q_OBJECT
public:
    explicit Collector(const QString& dbPath, QObject* parent = nullptr);
    ~Collector() override;

    void tick(const Inputs& in);            // раз в секунду
    void reloadSettings();                  // после «СОХРАНИТЬ» на странице настроек
    void flush(bool blocking);              // записать текущую минуту (выход программы, ПВИ)
    bool ok() const { return started; }

    // Источники по шинам (0 — j1939Deivce, ДВС надстройки; 1 — canDeivce, шасси). Разные машины — разные варианты:
    //   levelBus — откуда уровень топлива DD (по умолчанию 1: общий бак, уровень даёт ДВС шасси);
    //   rateMask — чьи LFE/LFC суммировать в расход из бака (бит 0 — ДВС надстройки, бит 1 — шасси; по умолчанию 3);
    //   odoBus   — откуда одометр VDHR/VD (по умолчанию 1: шасси)
    //   speedBus — откуда скорость машины CCVS/EBC2 (по умолчанию 1: шасси)
    // Обороты, ОЖ, давление масла, DM1, HOURS — всегда шина 0: ДВС надстройки, которым управляет пульт
    void setBuses(int levelBus, int rateMask, int odoBus, int speedBus)
    { this->levelBus = levelBus; this->rateMask = rateMask; this->odoBus = odoBus; this->speedBus = speedBus; }

public slots:
    void onEngineFrame(quint32 pgn, quint8 sa, QByteArray data);   // canj1939 (j1939Deivce)
    void onChassisFrame(quint32 pgn, quint8 sa, QByteArray data);  // canj1939Main (canDeivce)

private:
    struct Val { double v = noValue(); qint64 ms = -1000000; };
    double fresh(const Val& x, qint64 maxAgeMs = 5000) const;
    void   parse(quint32 pgn, const QByteArray& data, int bus);
    void   push(EventRec e);
    void   edge(bool& prev, bool now, int onType, int offType, int sev);
    void   writeMinute();
    void   onKeyOff(bool byPvi);

    QString       path;
    QThread*      thread = nullptr;
    Writer*       writer = nullptr;
    bool          started = false;
    Settings      st;
    QElapsedTimer clock;
    int           levelBus = 1, rateMask = 3, odoBus = 1, speedBus = 1;

    // J1939
    Val rpm, coolant, oilP, rate[2], lfcV[2], level, ecuH, odo, ccvs, ebc2;
    double lfcSum() const;              // сумма LFC по rateMask; NaN — нет свежих данных
    qint64 eec1Ms = -1000000;
    QHash<quint32, qint64> dtcSeen;     // spn<<5|fmi -> ms
    QHash<quint32, qint64> dtcActive;   // spn<<5|fmi -> t начала (сек)

    // сборка
    MinuteBuilder builder;
    QVector<EventRec> pending;
    LevelFilter   levelFilter{60};
    FuelDetector  fuel;
    double        levelHeld = noValue(); // последний сглаженный уровень
    double        fuelUsed = 0;         // нарастающий расход по LFE, л
    qint64        lastFuelFeed = 0;
    bool          keyPrevFuel = false;
    // снимок «уровень / одометр / LFC» — раз в минуту и при выключении ключа (meta "snap"); при запуске программы
    // и включении ключа сравнивается с текущим: работа шасси и заправки/сливы при выключенной системе
    // lfcAtLevel — показание LFC в момент, когда был снят уровень (уровень меряется только на стоянке)
    struct Snap { qint64 t = 0; double level = noValue(), odo = noValue(), lfc = noValue(), lfcAtLevel = noValue(); };
    Snap          snap;
    bool          gapPending = true;       // ждём одометр и LFC
    bool          gapLevelPending = false; // ждём уровень на стоянке
    qint64        gapSince = 0;
    double        odoHeld = noValue(), lfcHeld = noValue(), lfcAtLevel = noValue();
    void          saveSnap();
    void          checkGap(qint64 now);

    // состояния для событий
    bool   first = true;
    bool   keyPrev = false, cleanPrev = false, pausePrev = false, bucPrev = false;
    bool   ecuLost = false;  qint64 ecuLostT = 0;
    bool   bucLost = false;  qint64 bucLostT = 0;
    bool   fltPrev[FltCount] = {false, false, false, false, false, false, false, false};
    qint64 fltT[FltCount] = {0, 0, 0, 0, 0, 0, 0, 0};
    bool   hydroHot = false;   qint64 hydroHotT = 0;   double hydroPeak = noValue();
    bool   coolHot = false;    qint64 coolHotT = 0;    double coolPeak = noValue();
    int    pviSec = 0;         bool   pviDone = false;
    bool   timeInvalidLogged = false;
    qint64 lastNow = 0;
    int    lastMode = -1;
    qint64 cycleEnd = 0;
    QString cycleReason;
    qint64 lastPurge = 0;
};

// Поток записи (владеет своим соединением с БД)
class Writer : public QObject
{
    Q_OBJECT
public:
    explicit Writer(const QString& path) : path(path) {}
    Store* store = nullptr;
    qint64 cycleId = 0;
    QString path;
    bool   init();
    ~Writer() override;
};

} // namespace WJ

#endif // WJCOLLECTOR_H
