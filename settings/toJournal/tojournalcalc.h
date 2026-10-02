/*
 * Журнал ТО — расчёты (без UI и без БД, покрыто тестами)
 * Версия: 01, 2026-09-28
 * Зависимости: Qt 5.15 (QtCore), tojournaltypes.h
 *
 * Правила (согласовано с Иваном 28.09):
 *  - Рубеж ТО — моточасы, кратные интервалу хотя бы одной периодической процедуры (ТО-450 = ТО-25 + ТО-50, ТО-100 не входит).
 *  - Срок работы идёт по сетке своей процедуры: после выполнения на рубеже M — следующий кратный интервалу рубеж после M.
 *    Невыполненная работа остаётся с прежним сроком (просрочка, остаток отрицательный).
 *  - Состав ТО на рубеже M — все периодические работы со сроком <= M (т.е. работы этого рубежа + просроченные).
 *  - Ближайшее плановое ТО — первый не записанный рубеж после последнего проведённого, не просроченный более чем на 10 м/ч.
 *  - «Пропуск» — рубеж, ТО по которому проведено позже чем через 10 м/ч или не проведено вовсе.
 */
#ifndef TOJOURNALCALC_H
#define TOJOURNALCALC_H

#include "tojournaltypes.h"

namespace ToJ
{

// Периодическая работа с посчитанным сроком
struct DueWork
{
    Work    work;
    int     interval = 0;
    QString procedureName;
    int     procedureSort = 0;
    int     due = 0;             // рубеж, к которому работа должна быть выполнена
};

struct Stats
{
    int    inTime = 0;
    int    missed = 0;
    double ratePerDay = -1;      // м/ч в сутки; < 0 — оценки нет
};

class Calc
{
public:
    Calc(const QVector<Procedure>& procedures, const QVector<Record>& records, double startHours);

    // Срок работы по интервалу процедуры
    static int dueOf(const Work& work, int interval);

    // Все периодические работы со сроками, по возрастанию срока
    QVector<DueWork> dueWorks() const;

    // Рубеж ближайшего планового ТО при текущих моточасах; kNoValue — нет периодических процедур
    int plannedMilestone(double hours) const;

    // Варианты для выбора ТО: пропущенные рубежи (до 4), ближайший, и 2 следующих
    QVector<int> milestoneOptions(double hours) const;

    // Состав ТО на рубеже: работы со сроком <= milestone, по категории и порядку в процедурах
    QVector<DueWork> checklist(int milestone) const;

    // Разовые работы (процедуры с интервалом 0)
    QVector<Work> oneTimeWorks() const;

    // Счётчики «в срок» / «пропуск»
    void countStats(double hours, int& inTime, int& missed) const;

    // Темп м/ч в сутки по суточным точкам (окно 30 дней); < 0 — мало данных
    static double ratePerDay(const QVector<HoursSample>& samples, double hours, const QDate& today);

    // Сутки до рубежа при темпе; kNoValue — оценки нет
    static int daysTo(double remaining, double rate);

    // Следующий рубеж строго после x
    int nextGridPointAfter(int x) const;
    bool hasPeriodic() const { return !intervals.isEmpty(); }

private:
    QVector<Procedure> procs;
    QVector<Record>    recs;
    QVector<int>       intervals;   // интервалы периодических процедур, без повторов
    double             start;
    int lastRecordedMilestone() const;
    int firstCandidate() const;
};

// Склонение: 1 день, 2 дня, 5 дней
QString plural(int n, const QString& one, const QString& few, const QString& many);

} // namespace ToJ

#endif // TOJOURNALCALC_H
