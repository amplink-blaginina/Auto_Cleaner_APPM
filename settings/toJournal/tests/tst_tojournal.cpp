// Журнал ТО — юнит-тесты ядра (x86/RPi, Qt 5.15 testlib+sql): qmake tests/tst_tojournal.pro && make && ./tst_tojournal
#include <QtTest>
#include <QTemporaryDir>
#include "../tojournalcalc.h"
#include "../tojournalstore.h"

using namespace ToJ;

static Procedure proc(int id, const QString& name, int interval, QStringList works, int cat = CatReplace, double anchor = 438)
{
    Procedure p;
    p.id = id;
    p.name = name;
    p.interval = interval;
    p.sort = id;
    int wid = id * 100;
    for (const QString& w : works)
    {
        Work x;
        x.id = wid++;
        x.procedureId = id;
        x.name = w;
        x.category = interval == 0 ? CatRepair : cat;
        x.anchorHours = anchor;
        p.works.append(x);
    }
    return p;
}

class TestToJ : public QObject
{
    Q_OBJECT
private slots:
    void due()
    {
        Work w;
        w.anchorHours = 438;
        QCOMPARE(Calc::dueOf(w, 25), 450);
        QCOMPARE(Calc::dueOf(w, 50), 450);
        QCOMPARE(Calc::dueOf(w, 100), 500);
        w.anchorHours = 450;
        QCOMPARE(Calc::dueOf(w, 25), 450);
        w.anchorHours = 0;
        QCOMPARE(Calc::dueOf(w, 25), 25);
        w.lastDone = 475;              // просроченную ТО-50 сделали на ТО-475
        QCOMPARE(Calc::dueOf(w, 50), 500);
        QCOMPARE(Calc::dueOf(w, 0), kNoValue);
    }

    void composition450()
    {
        QVector<Procedure> ps { proc(1, "ТО-25", 25, {"Гидрофильтр", "Масляный фильтр"}),
                                proc(2, "ТО-50", 50, {"Протяжка болтов"}, CatService),
                                proc(3, "ТО-100", 100, {"Осмотр вентилятора"}, CatInspect),
                                proc(4, "ТО-0", 0, {"Зубья ковша"}) };
        Calc c(ps, {}, 438);
        QCOMPARE(c.plannedMilestone(438), 450);
        auto cl = c.checklist(450);
        QCOMPARE(cl.size(), 3);        // ТО-25 + ТО-50, ТО-100 не входит
        QCOMPARE(cl[0].work.category, int(CatReplace));
        QCOMPARE(cl[2].work.name, QString("Протяжка болтов"));
        QCOMPARE(c.checklist(500).size(), 4);
        QCOMPARE(c.oneTimeWorks().size(), 1);
        QCOMPARE(c.milestoneOptions(438), (QVector<int>{450, 475, 500}));
    }

    void plannedAfterRecordAndLate()
    {
        QVector<Procedure> ps { proc(1, "ТО-25", 25, {"A"}), proc(2, "ТО-50", 50, {"B"}) };
        Record r;
        r.milestone = 450;
        r.hours = 448;
        Calc c(ps, {r}, 400);
        QCOMPARE(c.plannedMilestone(452), 475);
        QCOMPARE(c.plannedMilestone(485), 475);   // просрочка 10 — ещё план
        QCOMPARE(c.plannedMilestone(486), 500);   // > 10 — пропуск, план следующий
        QCOMPARE(c.milestoneOptions(486), (QVector<int>{475, 500, 525, 550}));
    }

    void partialLeavesOverdue()
    {
        QVector<Procedure> ps { proc(1, "ТО-25", 25, {"A", "B"}) };
        ps[0].works[0].lastDone = 450;   // A сделали, B — нет
        Record r;
        r.milestone = 450;
        r.hours = 451;
        Calc c(ps, {r}, 438);
        auto dw = c.dueWorks();
        QCOMPARE(dw[0].work.name, QString("B"));
        QCOMPARE(dw[0].due, 450);        // остаток при 460 = -10
        QCOMPARE(dw[1].due, 475);
        QCOMPARE(c.plannedMilestone(460), 475);
        QCOMPARE(c.checklist(475).size(), 2);   // просроченная B попадает в ТО-475
    }

    void stats()
    {
        QVector<Procedure> ps { proc(1, "ТО-25", 25, {"A"}) };
        Record a; a.milestone = 25; a.hours = 30;     // в срок
        Record b; b.milestone = 50; b.hours = 61;     // пропуск (11)
        Record e; e.milestone = kNoValue; e.hours = 70;  // ремонт — не считается
        Calc c(ps, {a, b, e}, 0);
        int in = 0, ms = 0;
        c.countStats(90, in, ms);      // 75 не проведено (90 > 85) — пропуск
        QCOMPARE(in, 1);
        QCOMPARE(ms, 2);
        c.countStats(84, in, ms);      // 75 ещё в допуске
        QCOMPARE(ms, 1);
    }

