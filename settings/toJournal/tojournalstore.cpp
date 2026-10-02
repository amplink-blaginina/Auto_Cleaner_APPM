/*
 * Журнал ТО — хранилище SQLite
 * Версия: 02, 2026-09-29
 * Изменения от 01: новая БД заполняется базовым составом ТО (tojournaldefaults.h)
 */
#include "tojournalstore.h"
#include "tojournaldefaults.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QDateTime>

namespace ToJ
{

static const int kSchemaVersion = 1;

Store::Store()
{
    conn = QStringLiteral("tojournal_%1").arg(quintptr(this));
}

Store::~Store()
{
    if (db.isOpen())
        db.close();
    db = QSqlDatabase();
    QSqlDatabase::removeDatabase(conn);
}

void Store::setError(const QString& where, const QString& what)
{
    err = where + ": " + what;
    qDebug() << "TO JOURNAL:" << err;
}

bool Store::exec(const QString& sql)
{
    QSqlQuery q(db);
    if (!q.exec(sql))
    {
        setError(sql.left(60), q.lastError().text());
        return false;
    }
    return true;
}

bool Store::open(const QString& path, double currentHours)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
    db.setDatabaseName(path);
    if (!db.open())
    {
        setError("open " + path, db.lastError().text());
        return false;
    }
    exec("PRAGMA journal_mode=WAL");
    exec("PRAGMA synchronous=FULL");
    exec("PRAGMA foreign_keys=ON");
    exec("PRAGMA busy_timeout=2000");

    QSqlQuery q(db);
    q.exec("PRAGMA user_version");
    int ver = q.next() ? q.value(0).toInt() : 0;
    if (ver == 0)
    {
        if (!createSchema(currentHours))
            return false;
    }
    else if (ver > kSchemaVersion)
    {
        setError("open", QString("версия БД %1 новее программы (%2)").arg(ver).arg(kSchemaVersion));
        db.close();
        return false;
    }
    qDebug() << "TO JOURNAL: БД" << path << "начало журнала" << startHours() << "м/ч";
    return true;
}

