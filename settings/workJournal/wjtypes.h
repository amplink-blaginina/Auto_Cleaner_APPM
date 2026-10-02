/*
 * Журнал работы — типы данных
 * Версия: 02, 2026-09-29
 * Изменения от 01: оборудование по логике программы (+магнитная плита, воздуходувка), D1–D3 по исходнику; событие EvOffRun; итоги «без системы»
 * Платформа: RPi 4B, Raspbian 12 bookworm armhf, Qt 5.15.8 (core); проверено на x86 Qt 5.15.13
 * Спецификация: RPI-RES_260929_42_work-journal-spec.md
 */
#ifndef WJTYPES_H
#define WJTYPES_H

#include <QString>
#include <QVector>
#include <QDateTime>
#include <QtGlobal>
#include <cmath>

namespace WJ
{

// «нет значения» для чисел с плавающей точкой (в БД — NULL)
inline double noValue() { return std::nan(""); }
inline bool   hasValue(double v) { return !std::isnan(v); }

// Режим секунды/минуты (п. 2 спецификации)
enum Mode
{
    ModeClean   = 0,   // Уборка — цикл уборки запущен с главного экрана
    ModeMove    = 1,   // Движение — ДВС работает, скорость > порога
    ModeIdle    = 2,   // Холостой ход — ДВС работает, скорость <= порога
    ModeOff     = 3,   // ДВС заглушён — ключ включён, обороты <= порога
    ModeNoLink  = 4,   // Нет связи с ЭБУ — нет EEC1 больше 5 с
    ModeCount   = 5,
    ModeKeyOff  = 5    // только для шкалы: ключ выключен / пульт не работал
};

// Оборудование (секунды работы за минуту). Подписи и сигналы — wjdefaults.h
enum Equip
{
    EqBroomDown = 0,   // щётка опущена (по логике программы: CentralBroom::state >= BroomDownOut)
    EqDumpDown,        // отвал опущен (FrontRail::state >= FrontRailDownOut)
    EqMagnetDown,      // магнитная плита опущена (BackMagnet::state >= BackMagnetDownOut)
    EqBlowerDown,      // воздуходувка опущена (Blower::state >= BlowerDownOut)
    EqRotD1,           // вращение щётки (ЧИМ D1)
    EqRotD2,           // вращение щётки в обратную сторону (ЧИМ D2)
    EqRotD3,           // вращение вентилятора (ЧИМ D3)
    EqCount
};

// Одна минута данных (таблица minute)
struct MinuteRec
{
    qint64 t = 0;                 // начало минуты, секунды UTC (кратно 60)
    int    secMode[ModeCount] = {0, 0, 0, 0, 0};
    int    secKey = 0;            // секунд с включённым ключом
    int    secEq[EqCount] = {};
    double rpmAvg = noValue(), rpmMax = noValue();
    double speedAvg = noValue(), speedMax = noValue();
    double distM = 0;             // путь за минуту, м
    double distCleanM = 0;        // путь в режиме уборки, м
    double fuelL = noValue();     // расход по LFE за минуту, л
    double fuelCleanL = noValue();
    double fuelLevel = noValue(); // уровень топлива на конец минуты (медиана 60 с), %
    double coolantMax = noValue();
    double oilPMin = noValue();   // кПа
    double hydroAvg = noValue(), hydroMax = noValue();
    int    hydroOverSec = 0;
    int    coolantOverSec = 0;
    double engineH0 = noValue();  // моточасы программы (TOCurValues["Engine"]) на начало минуты
    double engineH = noValue();   // ... на конец минуты
    double ecuH = noValue();      // моточасы ЭБУ (HOURS), справочно
    double odoKm = noValue();     // одометр (VDHR/VD), справочно
    double lfcL = noValue();      // общий расход ЭБУ (LFC), справочно
    double lat = noValue(), lon = noValue();   // резерв GNSS
    int    gnssFix = -1;
    bool   timeInvalid = false;