    void rate()
    {
        QDate t(2026, 6, 3);
        QVector<HoursSample> s { {t.addDays(-40), 300}, {t.addDays(-20), 400}, {t.addDays(-10), 420}, {t, 438} };
        QCOMPARE(Calc::ratePerDay(s, 440, t), 2.0);   // (440-400)/20
        QCOMPARE(Calc::ratePerDay({{t, 438}}, 440, t), -1.0);
        QCOMPARE(Calc::daysTo(12, 2.0), 6);
        QCOMPARE(Calc::daysTo(12, -1), kNoValue);
        QCOMPARE(plural(1, "день", "дня", "дней"), QString("день"));
        QCOMPARE(plural(3, "день", "дня", "дней"), QString("дня"));
        QCOMPARE(plural(11, "день", "дня", "дней"), QString("дней"));
        QCOMPARE(plural(22, "день", "дня", "дней"), QString("дня"));
    }

    void store()
    {
        QTemporaryDir dir;
        QString path = dir.path() + "/sub/to.db";
        {
            Store s;
            QVERIFY(s.open(path, 438));
            QCOMPARE(s.startHours(), 438.0);

            // новая БД заполнена базовым составом ТО (tojournaldefaults.h)
            auto def = s.procedures();
            QCOMPARE(def.size(), 7);
            QCOMPARE(def[0].interval, 25);
            QCOMPARE(def[1].interval, 50);
            QCOMPARE(def[1].works.size(), 8);
            QCOMPARE(def[5].interval, 5000);
            QCOMPARE(def[6].interval, 0);
            QCOMPARE(def[1].works[0].anchorHours, 438.0);
            {
                Calc c(def, {}, 438);
                QCOMPARE(c.plannedMilestone(438), 450);
                QCOMPARE(c.checklist(450).size(), 1 + 8);            // 25 + 50
                QCOMPARE(c.checklist(500).size(), 1 + 8 + 5 + 5);    // 25 + 50 + 100 + 500
                QCOMPARE(c.checklist(1000).size(), 1 + 8 + 5 + 5 + 1);
                QCOMPARE(c.checklist(5000).size(), 1 + 8 + 5 + 5 + 1 + 1);
                QCOMPARE(c.oneTimeWorks().size(), 3);
            }
            for (const Procedure& d : def)
                QVERIFY(s.deleteProcedure(d.id));
            QCOMPARE(s.procedures().size(), 0);

            Procedure p = proc(kNoValue, "ТО-25", 25, {"A", "B"});
            for (Work& w : p.works) w.id = kNoValue;
            int id = s.saveProcedure(p, 438);
            QVERIFY(id != kNoValue);
            auto ps = s.procedures();
            QCOMPARE(ps.size(), 1);
            QCOMPARE(ps[0].works.size(), 2);
            QCOMPARE(ps[0].works[0].anchorHours, 438.0);

            // удалить B, переименовать A, добавить C
            Procedure e = ps[0];
            e.works.removeAt(1);
            e.works[0].name = "A2";
            Work c; c.name = "C"; c.category = CatInspect;
            e.works.append(c);
            e.interval = 50;
            QVERIFY(s.saveProcedure(e, 440) == id);
            ps = s.procedures();
            QCOMPARE(ps[0].interval, 50);
            QCOMPARE(ps[0].works.size(), 2);
            QCOMPARE(ps[0].works[0].name, QString("A2"));
            QCOMPARE(ps[0].works[1].anchorHours, 440.0);

            Record r;
            r.time = QDateTime(QDate(2026, 6, 3), QTime(10, 0));
            r.hours = 448;
            r.milestone = 450;
            r.executor = "ООО \"СервисТех\"";
            RecordItem it; it.workId = ps[0].works[0].id; it.name = "A2"; it.category = CatReplace;
            r.items.append(it);
            RecordItem ex; ex.name = "Зубья ковша"; ex.category = CatRepair; ex.periodic = false;
            r.items.append(ex);
            QVERIFY(s.addRecord(r) != kNoValue);
            ps = s.procedures();
            QCOMPARE(ps[0].works[0].lastDone, 450);
            QCOMPARE(ps[0].works[1].lastDone, int(kNoValue));
            auto rs = s.records();
            QCOMPARE(rs.size(), 1);
            QCOMPARE(rs[0].items.size(), 2);
            QCOMPARE(rs[0].items[1].periodic, false);
            QCOMPARE(rs[0].executor, QString("ООО \"СервисТех\""));

            s.logHours(448);
            s.logHours(448.3); // < 60 с и < 0.5 — не пишется
            QCOMPARE(s.hoursSamples(30).size(), 1);
            QCOMPARE(s.hoursSamples(30)[0].hours, 448.0);

            QVERIFY(s.deleteProcedure(id));
            QCOMPARE(s.procedures().size(), 0);
            QCOMPARE(s.records()[0].items.size(), 2);   // история не зависит от процедур
        }
        {// повторное открытие: начало журнала не меняется
            Store s;
            QVERIFY(s.open(path, 999));
            QCOMPARE(s.startHours(), 438.0);
            QCOMPARE(s.records().size(), 1);
        }
    }
};

QTEST_MAIN(TestToJ)
#include "tst_tojournal.moc"
