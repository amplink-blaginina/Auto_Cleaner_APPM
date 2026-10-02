/*
 * Журнал ТО — хранилище SQLite
 * Версия: 01, 2026-09-28
 * Зависимости: Qt 5.15 (QtSql, драйвер QSQLITE — пакет libqt5sql5-sqlite), tojournaltypes.h
 *
 * Файл БД: WAL + synchronous=FULL — запись переживает пропадание питания.
 * Все изменения — в транзакциях. Работает в потоке GUI (операции единичные, < 5 мс на SD).
 */
#ifndef TOJOURNALSTORE_H
#define TOJOURNALSTORE_H

#include <QString>
#include <QSqlDatabase>
#include "tojournaltypes.h"

namespace ToJ
{

class Store
{
public:
    Store();
    ~Store();

    // Открыть/создать БД. currentHours — моточасы на момент создания (начало журнала); < 0 — неизвестны
    bool open(const QString& path, double currentHours);
    bool isOpen() const { return db.isOpen(); }
    QString lastError() const { return err; }

    double startHours() const;                      // < 0 — ещё неизвестно
    double lastKnownHours() const;                  // последние сохранённые моточасы (если счётчик недоступен)

    QVector<Procedure> procedures() const;          // с работами, по порядку
    QVector<Record>    records() const;             // новые сверху
    QVector<HoursSample> hoursSamples(int days) const;

    // Сохранить процедуру целиком (новая — id == kNoValue). Работы, которых нет в списке, удаляются.
    // Новым работам anchorHours = currentHours. Возвращает id процедуры или kNoValue.
    int  saveProcedure(const Procedure& p, double currentHours);
    bool deleteProcedure(int id);

    // Провести ТО: запись + отметка lastDone у выполненных периодических работ
    int  addRecord(const Record& r);

    // Суточная точка моточасов (не чаще раза в 60 с на запись; в сутках хранится максимум)
    void logHours(double hours);

private:
    QSqlDatabase db;
    QString      conn;
    QString      err;
    qint64       lastLogMs = 0;
    double       lastLogHours = -1;

    bool exec(const QString& sql);
    bool createSchema(double currentHours);
    void setError(const QString& where, const QString& what);
};

} // namespace ToJ

#endif // TOJOURNALSTORE_H