bool Store::createSchema(double currentHours)
{
    db.transaction();
    bool ok = exec("CREATE TABLE IF NOT EXISTS meta (key TEXT PRIMARY KEY, value TEXT)")
           && exec("CREATE TABLE IF NOT EXISTS procedure (id INTEGER PRIMARY KEY, name TEXT NOT NULL,"
                   " interval INTEGER NOT NULL, sort INTEGER NOT NULL DEFAULT 0)")
           && exec("CREATE TABLE IF NOT EXISTS work (id INTEGER PRIMARY KEY,"
                   " procedure_id INTEGER NOT NULL REFERENCES procedure(id) ON DELETE CASCADE,"
                   " name TEXT NOT NULL, category INTEGER NOT NULL, sort INTEGER NOT NULL DEFAULT 0,"
                   " anchor_hours REAL NOT NULL, last_done INTEGER)")
           && exec("CREATE TABLE IF NOT EXISTS record (id INTEGER PRIMARY KEY, ts TEXT NOT NULL,"
                   " hours REAL NOT NULL, milestone INTEGER, executor TEXT NOT NULL)")
           && exec("CREATE TABLE IF NOT EXISTS record_item (id INTEGER PRIMARY KEY,"
                   " record_id INTEGER NOT NULL REFERENCES record(id) ON DELETE CASCADE,"
                   " work_id INTEGER, name TEXT NOT NULL, category INTEGER NOT NULL,"
                   " periodic INTEGER NOT NULL, sort INTEGER NOT NULL)")
           && exec("CREATE TABLE IF NOT EXISTS hours_log (day TEXT PRIMARY KEY, hours REAL NOT NULL)")
           && exec("CREATE INDEX IF NOT EXISTS work_proc ON work(procedure_id)")
           && exec("CREATE INDEX IF NOT EXISTS item_rec ON record_item(record_id)");
    if (ok && currentHours >= 0)
    {// моточасы неизвестны — начало журнала запишет первый logHours()
        QSqlQuery q(db);
        q.prepare("INSERT OR REPLACE INTO meta(key, value) VALUES('start_hours', ?)");
        q.addBindValue(QString::number(currentHours, 'f', 1));
        ok = q.exec();
    }
    // базовый состав ТО (tojournaldefaults.h) — только в новой БД
    for (int p = 0; ok && p < int(sizeof(kDefaultProcedures) / sizeof(kDefaultProcedures[0])); ++p)
    {
        const DefaultProcedure& d = kDefaultProcedures[p];
        QSqlQuery ip(db);
        ip.prepare("INSERT INTO procedure(name, interval, sort) VALUES(?, ?, ?)");
        ip.addBindValue(QString::fromUtf8(d.name));
        ip.addBindValue(d.interval);
        ip.addBindValue(p + 1);
        ok = ip.exec();
        int pid = ok ? ip.lastInsertId().toInt() : kNoValue;
        for (int w = 0; ok && w < d.count; ++w)
        {
            QSqlQuery iw(db);
            iw.prepare("INSERT INTO work(procedure_id, name, category, sort, anchor_hours, last_done)"
                       " VALUES(?, ?, ?, ?, ?, NULL)");
            iw.addBindValue(pid);
            iw.addBindValue(QString::fromUtf8(d.works[w].name));
            iw.addBindValue(d.works[w].category);
            iw.addBindValue(w);
            iw.addBindValue(currentHours >= 0 ? currentHours : 0.0);
            ok = iw.exec();
        }
        if (!ok)
            setError("базовый состав ТО", ip.lastError().text());
    }
    if (ok)
        qDebug() << "TO JOURNAL: новая БД заполнена базовым составом ТО";
    ok = ok && exec(QString("PRAGMA user_version=%1").arg(kSchemaVersion));
    if (!ok)
    {
        db.rollback();
        return false;
    }
    return db.commit();
}

double Store::startHours() const
{
    QSqlQuery q(db);
    q.exec("SELECT value FROM meta WHERE key='start_hours'");
    return q.next() ? q.value(0).toDouble() : -1.0;
}

double Store::lastKnownHours() const
{
    QSqlQuery q(db);
    q.exec("SELECT MAX(h) FROM (SELECT MAX(hours) AS h FROM hours_log UNION ALL SELECT MAX(hours) FROM record"
           " UNION ALL SELECT CAST(value AS REAL) FROM meta WHERE key='start_hours')");
    return (q.next() && !q.value(0).isNull()) ? q.value(0).toDouble() : 0.0;
}

QVector<Procedure> Store::procedures() const
{
    QVector<Procedure> out;
    QSqlQuery q(db);
    q.exec("SELECT id, name, interval, sort FROM procedure ORDER BY sort, id");
    while (q.next())
    {
        Procedure p;
        p.id = q.value(0).toInt();
        p.name = q.value(1).toString();
        p.interval = q.value(2).toInt();
        p.sort = q.value(3).toInt();
        out.append(p);
    }
    QSqlQuery w(db);
    w.exec("SELECT id, procedure_id, name, category, sort, anchor_hours, last_done FROM work ORDER BY sort, id");
    while (w.next())
    {
        Work x;
        x.id = w.value(0).toInt();
        x.procedureId = w.value(1).toInt();
        x.name = w.value(2).toString();
        x.category = w.value(3).toInt();
        x.sort = w.value(4).toInt();
        x.anchorHours = w.value(5).toDouble();
        x.lastDone = w.value(6).isNull() ? kNoValue : w.value(6).toInt();
        for (Procedure& p : out)
        {
            if (p.id == x.procedureId)
            {
                p.works.append(x);
                break;
            }
        }
    }
    return out;
}

