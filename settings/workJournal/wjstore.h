/*
 * Журнал работы — хранилище SQLite (минуты, события, циклы ключа, график смен, настройки)
 * Версия: 01, 2026-09-29
 * Зависимости: Qt 5.15 sql + драйвер QSQLITE (libqt5sql5-sqlite)
 * WAL + synchronous=FULL, запись минуты — одна транзакция. Каждый поток открывает своё соединение (connName).
 * Изменения: первая версия
 */
#ifndef WJSTORE_H
#define WJSTORE_H

#include "wjtypes.h"
#include "wjcalc.h"
#include <QString>
#include <QVector>
#include <QVariant>

namespace WJ
{

class Store
{
public:
    Store() = default;
    ~Store();
    Store(const Store&) = delete;
    Store& operator=(const Store&) = delete;

    bool    open(const QString& path, const QString& connName);
    void    close();
    bool    isOpen() const { return opened; }
    QString lastError() const { return err; }

    // настройки и график смен
    Settings    settings();
    bool        saveSettings(const Settings& s);
    ScheduleSet schedules();
    bool        saveSchedule(const Schedule& s);     // действует с s.fromDay
    QString     meta(const QString& key, const QString& def = QString());
    bool        setMeta(const QString& key, const QString& value);

    // запись (поток сборщика)
    qint64 beginCycle(qint64 start);
    bool   writeMinute(const MinuteRec& m, const QVector<EventRec>& events, qint64 cycleId, qint64 cycleEnd, const QString& cycleReason);
    bool   writeEvents(const QVector<EventRec>& events);
    bool   updateCycle(qint64 cycleId, qint64 end, const QString& reason);
    int    purge(int keepDays, qint64 now);

    // чтение
    QVector<MinuteRec> minutes(qint64 from, qint64 to);
    QVector<EventRec>  events(qint64 from, qint64 to);
    QVector<CycleRec>  cycles(qint64 from, qint64 to);
    qint64             firstMinute();                 // 0 — пусто

private:
    bool exec(const QString& sql);
    bool createSchema();

    QString conn;
    QString err;
    bool    opened = false;
};

} // namespace WJ

#endif // WJSTORE_H
