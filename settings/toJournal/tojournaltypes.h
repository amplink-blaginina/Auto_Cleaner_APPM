/*
 * Журнал ТО — типы данных
 * Версия: 01, 2026-09-28
 * Зависимости: Qt 5.15 (QtCore)
 */
#ifndef TOJOURNALTYPES_H
#define TOJOURNALTYPES_H

#include <QString>
#include <QDateTime>
#include <QVector>

namespace ToJ
{

// Категория работы: цвет точки и группа в списке «Работы»
enum Category
{
    CatReplace = 0,   // Замена        — оранжевая
    CatService = 1,   // Обслуживание  — зелёная
    CatInspect = 2,   // Осмотр        — синяя
    CatRepair  = 3,   // Ремонт/разовая — фиолетовая (только в процедурах с интервалом 0)
    CatCount   = 4
};

const int kNoValue      = -1;   // «нет значения» для целых полей
const int kIntervalStep = 25;   // шаг интервала в редакторе, м/ч
const int kMaxInterval  = 5000; // верхний предел интервала, м/ч
const int kLateLimit    = 10;   // ТО позже рубежа более чем на 10 м/ч — «пропуск»

// Работа внутри процедуры
struct Work
{
    int     id          = kNoValue;
    int     procedureId = kNoValue;
    QString name;
    int     category    = CatReplace;
    int     sort        = 0;
    double  anchorHours = 0;        // моточасы при создании работы — от них первый срок
    int     lastDone    = kNoValue; // рубеж ТО, на котором работа последний раз выполнена
};

// Процедура ТО: интервал в м/ч (0 — разовые работы, из них список «Дополнительные работы»)
struct Procedure
{
    int           id       = kNoValue;
    QString       name;
    int           interval = kIntervalStep;
    int           sort     = 0;
    QVector<Work> works;
};

// Строка записи журнала (копия названия и категории на момент ТО — история не зависит от правок процедур)
struct RecordItem
{
    int     workId   = kNoValue;
    QString name;
    int     category = CatReplace;
    bool    periodic = true;
};

// Запись журнала
struct Record
{
    int                 id        = kNoValue;
    QDateTime           time;
    double              hours     = 0;
    int                 milestone = kNoValue;   // рубеж ТО (например 450); kNoValue — только разовые работы
    QString             executor;
    QVector<RecordItem> items;
};

// Суточная точка моточасов для расчёта темпа
struct HoursSample
{
    QDate  day;
    double hours = 0;
};

} // namespace ToJ

#endif // TOJOURNALTYPES_H