QVector<Record> Store::records() const
{
    QVector<Record> out;
    QSqlQuery q(db);
    q.exec("SELECT id, ts, hours, milestone, executor FROM record ORDER BY ts DESC, id DESC");
    while (q.next())
    {
        Record r;
        r.id = q.value(0).toInt();
        r.time = QDateTime::fromString(q.value(1).toString(), Qt::ISODate);
        r.hours = q.value(2).toDouble();
        r.milestone = q.value(3).isNull() ? kNoValue : q.value(3).toInt();
        r.executor = q.value(4).toString();
        out.append(r);
    }
    QSqlQuery i(db);
    i.exec("SELECT record_id, work_id, name, category, periodic FROM record_item ORDER BY record_id, sort");
    while (i.next())
    {
        int rid = i.value(0).toInt();
        RecordItem it;
        it.workId = i.value(1).isNull() ? kNoValue : i.value(1).toInt();
        it.name = i.value(2).toString();
        it.category = i.value(3).toInt();
        it.periodic = i.value(4).toInt() != 0;
        for (Record& r : out)
        {
            if (r.id == rid)
            {
                r.items.append(it);
                break;
            }
        }
    }
    return out;
}

QVector<HoursSample> Store::hoursSamples(int days) const
{
    QVector<HoursSample> out;
    QSqlQuery q(db);
    q.prepare("SELECT day, hours FROM hours_log WHERE day >= ? ORDER BY day");
    q.addBindValue(QDate::currentDate().addDays(-days).toString(Qt::ISODate));
    q.exec();
    while (q.next())
    {
        HoursSample s;
        s.day = QDate::fromString(q.value(0).toString(), Qt::ISODate);
        s.hours = q.value(1).toDouble();
        out.append(s);
    }
    return out;
}

int Store::saveProcedure(const Procedure& p, double currentHours)
{
    if (!db.transaction())
    {
        setError("saveProcedure", db.lastError().text());
        return kNoValue;
    }
    int id = p.id;
    QSqlQuery q(db);
    bool ok;
    if (id == kNoValue)
    {
        q.prepare("INSERT INTO procedure(name, interval, sort) VALUES(?, ?,"
                  " (SELECT IFNULL(MAX(sort), 0) + 1 FROM procedure))");
        q.addBindValue(p.name);
        q.addBindValue(p.interval);
        ok = q.exec();
        id = ok ? q.lastInsertId().toInt() : kNoValue;
    }
    else
    {
        q.prepare("UPDATE procedure SET name=?, interval=? WHERE id=?");
        q.addBindValue(p.name);
        q.addBindValue(p.interval);
        q.addBindValue(id);
        ok = q.exec();
    }

    // работы: обновить существующие, добавить новые, удалить убранные
    QList<int> keep;
    for (int i = 0; ok && i < p.works.size(); ++i)
    {
        const Work& w = p.works.at(i);
        QSqlQuery u(db);
        if (w.id == kNoValue)
        {
            u.prepare("INSERT INTO work(procedure_id, name, category, sort, anchor_hours, last_done)"
                      " VALUES(?, ?, ?, ?, ?, NULL)");
            u.addBindValue(id);
            u.addBindValue(w.name);
            u.addBindValue(w.category);
            u.addBindValue(i);
            u.addBindValue(currentHours);
            ok = u.exec();
            if (ok)
                keep.append(u.lastInsertId().toInt());
        }
        else
        {
            u.prepare("UPDATE work SET name=?, category=?, sort=? WHERE id=? AND procedure_id=?");
            u.addBindValue(w.name);
            u.addBindValue(w.category);
            u.addBindValue(i);
            u.addBindValue(w.id);
            u.addBindValue(id);
            ok = u.exec();
            keep.append(w.id);
        }
        if (!ok)
            setError("saveProcedure work", u.lastError().text());
    }
    if (ok)
    {
        QStringList ids;
        for (int k : keep)
            ids << QString::number(k);
        QSqlQuery d(db);
        QString sql = QString("DELETE FROM work WHERE procedure_id=%1").arg(id);
        if (!ids.isEmpty())
            sql += QString(" AND id NOT IN (%1)").arg(ids.join(','));
        ok = d.exec(sql);
    }
    if (!ok)
    {
        if (err.isEmpty())
            setError("saveProcedure", q.lastError().text());
        db.rollback();
        return kNoValue;
    }
    db.commit();
    qDebug() << "TO JOURNAL: процедура сохранена" << id << p.name << p.interval << "м/ч," << p.works.size() << "работ";
    return id;
}

