/*
 * Журнал работы — хранилище SQLite (см. wjstore.h)
 * Версия: 02, 2026-09-29
 * Изменения от 01: колонки eq5, eq6
 */
#include "wjstore.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

namespace WJ
{

static QVariant nv(double v) { return hasValue(v) ? QVariant(v) : QVariant(QVariant::Double); }
static double   dv(const QVariant& v) { return v.isNull() ? noValue() : v.toDouble(); }

Store::~Store()
{
    close();
}

void Store::close()
{
    if (conn.isEmpty())
        return;
    {
        QSqlDatabase db = QSqlDatabase::database(conn, false);
        if (db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(conn);
    conn.clear();
    opened = false;
}

bool Store::exec(const QString& sql)
{
    QSqlQuery q(QSqlDatabase::database(conn));
    if (!q.exec(sql))
    {
        err = q.lastError().text() + " | " + sql.left(80);
        return false;
    }
    return true;
}

bool Store::open(const QString& path, const QString& connName)
{
    close();
    conn = connName;
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", conn);
    db.setDatabaseName(path);
    db.setConnectOptions("QSQLITE_BUSY_TIMEOUT=5000");
    if (!db.open())
    {
        err = db.lastError().text();
        return false;
    }
    exec("PRAGMA journal_mode=WAL");
    exec("PRAGMA synchronous=FULL");
    if (!createSchema())
        return false;
    opened = true;
    return true;
}

bool Store::createSchema()
{
    const char* ddl[] = {
        "CREATE TABLE IF NOT EXISTS meta(k TEXT PRIMARY KEY, v TEXT)",
        "CREATE TABLE IF NOT EXISTS minute("
        " t INTEGER PRIMARY KEY,"
        " s_clean INTEGER, s_move INTEGER, s_idle INTEGER, s_off INTEGER, s_nolink INTEGER, s_key INTEGER,"
        " eq0 INTEGER, eq1 INTEGER, eq2 INTEGER, eq3 INTEGER, eq4 INTEGER, eq5 INTEGER, eq6 INTEGER,"
        " rpm_avg REAL, rpm_max REAL, spd_avg REAL, spd_max REAL, dist_m REAL, dist_clean_m REAL,"
        " fuel_l REAL, fuel_clean_l REAL, fuel_level REAL, coolant_max REAL, oilp_min REAL,"
        " hydro_avg REAL, hydro_max REAL, hydro_over_s INTEGER, coolant_over_s INTEGER,"
        " engine_h0 REAL, engine_h REAL, ecu_h REAL, odo_km REAL, lfc_l REAL,"
        " lat REAL, lon REAL, gnss_fix INTEGER, time_invalid INTEGER)",
        "CREATE TABLE IF NOT EXISTS event(id INTEGER PRIMARY KEY AUTOINCREMENT, t INTEGER, type INTEGER, severity INTEGER,"
        " code INTEGER, value REAL, value2 REAL, value3 REAL, mode INTEGER, key_on INTEGER, text TEXT)",
        "CREATE INDEX IF NOT EXISTS event_t ON event(t)",
        "CREATE TABLE IF NOT EXISTS cycle(id INTEGER PRIMARY KEY AUTOINCREMENT, start INTEGER, end INTEGER, reason TEXT)",
        "CREATE INDEX IF NOT EXISTS cycle_start ON cycle(start)",
        "CREATE TABLE IF NOT EXISTS schedule(from_day TEXT PRIMARY KEY, cnt INTEGER, s1 INTEGER, s2 INTEGER, s3 INTEGER)",
    };
    for (const char* s : ddl)
        if (!exec(QString::fromLatin1(s)))
            return false;
    if (meta("schema").isEmpty())
        setMeta("schema", "1");
    return true;
}

// ---------------- meta / настройки ----------------
QString Store::meta(const QString& key, const QString& def)
{
    QSqlQuery q(QSqlDatabase::database(conn));
    q.prepare("SELECT v FROM meta WHERE k=?");
    q.addBindValue(key);
    if (q.exec() && q.next())
        return q.value(0).toString();
    return def;
}

bool Store::setMeta(const QString& key, const QString& value)
{
    QSqlQuery q(QSqlDatabase::database(conn));
    q.prepare("INSERT OR REPLACE INTO meta(k, v) VALUES(?, ?)");
    q.addBindValue(key);
    q.addBindValue(value);
    if (!q.exec())
    {
        err = q.lastError().text();
        return false;
    }
    return true;
}

Settings Store::settings()
{
    Settings s;
    auto d = [this](const char* k, double def) { bool ok; double v = meta(k).toDouble(&ok); return ok ? v : def; };
    s.gosNumber     = meta("gos_number");
    s.organization  = meta("organization");
    s.rpmOn         = d("rpm_on", s.rpmOn);
    s.speedMove     = d("speed_move", s.speedMove);
    s.hydroOver     = d("hydro_over", s.hydroOver);
    s.coolantOver   = d("coolant_over", s.coolantOver);
    s.tankL         = d("tank_l", s.tankL);
    s.refuelPct     = d("refuel_pct", s.refuelPct);
    s.drainPct      = d("drain_pct", s.drainPct);
    s.fuelWindowMin = int(d("fuel_window_min", s.fuelWindowMin));
    s.residualPct   = d("residual_pct", s.residualPct);
    s.keepDays      = int(d("keep_days", s.keepDays));
    return s;
}

bool Store::saveSettings(const Settings& s)
{
    QSqlDatabase db = QSqlDatabase::database(conn);
    db.transaction();
    bool ok = setMeta("gos_number", s.gosNumber)
           && setMeta("organization", s.organization)
           && setMeta("rpm_on", QString::number(s.rpmOn))
           && setMeta("speed_move", QString::number(s.speedMove))
           && setMeta("hydro_over", QString::number(s.hydroOver))
           && setMeta("coolant_over", QString::number(s.coolantOver))
           && setMeta("tank_l", QString::number(s.tankL))
           && setMeta("refuel_pct", QString::number(s.refuelPct))
           && setMeta("drain_pct", QString::number(s.drainPct))
           && setMeta("fuel_window_min", QString::number(s.fuelWindowMin))
           && setMeta("residual_pct", QString::number(s.residualPct))
           && setMeta("keep_days", QString::number(s.keepDays));
    if (ok)
        ok = db.commit();
    else
        db.rollback();
    return ok;
}

ScheduleSet Store::schedules()
{
    QVector<Schedule> list;
    QSqlQuery q(QSqlDatabase::database(conn));
    if (q.exec("SELECT from_day, cnt, s1, s2, s3 FROM schedule"))
        while (q.next())
        {
            Schedule s;
            s.fromDay = QDate::fromString(q.value(0).toString(), Qt::ISODate);
            s.count = q.value(1).toInt();
            s.start[0] = q.value(2).toInt();
            s.start[1] = q.value(3).toInt();
            s.start[2] = q.value(4).toInt();
            list.append(s);
        }
    return ScheduleSet(list);
}

bool Store::saveSchedule(const Schedule& s)
{
    QSqlQuery q(QSqlDatabase::database(conn));
    q.prepare("INSERT OR REPLACE INTO schedule(from_day, cnt, s1, s2, s3) VALUES(?, ?, ?, ?, ?)");
    q.addBindValue(s.fromDay.isValid() ? s.fromDay.toString(Qt::ISODate) : QString("0000-00-00"));
    q.addBindValue(s.count);
    q.addBindValue(s.start[0]);
    q.addBindValue(s.start[1]);
    q.addBindValue(s.start[2]);
    if (!q.exec())
    {
        err = q.lastError().text();
        return false;
    }
    return true;
}

// ---------------- запись ----------------
qint64 Store::beginCycle(qint64 start)
{
    QSqlQuery q(QSqlDatabase::database(conn));
    q.prepare("INSERT INTO cycle(start, end, reason) VALUES(?, ?, ?)");
    q.addBindValue(start);
    q.addBindValue(start);
    q.addBindValue(QString::fromUtf8("нет данных"));
    if (!q.exec())
    {
        err = q.lastError().text();
        return 0;
    }
    return q.lastInsertId().toLongLong();
}

static bool insertEvent(QSqlQuery& q, const EventRec& e)
{
    q.addBindValue(e.t);
    q.addBindValue(e.type);
    q.addBindValue(e.severity);
    q.addBindValue(e.code);
    q.addBindValue(nv(e.value));
    q.addBindValue(nv(e.value2));
    q.addBindValue(nv(e.value3));
    q.addBindValue(e.mode);
    q.addBindValue(e.keyOn ? 1 : 0);
    q.addBindValue(e.text);
    return q.exec();
}

bool Store::writeEvents(const QVector<EventRec>& events)
{
    if (events.isEmpty())
        return true;
    QSqlDatabase db = QSqlDatabase::database(conn);
    db.transaction();
    QSqlQuery q(db);
    q.prepare("INSERT INTO event(t, type, severity, code, value, value2, value3, mode, key_on, text) VALUES(?,?,?,?,?,?,?,?,?,?)");
    for (const EventRec& e : events)
        if (!insertEvent(q, e))
        {
            err = q.lastError().text();
            db.rollback();
            return false;
        }
    return db.commit();
}

bool Store::writeMinute(const MinuteRec& m, const QVector<EventRec>& events, qint64 cycleId, qint64 cycleEnd, const QString& cycleReason)
{
    QSqlDatabase db = QSqlDatabase::database(conn);
    db.transaction();
    QSqlQuery q(db);
    q.prepare("INSERT OR REPLACE INTO minute(t, s_clean, s_move, s_idle, s_off, s_nolink, s_key, eq0, eq1, eq2, eq3, eq4, eq5, eq6,"
              " rpm_avg, rpm_max, spd_avg, spd_max, dist_m, dist_clean_m, fuel_l, fuel_clean_l, fuel_level, coolant_max, oilp_min,"
              " hydro_avg, hydro_max, hydro_over_s, coolant_over_s, engine_h0, engine_h, ecu_h, odo_km, lfc_l, lat, lon, gnss_fix, time_invalid)"
              " VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
    q.addBindValue(m.t);
    for (int i = 0; i < ModeCount; ++i)
        q.addBindValue(m.secMode[i]);
    q.addBindValue(m.secKey);
    for (int i = 0; i < EqCount; ++i)
        q.addBindValue(m.secEq[i]);
    for (double v : {m.rpmAvg, m.rpmMax, m.speedAvg, m.speedMax, m.distM, m.distCleanM, m.fuelL, m.fuelCleanL, m.fuelLevel,
                     m.coolantMax, m.oilPMin, m.hydroAvg, m.hydroMax})
        q.addBindValue(nv(v));
    q.addBindValue(m.hydroOverSec);
    q.addBindValue(m.coolantOverSec);
    for (double v : {m.engineH0, m.engineH, m.ecuH, m.odoKm, m.lfcL, m.lat, m.lon})
        q.addBindValue(nv(v));
    q.addBindValue(m.gnssFix < 0 ? QVariant(QVariant::Int) : QVariant(m.gnssFix));
    q.addBindValue(m.timeInvalid ? 1 : 0);
    bool ok = q.exec();
    if (!ok)
        err = q.lastError().text();

    if (ok && !events.isEmpty())
    {
        QSqlQuery e(db);
        e.prepare("INSERT INTO event(t, type, severity, code, value, value2, value3, mode, key_on, text) VALUES(?,?,?,?,?,?,?,?,?,?)");
        for (const EventRec& ev : events)
            if (!insertEvent(e, ev)) { ok = false; err = e.lastError().text(); break; }
    }
    if (ok && cycleId > 0)
    {
        QSqlQuery c(db);
        c.prepare("UPDATE cycle SET end=?, reason=? WHERE id=?");
        c.addBindValue(cycleEnd);
        c.addBindValue(cycleReason);
        c.addBindValue(cycleId);
        if (!c.exec()) { ok = false; err = c.lastError().text(); }
    }
    if (ok)
        ok = db.commit();
    else
        db.rollback();
    return ok;
}

bool Store::updateCycle(qint64 cycleId, qint64 end, const QString& reason)
{
    if (cycleId <= 0)
        return true;
    QSqlQuery c(QSqlDatabase::database(conn));
    c.prepare("UPDATE cycle SET end=?, reason=? WHERE id=?");
    c.addBindValue(end);
    c.addBindValue(reason);
    c.addBindValue(cycleId);
    if (!c.exec())
    {
        err = c.lastError().text();
        return false;
    }
    return true;
}

int Store::purge(int keepDays, qint64 now)
{
    if (keepDays <= 0)
        return 0;
    const qint64 edge = now - qint64(keepDays) * 86400;
    QSqlQuery q(QSqlDatabase::database(conn));
    int n = 0;
    q.prepare("DELETE FROM minute WHERE t < ?");
    q.addBindValue(edge);
    if (q.exec()) n += q.numRowsAffected();
    q.prepare("DELETE FROM event WHERE t < ?");
    q.addBindValue(edge);
    if (q.exec()) n += q.numRowsAffected();
    q.prepare("DELETE FROM cycle WHERE end < ?");
    q.addBindValue(edge);
    if (q.exec()) n += q.numRowsAffected();
    return n;
}

// ---------------- чтение ----------------
QVector<MinuteRec> Store::minutes(qint64 from, qint64 to)
{
    QVector<MinuteRec> out;
    QSqlQuery q(QSqlDatabase::database(conn));
    q.setForwardOnly(true);
    q.prepare("SELECT t, s_clean, s_move, s_idle, s_off, s_nolink, s_key, eq0, eq1, eq2, eq3, eq4, eq5, eq6,"
              " rpm_avg, rpm_max, spd_avg, spd_max, dist_m, dist_clean_m, fuel_l, fuel_clean_l, fuel_level, coolant_max, oilp_min,"
              " hydro_avg, hydro_max, hydro_over_s, coolant_over_s, engine_h0, engine_h, ecu_h, odo_km, lfc_l, lat, lon, gnss_fix, time_invalid"
              " FROM minute WHERE t >= ? AND t < ? ORDER BY t");
    q.addBindValue(from);
    q.addBindValue(to);
    if (!q.exec())
    {
        err = q.lastError().text();
        return out;
    }
    while (q.next())
    {
        MinuteRec m;
        int c = 0;
        m.t = q.value(c++).toLongLong();
        for (int i = 0; i < ModeCount; ++i) m.secMode[i] = q.value(c++).toInt();
        m.secKey = q.value(c++).toInt();
        for (int i = 0; i < EqCount; ++i) m.secEq[i] = q.value(c++).toInt();
        double* dst[] = {&m.rpmAvg, &m.rpmMax, &m.speedAvg, &m.speedMax, &m.distM, &m.distCleanM, &m.fuelL, &m.fuelCleanL,
                         &m.fuelLevel, &m.coolantMax, &m.oilPMin, &m.hydroAvg, &m.hydroMax};
        for (double* p : dst) *p = dv(q.value(c++));
        if (!hasValue(m.distM)) m.distM = 0;
        if (!hasValue(m.distCleanM)) m.distCleanM = 0;
        m.hydroOverSec = q.value(c++).toInt();
        m.coolantOverSec = q.value(c++).toInt();
        double* dst2[] = {&m.engineH0, &m.engineH, &m.ecuH, &m.odoKm, &m.lfcL, &m.lat, &m.lon};
        for (double* p : dst2) *p = dv(q.value(c++));
        m.gnssFix = q.value(c).isNull() ? -1 : q.value(c).toInt();
        ++c;
        m.timeInvalid = q.value(c++).toInt() != 0;
        out.append(m);
    }
    return out;
}

QVector<EventRec> Store::events(qint64 from, qint64 to)
{
    QVector<EventRec> out;
    QSqlQuery q(QSqlDatabase::database(conn));
    q.setForwardOnly(true);
    q.prepare("SELECT id, t, type, severity, code, value, value2, value3, mode, key_on, text FROM event WHERE t >= ? AND t < ? ORDER BY t, id");
    q.addBindValue(from);
    q.addBindValue(to);
    if (!q.exec())
    {
        err = q.lastError().text();
        return out;
    }
    while (q.next())
    {
        EventRec e;
        e.id = q.value(0).toLongLong();
        e.t = q.value(1).toLongLong();
        e.type = q.value(2).toInt();
        e.severity = q.value(3).toInt();
        e.code = q.value(4).toLongLong();
        e.value = dv(q.value(5));
        e.value2 = dv(q.value(6));
        e.value3 = dv(q.value(7));
        e.mode = q.value(8).toInt();
        e.keyOn = q.value(9).toInt() != 0;
        e.text = q.value(10).toString();
        out.append(e);
    }
    return out;
}

QVector<CycleRec> Store::cycles(qint64 from, qint64 to)
{
    QVector<CycleRec> out;
    QSqlQuery q(QSqlDatabase::database(conn));
    q.prepare("SELECT id, start, end, reason FROM cycle WHERE end >= ? AND start < ? ORDER BY start");
    q.addBindValue(from);
    q.addBindValue(to);
    if (q.exec())
        while (q.next())
        {
            CycleRec c;
            c.id = q.value(0).toLongLong();
            c.start = q.value(1).toLongLong();
            c.end = q.value(2).toLongLong();
            c.endReason = q.value(3).toString();
            out.append(c);
        }
    return out;
}

qint64 Store::firstMinute()
{
    QSqlQuery q(QSqlDatabase::database(conn));
    if (q.exec("SELECT MIN(t) FROM minute") && q.next() && !q.value(0).isNull())
        return q.value(0).toLongLong();
    return 0;
}

} // namespace WJ