    int  secTotal() const { int s = 0; for (int i = 0; i < ModeCount; ++i) s += secMode[i]; return s; }
    int  dominantMode() const;    // режим с наибольшим числом секунд; ModeKeyOff — если секунд нет
};

// События (таблица event)
enum EventType
{
    EvProgramStart = 1,
    EvKeyOn,
    EvKeyOff,
    EvPviOff,          // выключение кнопкой ПВИ
    EvCleanStart,
    EvCleanStop,
    EvPauseOn,
    EvPauseOff,
    EvEcuLost,         // value = длительность, с (записывается при восстановлении)
    EvBucLost,
    EvDtc,             // code = SPN, value = FMI; text — «снят»/пусто
    EvFilter,          // code = номер датчика (FilterId), value = длительность, с
    EvHydroOverheat,   // value = макс. температура
    EvCoolantOverheat,
    EvRefuel,          // value = литры, value2 = уровень до, value3 = уровень после; code = 1 — при выключенном ключе
    EvFuelDrain,
    EvTimeInvalid,
    EvOffRun           // работа шасси при выключенной системе управления: value = пробег, км; value2 = топливо по LFC, л;
                       // value3 = уровень до, %; code = длительность, с
};

enum Severity { SevInfo = 0, SevWarn = 1, SevAlarm = 2 };

enum FilterId
{
    FltDrain = 0, FltPress1, FltPress2, FltPress3, FltAir, FltOil, FltWater, FltAlarmBtn, FltCount
};

struct EventRec
{
    qint64  id = 0;
    qint64  t = 0;         // секунды UTC
    int     type = 0;
    int     severity = SevInfo;
    qint64  code = 0;
    double  value = noValue();
    double  value2 = noValue();
    double  value3 = noValue();
    int     mode = -1;     // режим в момент события (для топлива)
    bool    keyOn = true;
    QString text;          // готовая строка для отчёта
};

// Цикл ключа (таблица cycle)
struct CycleRec
{
    qint64  id = 0;
    qint64  start = 0, end = 0;   // секунды UTC
    QString endReason;            // «ключ», «ПВИ», «нет данных»
};

// График смен. Начала смен — минуты от 00:00, по порядку смен; смена 1 открывает отчётные сутки
struct Schedule
{
    QDate fromDay;                // отчётные сутки, с которых действует
    int   count = 2;
    int   start[3] = {8 * 60, 20 * 60, 0};

    int  offset(int i) const { return ((start[i] - start[0]) % 1440 + 1440) % 1440; }   // от начала суток, мин
    bool valid() const;
};

// Настройки журнала (таблица kv, группа «Настройки журнала»)
struct Settings
{
    QString gosNumber;
    QString organization;
    double rpmOn = 300;           // об/мин — «ДВС работает» для режимов
    double speedMove = 3;         // км/ч — «движение»
    double hydroOver = 80;        // °C
    double coolantOver = 105;     // °C
    double tankL = 250;           // л
    double refuelPct = 10;        // % бака
    double drainPct = 5;          // % бака
    int    fuelWindowMin = 5;     // мин
    double residualPct = 5;       // % бака — порог невязки баланса
    int    keepDays = 365;        // хранение минут и событий, сут
};

// Интервал смены
struct ShiftSlot
{
    QDate     day;                // отчётные сутки
    int       no = 1;             // номер смены 1..3
    QDateTime begin, end;         // локальное время
    bool isValid() const { return begin.isValid(); }
};

// Итоги за интервал (смена, сутки, строка периода)
struct Totals
{
    qint64 begin = 0, end = 0;    // секунды UTC
    int    minutes = 0;           // минут с данными
    int    secMode[ModeCount] = {0, 0, 0, 0, 0};
    int    secKey = 0;
    int    secEq[EqCount] = {};
    double distM = 0, distCleanM = 0;
    double fuelL = noValue(), fuelCleanL = noValue();
    double speedCleanAvg = noValue(), speedMax = noValue();
    double rpmAvg = noValue(), rpmMax = noValue();
    double coolantMax = noValue(), oilPMin = noValue();
    double hydroMax = noValue();
    int    hydroOverSec = 0, coolantOverSec = 0;
    double engineHStart = noValue(), engineHEnd = noValue();
    double levelStart = noValue(), levelEnd = noValue();   // %
    qint64 firstKeyOn = 0, lastKeyOn = 0;                  // секунды UTC; 0 — нет данных
    int    events = 0, warnEvents = 0, alarmEvents = 0;
    double refuelL = 0, drainL = 0;
    int    refuelCount = 0, drainCount = 0;
    double offKm = 0, offFuelL = 0;      // работа шасси при выключенной системе (EvOffRun)
    int    offCount = 0;
    bool   timeInvalid = false;

    int    secEngine() const { return secMode[ModeClean] + secMode[ModeMove] + secMode[ModeIdle]; }
    double engineH() const { return hasValue(engineHStart) && hasValue(engineHEnd) ? engineHEnd - engineHStart : noValue(); }
    double fuelPerCleanHour() const;
    double residualL(double tankL) const;   // невязка баланса, л (NaN — нет уровня)
};

} // namespace WJ

#endif // WJTYPES_H