bool Store::deleteProcedure(int id)
{
    QSqlQuery q(db);
    q.prepare("DELETE FROM procedure WHERE id=?");
    q.addBindValue(id);
    if (!q.exec())
    {
        setError("deleteProcedure", q.lastError().text());
        return false;
    }
    qDebug() << "TO JOURNAL: процедура удалена" << id;
    return true;
}

int Store::addRecord(const Record& r)
{
    if (!db.transaction())
    {
        setError("addRecord", db.lastError().text());
        return kNoValue;
    }
    QSqlQuery q(db);
    q.prepare("INSERT INTO record(ts, hours, milestone, executor) VALUES(?, ?, ?, ?)");
    q.addBindValue(r.time.toString(Qt::ISODate));
    q.addBindValue(r.hours);
    q.addBindValue(r.milestone == kNoValue ? QVariant(QVariant::Int) : QVariant(r.milestone));
    q.addBindValue(r.executor);
    bool ok = q.exec();
    int rid = ok ? q.lastInsertId().toInt() : kNoValue;
    for (int i = 0; ok && i < r.items.size(); ++i)
    {
        const RecordItem& it = r.items.at(i);
        QSqlQuery a(db);
        a.prepare("INSERT INTO record_item(record_id, work_id, name, category, periodic, sort) VALUES(?, ?, ?, ?, ?, ?)");
        a.addBindValue(rid);
        a.addBindValue(it.workId == kNoValue ? QVariant(QVariant::Int) : QVariant(it.workId));
        a.addBindValue(it.name);
        a.addBindValue(it.category);
        a.addBindValue(it.periodic ? 1 : 0);
        a.addBindValue(i);
        ok = a.exec();
        if (ok && it.periodic && it.workId != kNoValue && r.milestone != kNoValue)
        {// выполненная периодическая работа — новый отсчёт от рубежа
            QSqlQuery u(db);
            u.prepare("UPDATE work SET last_done=? WHERE id=? AND (last_done IS NULL OR last_done < ?)");
            u.addBindValue(r.milestone);
            u.addBindValue(it.workId);
            u.addBindValue(r.milestone);
            ok = u.exec();
        }
        if (!ok)
            setError("addRecord item", a.lastError().text());
    }
    if (!ok)
    {
        if (err.isEmpty())
            setError("addRecord", q.lastError().text());
        db.rollback();
        return kNoValue;
    }
    db.commit();
    qDebug() << "TO JOURNAL: запись" << rid << "ТО-" << r.milestone << r.hours << "м/ч," << r.items.size() << "работ," << r.executor;
    return rid;
}

void Store::logHours(double hours)
{
    if (!db.isOpen() || hours < 0)
        return;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (lastLogMs != 0 && now - lastLogMs < 60000 && qAbs(hours - lastLogHours) < 0.5)
        return;
    lastLogMs = now;
    lastLogHours = hours;
    QSqlQuery q(db);
    q.prepare("INSERT INTO hours_log(day, hours) VALUES(?, ?)"
              " ON CONFLICT(day) DO UPDATE SET hours=MAX(hours, excluded.hours)");
    q.addBindValue(QDate::currentDate().toString(Qt::ISODate));
    q.addBindValue(hours);
    if (!q.exec())
        setError("logHours", q.lastError().text());
    QSqlQuery s(db);
    s.prepare("INSERT OR IGNORE INTO meta(key, value) VALUES('start_hours', ?)");
    s.addBindValue(QString::number(hours, 'f', 1));
    s.exec();
}

} // namespace ToJ
