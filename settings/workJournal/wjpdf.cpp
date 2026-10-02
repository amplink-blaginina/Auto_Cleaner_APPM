/*
 * Журнал работы — PDF-отчёты (см. wjpdf.h)
 * Версия: 02, 2026-09-29
 * Изменения от 01: строка и колонка «без системы» в отчётах
 *
 * Разрешение 254 dpi: 1 мм = 10 точек. Раскладка в два прохода: первый считает страницы («Стр. N из M»),
 * второй рисует. Строка таблицы не разрывается, шапка таблицы повторяется на новой странице.
 */
#include "wjpdf.h"
#include "wjdefaults.h"
#include <QPdfWriter>
#include <QPainter>
#include <QPageSize>
#include <QFontMetricsF>
#include <QDir>
#include <QFile>
#include <functional>

namespace WJ
{

namespace
{
const qreal kL = 150, kR = 1970, kT = 120, kB = 2790;   // поля: слева 15, справа 14, сверху 12, снизу ~18 мм
const QColor cInk(0x1b, 0x1f, 0x24), cGray(0x6b, 0x70, 0x75), cLine(0xe3, 0xe6, 0xea), cLine2(0x9a, 0xa0, 0xa6),
             cBox(0xcf, 0xd4, 0xda), cTile(0xf2, 0xf4, 0xf6), cRed(0xc9, 0x28, 0x1f), cGreen(0x22, 0xa4, 0x47),
             cOrange(0xe0, 0x9a, 0x00);

QString U(const char* s) { return QString::fromUtf8(s); }

struct Doc
{
    QPdfWriter& w;
    QPainter*   p;
    QString     family;
    QString     footer;
    int         page = 1, total = 1;
    qreal       y = kT;
    std::function<void()> onNewPage;

    Doc(QPdfWriter& w, QPainter* p, const QString& fam) : w(w), p(p), family(fam) {}

    QFont font(qreal pt, bool bold) const
    {
        QFont f;
        if (!family.isEmpty())
            f.setFamily(family);
        f.setPointSizeF(pt);
        f.setBold(bold);
        return f;
    }
    qreal width(const QString& s, qreal pt, bool bold) const { return QFontMetricsF(font(pt, bold), &w).horizontalAdvance(s); }
    void text(const QRectF& r, const QString& s, qreal pt, bool bold, const QColor& c, int align = Qt::AlignLeft | Qt::AlignVCenter)
    {
        if (!p) return;
        p->setFont(font(pt, bold));
        p->setPen(c);
        p->drawText(r, align | Qt::TextSingleLine, s);
    }
    void fill(const QRectF& r, const QColor& c, qreal radius = 0)
    {
        if (!p) return;
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->setPen(Qt::NoPen);
        p->setBrush(c);
        if (radius > 0) p->drawRoundedRect(r, radius, radius); else p->drawRect(r);
        p->restore();
    }
    void hline(qreal x1, qreal x2, qreal yy, const QColor& c, qreal wdt = 3)
    {
        if (!p) return;
        p->setPen(QPen(c, wdt));
        p->drawLine(QPointF(x1, yy), QPointF(x2, yy));
    }
    void box(const QRectF& r, const QColor& c, qreal radius)
    {
        if (!p) return;
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->setPen(QPen(c, 3));
        p->setBrush(Qt::NoBrush);
        p->drawRoundedRect(r, radius, radius);
        p->restore();
    }
    void dot(qreal cx, qreal cy, const QColor& c, qreal d = 24)
    {
        if (!p) return;
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->setPen(Qt::NoPen);
        p->setBrush(c);
        p->drawEllipse(QPointF(cx, cy), d / 2, d / 2);
        p->restore();
    }
    void finishPage()
    {
        if (!p) return;
        hline(kL, kR, 2855, cLine2, 3);
        text(QRectF(kL, 2865, 1500, 40), footer, 7, false, cGray);
        text(QRectF(kR - 400, 2865, 400, 40), U("Стр. %1 из %2").arg(page).arg(total), 7, false, cGray, Qt::AlignRight | Qt::AlignVCenter);
    }
    void newPage()
    {
        finishPage();
        if (p) w.newPage();
        ++page;
        y = kT;
        if (onNewPage) onNewPage();
    }
    void ensure(qreal h) { if (y + h > kB) newPage(); }
};

struct Col { QString title; qreal frac; int align; };

// Таблица с повтором шапки. rowColor(row, col) — цвет текста ячейки (невалидный — обычный)
void table(Doc& d, const QVector<Col>& cols, const QVector<QStringList>& rows, const QStringList& total,
           std::function<QColor(int, int)> rowColor = nullptr, std::function<bool(int, int)> rowBold = nullptr,
           std::function<void(Doc&, int, int, const QRectF&)> cellPainter = nullptr)
{
    const qreal W = kR - kL, rowH = 54, headH = 50;
    QVector<qreal> xs;
    qreal x = kL;
    for (const Col& c : cols) { xs.append(x); x += c.frac * W; }
    auto header = [&]()
    {
        for (int i = 0; i < cols.size(); ++i)
            d.text(QRectF(xs[i] + 20, d.y, cols[i].frac * W - 40, headH), cols[i].title.toUpper(), 7, false, cGray, cols[i].align | Qt::AlignVCenter);
        d.y += headH;
        d.hline(kL, kR, d.y, cLine2, 3);
    };
    d.ensure(headH + rowH);
    header();
    auto prevHook = d.onNewPage;
    d.onNewPage = header;
    for (int r = 0; r < rows.size(); ++r)
    {
        d.ensure(rowH);
        for (int i = 0; i < cols.size() && i < rows[r].size(); ++i)
        {
            const QRectF cell(xs[i] + 20, d.y, cols[i].frac * W - 40, rowH);
            if (cellPainter && rows[r][i] == "@")
            {
                cellPainter(d, r, i, cell);
                continue;
            }
            QColor c = rowColor ? rowColor(r, i) : QColor();
            bool b = rowBold ? rowBold(r, i) : false;
            d.text(cell, rows[r][i], 9, b, c.isValid() ? c : cInk, cols[i].align | Qt::AlignVCenter);
        }
        d.y += rowH;
        d.hline(kL, kR, d.y, cLine, 2);
    }
    if (!total.isEmpty())
    {
        d.ensure(rowH);
        d.hline(kL, kR, d.y, cInk, 5);
        for (int i = 0; i < cols.size() && i < total.size(); ++i)
            d.text(QRectF(xs[i] + 20, d.y, cols[i].frac * W - 40, rowH), total[i], 9, true, cInk, cols[i].align | Qt::AlignVCenter);
        d.y += rowH;
    }
    d.onNewPage = prevHook;
}

void h2(Doc& d, const QString& t)
{
    d.ensure(160);
    d.y += 38;
    d.text(QRectF(kL, d.y, kR - kL, 50), t.toUpper(), 10, true, cInk);
    d.y += 52;
    d.hline(kL, kR, d.y, cBox, 3);
    d.y += 12;
}

// Шапка: заголовок, логотип, реквизиты
void top(Doc& d, const QString& title, const QString& sub, const Settings& st, const PdfInfo& info)
{
    d.text(QRectF(kL, d.y, 1200, 80), title, 17, true, cInk);
    d.text(QRectF(kL, d.y + 80, 1300, 50), sub, 10.5, false, cInk);
    if (d.p)
        drawLogo(*d.p, QRectF(kR - 500, d.y + 8, 500, 85));
    d.text(QRectF(kR - 900, d.y + 95, 900, 40), U("Журнал работы машины · Сформирован: ") + info.created.toString("dd.MM.yyyy HH:mm"),
           8, false, cGray, Qt::AlignRight | Qt::AlignVCenter);
    d.y += 150;
    d.hline(kL, kR, d.y, cInk, 6);
    d.y += 30;

    const qreal H = 110, W = kR - kL;
    const qreal fr[] = {0.38, 0.24, 0.38};
    const QString lab[] = {U("Машина"), U("Гос. номер"), U("Организация")};
    const QString val[] = {info.machineTitle.isEmpty() ? U("—") : info.machineTitle, st.gosNumber, st.organization};
    d.box(QRectF(kL, d.y, W, H), cBox, 20);
    qreal x = kL;
    for (int i = 0; i < 3; ++i)
    {
        const qreal cw = fr[i] * W;
        if (i > 0 && d.p) { d.p->setPen(QPen(cBox, 3)); d.p->drawLine(QPointF(x, d.y), QPointF(x, d.y + H)); }
        d.text(QRectF(x + 30, d.y + 12, cw - 60, 36), lab[i].toUpper(), 7, false, cGray);
        if (val[i].isEmpty())
            d.hline(x + 30, x + cw - 40, d.y + 92, cInk, 3);       // пусто — графа для записи от руки
        else
            d.text(QRectF(x + 30, d.y + 48, cw - 60, 50), val[i], 10, true, cInk);
        x += cw;
    }
    d.y += H + 40;
}

struct Kpi { QString label, value, unit, sub; };

void kpis(Doc& d, const QVector<Kpi>& k)
{
    const qreal gap = 25, W = (kR - kL - gap * (k.size() - 1)) / k.size(), H = 165;
    qreal x = kL;
    for (const Kpi& t : k)
    {
        d.fill(QRectF(x, d.y, W, H), cTile, 20);
        d.text(QRectF(x + 30, d.y + 18, W - 60, 36), t.label.toUpper(), 7, false, cGray);
        d.text(QRectF(x + 30, d.y + 56, W - 60, 64), t.value, 14, true, cInk);
        const qreal vw = d.width(t.value, 14, true);
        if (!t.unit.isEmpty())
            d.text(QRectF(x + 30 + vw + 12, d.y + 64, W - 60 - vw, 56), t.unit, 8.5, false, cGray);
        d.text(QRectF(x + 30, d.y + 120, W - 60, 36), t.sub, 7, false, cGray);
        x += W + gap;
    }
    d.y += H;
}

void signatures(Doc& d, const QString& a, const QString& b)
{
    d.ensure(140);
    d.y += 60;
    const qreal W = (kR - kL - 120) / 2;
    for (int i = 0; i < 2; ++i)
    {
        const qreal x = kL + i * (W + 120);
        d.text(QRectF(x, d.y, W, 60), (i == 0 ? a : b) + U(": ______________________ / ______________"), 8, false, cGray,
               Qt::AlignLeft | Qt::AlignBottom);
        d.hline(x, x + W, d.y + 66, cInk, 3);
    }
    d.y += 80;
}

QString footerSources() { return U(" · Журнал работы v1 · источники: ЭБУ (J1939), БУЦ (CAN); GNSS — нет данных"); }

QString sevTag(const EventRec& e)
{
    if (isEndMark(e)) return QString();
    if (e.severity == SevAlarm) return U("АВАРИЯ");
    if (e.severity == SevWarn) return U("ПРЕДУПР.");
    return QString();
}

bool render(QPdfWriter& w, const PdfInfo& info, std::function<void(Doc&)> body)
{
    Doc dry(w, nullptr, info.fontFamily);
    body(dry);
    QPainter painter;
    if (!painter.begin(&w))
        return false;
    Doc d(w, &painter, info.fontFamily);
    d.total = dry.page;
    body(d);
    d.finishPage();
    painter.end();
    return true;
}

bool openWriter(QPdfWriter& w, const QString& title)
{
    w.setPageSize(QPageSize(QPageSize::A4));
    w.setPageMargins(QMarginsF(0, 0, 0, 0));
    w.setResolution(254);
    w.setTitle(title);
    w.setCreator(U("Auto_Cleaner — Журнал работы"));
    return true;
}

QString prepareFile(const PdfInfo& info, const QString& name, QString* err)
{
    if (!QDir().mkpath(info.exportDir))
    {
        if (err) *err = U("нет каталога ") + info.exportDir;
        return QString();
    }
    return QDir(info.exportDir).filePath(name);
}

void appendManifest(const PdfInfo& info, const QString& file, const QString& kind)
{
    QFile f(QDir(info.exportDir).filePath("manifest.txt"));
    if (f.open(QIODevice::Append | QIODevice::Text))
        f.write((info.created.toString(Qt::ISODate) + ";" + QFileInfo(file).fileName() + ";" + kind + "\n").toUtf8());
}

} // namespace

QString pdfPlate(const QString& gos)
{
    static const QString ru = U("АВЕКМНОРСТУХ");
    static const QString en = "ABEKMHOPCTYX";
    QString out;
    for (QChar c : gos.toUpper())
    {
        int i = ru.indexOf(c);
        if (i >= 0) out += en[i];
        else if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) out += c;
    }
    return out.isEmpty() ? QString("noreg") : out;
}

// ============================ смена ============================
QString writeShiftPdf(const ShiftReport& r, const PdfInfo& info, QString* err)
{
    const QString name = QString("WJ_%1_%2_S%3.pdf").arg(pdfPlate(r.st.gosNumber), r.slot.day.toString("yyyyMMdd")).arg(r.slot.no);
    const QString path = prepareFile(info, name, err);
    if (path.isEmpty())
        return QString();
    QPdfWriter w(path);
    openWriter(w, U("Отчёт за смену"));
    const Totals& t = r.tot;
    const qint64 b = r.slot.begin.toSecsSinceEpoch(), e = r.slot.end.toSecsSinceEpoch();

    auto body = [&](Doc& d)
    {
        d.footer = U("Отчёт за смену %1, %2 · пульт").arg(r.slot.no).arg(r.slot.day.toString("dd.MM.yyyy")) + footerSources();
        QString sub = U("Смена %1 · %2 · %3 – %4").arg(r.slot.no).arg(r.slot.day.toString("dd.MM.yyyy"),
                        r.slot.begin.toString("HH:mm"), r.slot.end.toString("HH:mm"));
        if (r.open)
            sub += U(" · смена не закрыта, данные на ") + r.dataAt.toString("HH:mm");
        top(d, U("ОТЧЁТ ЗА СМЕНУ"), sub, r.st, info);
        kpis(d, {
            {U("Уборка"), fmtHM(t.secMode[ModeClean]), "", fmtPct(t.secMode[ModeClean], t.secEngine()) + U(" от работы ДВС")},
            {U("Работа ДВС"), fmtHM(t.secEngine()), "", U("ключ вкл. ") + fmtHM(t.secKey)},
            {U("Пробег"), fmtNum(t.distM / 1000.0), U("км"), U("в уборке ") + fmtNum(t.distCleanM / 1000.0)},
            {U("Топливо"), fmtNum(t.fuelL), U("л"), fmtNum(t.fuelPerCleanHour()) + U(" л/ч в уборке")},
            {U("Моточасы"), fmtNum(t.engineH()), U("м/ч"), fmtNum(t.engineHStart) + U(" → ") + fmtNum(t.engineHEnd)}});

        // шкала режимов
        h2(d, U("Режимы работы по времени"));
        d.ensure(160);
        const qreal W = kR - kL, H = 70;
        d.fill(QRectF(kL, d.y, W, H), modeColorPrint(ModeKeyOff), 10);
        const qreal span = qMax<qint64>(60, e - b);
        int runMode = -2; qreal runX = 0;
        auto flushRun = [&](qreal x2) { if (runMode >= 0 && runMode != ModeKeyOff) d.fill(QRectF(runX, d.y, x2 - runX + 0.5, H), modeColorPrint(runMode)); };
        for (const MinuteRec& m : r.minutes)
        {
            const qreal x = kL + (m.t - b) / span * W;
            const int md = m.dominantMode();
            if (md != runMode || x - runX > W / span * 61 + 1) { flushRun(x); runMode = md; runX = x; }
            runX = qMin(runX, x);
            flushRun(x + W / span * 60);
            runMode = md; runX = x + W / span * 60;
            runMode = -2;
        }
        const int hours = int(span / 3600);
        const int step = hours > 12 ? 2 : 1;
        for (int i = 0; i <= hours; i += step)
        {
            const qreal x = kL + i * 3600.0 / span * W;
            d.text(QRectF(x - 60, d.y + H + 6, 120, 34), r.slot.begin.addSecs(i * 3600).toString("HH:mm"), 7, false, cGray, Qt::AlignCenter);
        }
        d.y += H + 46;
        qreal lx = kL;
        for (int m = 0; m <= ModeKeyOff; ++m)
        {
            d.dot(lx + 12, d.y + 18, modeColorPrint(m));
            d.text(QRectF(lx + 32, d.y, 400, 36), modeName(m), 7.5, false, cInk);
            lx += 32 + d.width(modeName(m), 7.5, false) + 45;
        }
        d.y += 50;

        // режимы | циклы ключа
        const qreal y0 = d.y;
        const qreal colW = (W - 60) / 2;
        auto miniTable = [&](qreal x, const QVector<Col>& cols, const QVector<QStringList>& rows, const QStringList& tot,
                             std::function<QColor(int)> dotColor)
        {
            qreal yy = y0;
            QVector<qreal> xs; qreal cx = x;
            for (const Col& c : cols) { xs.append(cx); cx += c.frac * colW; }
            for (int i = 0; i < cols.size(); ++i)
                d.text(QRectF(xs[i] + 20, yy, cols[i].frac * colW - 40, 50), cols[i].title.toUpper(), 7, false, cGray, cols[i].align | Qt::AlignVCenter);
            yy += 50;
            d.hline(x, x + colW, yy, cLine2, 3);
            for (int rI = 0; rI < rows.size(); ++rI)
            {
                for (int i = 0; i < cols.size(); ++i)
                {
                    qreal ox = 0;
                    if (i == 0 && dotColor && dotColor(rI).isValid()) { d.dot(xs[0] + 32, yy + 27, dotColor(rI)); ox = 34; }
                    d.text(QRectF(xs[i] + 20 + ox, yy, cols[i].frac * colW - 40 - ox, 54), rows[rI][i], 9, false, cInk, cols[i].align | Qt::AlignVCenter);
                }
                yy += 54;
                d.hline(x, x + colW, yy, cLine, 2);
            }
            if (!tot.isEmpty())
            {
                d.hline(x, x + colW, yy, cInk, 5);
                for (int i = 0; i < cols.size(); ++i)
                    d.text(QRectF(xs[i] + 20, yy, cols[i].frac * colW - 40, 54), tot[i], 9, true, cInk, cols[i].align | Qt::AlignVCenter);
                yy += 54;
            }
            return yy;
        };
        QVector<QStringList> mrows;
        const int order[] = {ModeClean, ModeMove, ModeIdle, ModeNoLink, ModeOff};
        for (int m : order)
            mrows.append({modeName(m), fmtHM(t.secMode[m]), (m == ModeOff || m == ModeNoLink) ? fmtPct(t.secMode[m], t.secKey) : fmtPct(t.secMode[m], t.secEngine())});
        const int keyOffSec = qMax<qint64>(0, qMin<qint64>(e, r.dataAt.toSecsSinceEpoch()) - b) - t.secKey;
        mrows.append({modeName(ModeKeyOff), fmtHM(qMax(0, keyOffSec)), U("—")});
        d.ensure(560);
        qreal ya = miniTable(kL, {{U("Режим"), 0.56, Qt::AlignLeft}, {U("Время"), 0.22, Qt::AlignRight}, {U("Доля"), 0.22, Qt::AlignRight}}, mrows,
                             {U("Ключ включён"), fmtHM(t.secKey), ""},
                             [&](int i) { return modeColorPrint(i < 5 ? order[i] : ModeKeyOff); });
        d.text(QRectF(kL, ya + 6, colW, 36), U("Доля: уборка, движение, х.х. — от работы ДВС (%1), прочее — от ключа.").arg(fmtHM(t.secEngine())), 6.5, false, cGray);
        QVector<QStringList> crows;
        for (const CycleRec& c : r.cycles)
            crows.append({QDateTime::fromSecsSinceEpoch(qMax(c.start, b)).toString("HH:mm"),
                          QDateTime::fromSecsSinceEpoch(qMin(c.end, e)).toString("HH:mm"),
                          fmtHM(int(qMin(c.end, e) - qMax(c.start, b))), c.endReason});
        qreal yb = miniTable(kL + colW + 60, {{U("Включение"), 0.26, Qt::AlignLeft}, {U("Выключение"), 0.28, Qt::AlignLeft},
                                              {U("Длит."), 0.2, Qt::AlignRight}, {U("Способ"), 0.26, Qt::AlignLeft}}, crows, {}, nullptr);
        d.text(QRectF(kL + colW + 60, yb + 6, colW, 36), U("Циклов ключа в смене: %1.").arg(r.cycles.size()), 7, false, cGray);
        d.y = qMax(ya, yb) + 50;

        // оборудование | двигатель и топливо
        d.ensure(520);
        const qreal y1 = d.y;
        auto kvBlock = [&](qreal x, const QString& title, const QVector<QPair<QString, QString>>& kv)
        {
            qreal yy = y1 + 38;
            d.text(QRectF(x, yy, colW, 50), title.toUpper(), 10, true, cInk);
            yy += 52;
            d.hline(x, x + colW, yy, cBox, 3);
            yy += 12;
            for (const auto& p : kv)
            {
                d.text(QRectF(x + 20, yy, colW * 0.6, 50), p.first, 9, false, cInk);
                d.text(QRectF(x + colW * 0.5, yy, colW * 0.5 - 20, 50), p.second, 9, true, cInk, Qt::AlignRight | Qt::AlignVCenter);
                yy += 50;
                d.hline(x, x + colW, yy, cLine, 2);
            }
            return yy;
        };
        QVector<QPair<QString, QString>> eq;
        for (int i = 0; i < EqCount; ++i)
            eq.append({equipName(i), fmtHM(t.secEq[i])});
        eq.append({U("Т гидробака: макс. / выше %1 °C").arg(fmtNum(r.st.hydroOver, 0)),
                   (hasValue(t.hydroMax) ? fmtNum(t.hydroMax, 0) + U(" °C") : U("—")) + U(" / ") + QString::number((t.hydroOverSec + 30) / 60) + U(" мин")});
        const double res = t.residualL(r.st.tankL);
        QVector<QPair<QString, QString>> en = {
            {U("Скорость уборки ср. / макс."), fmtNum(t.speedCleanAvg) + " / " + fmtNum(t.speedMax) + U(" км/ч")},
            {U("Обороты ср. / макс."), fmtInt(t.rpmAvg) + " / " + fmtInt(t.rpmMax) + U(" об/мин")},
            {U("Т ОЖ макс."), hasValue(t.coolantMax) ? fmtNum(t.coolantMax, 0) + U(" °C") : U("—")},
            {U("Давление масла мин."), hasValue(t.oilPMin) ? fmtNum(t.oilPMin / 100.0, 1) + U(" бар") : U("—")},
            {U("Топливо: уровень нач. → кон."), fmtNum(t.levelStart, 0) + U(" % → ") + fmtNum(t.levelEnd, 0) + " %"},
            {U("Заправки / сливы, л"), (t.refuelCount ? fmtNum(t.refuelL) : U("нет")) + " / " + (t.drainCount ? fmtNum(t.drainL) : U("нет"))},
            {U("Шасси без системы: пробег / топливо"), t.offCount ? fmtNum(t.offKm) + U(" км / ") + fmtNum(t.offFuelL) + U(" л") : U("нет")},
            {U("Невязка баланса"), hasValue(res) ? fmtNum(res) + U(" л") : U("—")}};
        qreal yc = kvBlock(kL, U("Уборочное оборудование"), eq);
        qreal yd = kvBlock(kL + colW + 60, U("Двигатель и топливо"), en);
        d.y = qMax(yc, yd) + 10;

        // события
        h2(d, U("События"));
        QVector<QStringList> er;
        QVector<EventRec> evs;
        for (const EventRec& ev : r.events)
        {
            er.append({QDateTime::fromSecsSinceEpoch(ev.t).toString("HH:mm:ss"), eventText(ev), sevTag(ev)});
            evs.append(ev);
        }
        if (er.isEmpty())
        {
            d.text(QRectF(kL + 20, d.y, kR - kL, 50), U("Событий за смену нет."), 9, false, cGray);
            d.y += 60;
        }
        else
            table(d, {{U("Время"), 0.13, Qt::AlignLeft}, {U("Событие"), 0.72, Qt::AlignLeft}, {U("Уровень"), 0.15, Qt::AlignLeft}}, er, {},
                  [&](int row, int col) { return col == 2 ? (evs[row].severity == SevAlarm ? cRed : cOrange) : QColor(); },
                  [&](int, int col) { return col == 2; });
        signatures(d, U("Оператор"), U("Мастер участка"));
    };
    if (!render(w, info, body))
    {
        if (err) *err = U("не удалось создать PDF");
        return QString();
    }
    appendManifest(info, path, "shift");
    return path;
}

// ============================ период ============================
QString writePeriodPdf(const PeriodReport& r, const PdfInfo& info, QString* err)
{
    const QString name = QString("WJ_%1_%2-%3_P.pdf").arg(pdfPlate(r.st.gosNumber), r.sel.from.toString("yyyyMMdd"), r.sel.to.toString("yyyyMMdd"));
    const QString path = prepareFile(info, name, err);
    if (path.isEmpty())
        return QString();
    QPdfWriter w(path);
    openWriter(w, U("Отчёт за период"));
    const Totals& t = r.total;

    auto body = [&](Doc& d)
    {
        const QString range = r.sel.kind == PerDay
            ? U("Сутки %1 · %2 – %3").arg(r.sel.from.toString("dd.MM.yyyy"), r.begin.toString("HH:mm"), r.end.toString("dd.MM.yyyy HH:mm"))
            : U("%1 – %2 · %3 суток").arg(r.sel.from.toString("dd.MM.yyyy"), r.sel.to.toString("dd.MM.yyyy")).arg(r.sel.days());
        d.footer = U("Отчёт за период ") + r.sel.from.toString("dd.MM.yyyy") + (r.sel.days() > 1 ? U("–") + r.sel.to.toString("dd.MM.yyyy") : QString())
                   + U(" · пульт") + footerSources();
        top(d, U("ОТЧЁТ ЗА ПЕРИОД"), range, r.st, info);
        const int days = qMax(1, r.daysWithWork);
        if (r.byShift)
            kpis(d, {{U("Уборка"), fmtHM(t.secMode[ModeClean]), "", fmtPct(t.secMode[ModeClean], t.secEngine()) + U(" от работы ДВС")},
                     {U("Моточасы"), (t.engineH() >= 0 ? "+" : "") + fmtNum(t.engineH()), U("м/ч"), fmtNum(t.engineHStart) + U(" → ") + fmtNum(t.engineHEnd)},
                     {U("Пробег"), fmtNum(t.distM / 1000.0), U("км"), U("в уборке ") + fmtNum(t.distCleanM / 1000.0)},
                     {U("Топливо"), fmtNum(t.fuelL), U("л"), fmtNum(t.fuelPerCleanHour()) + U(" л/ч в уборке")},
                     {U("События"), QString::number(t.events), "", U("%1 предупр. · %2 авар.").arg(t.warnEvents).arg(t.alarmEvents)}});
        else
            kpis(d, {{U("Уборка"), fmtHM(t.secMode[ModeClean]), "", U("ср. ") + fmtHM(t.secMode[ModeClean] / days) + U(" в день")},
                     {U("Моточасы"), (t.engineH() >= 0 ? "+" : "") + fmtNum(t.engineH()), U("м/ч"), fmtNum(t.engineHStart) + U(" → ") + fmtNum(t.engineHEnd)},
                     {U("Пробег"), fmtNum(t.distM / 1000.0), U("км"), t.offCount ? U("+ без системы ") + fmtNum(t.offKm) : U("ср. ") + fmtNum(t.distM / 1000.0 / days) + U(" в день")},
                     {U("Топливо"), fmtNum(t.fuelL), U("л"), hasValue(t.engineH()) && t.engineH() > 0 ? fmtNum(t.fuelL / t.engineH()) + U(" л/м·ч") : QString()},
                     {U("Дней с работой"), QString::number(r.daysWithWork), U("из %1").arg(r.rows.size()), ""}});

        h2(d, r.byShift ? U("По сменам") : U("По суткам"));
        QVector<QStringList> rows;
        for (const PeriodRow& row : r.rows)
            rows.append({row.label, fmtHM(row.tot.secMode[ModeClean]), fmtHM(row.tot.secEngine()), fmtNum(row.tot.distM / 1000.0),
                         fmtNum(row.tot.fuelL), fmtNum(row.tot.fuelPerCleanHour()), QString::number(row.tot.events)});
        table(d, {{r.byShift ? U("Смена") : U("Сутки"), 0.22, Qt::AlignLeft}, {U("Уборка"), 0.12, Qt::AlignRight}, {U("Работа ДВС"), 0.14, Qt::AlignRight},
                  {U("Пробег, км"), 0.13, Qt::AlignRight}, {U("Топливо, л"), 0.13, Qt::AlignRight}, {U("л/ч уборки"), 0.13, Qt::AlignRight},
                  {U("События"), 0.13, Qt::AlignRight}},
              rows, {U("Итого"), fmtHM(t.secMode[ModeClean]), fmtHM(t.secEngine()), fmtNum(t.distM / 1000.0), fmtNum(t.fuelL),
                     fmtNum(t.fuelPerCleanHour()), QString::number(t.events)});

        h2(d, U("Топливо: заправки и сливы"));
        if (r.fuelEvents.isEmpty())
        {
            d.text(QRectF(kL + 20, d.y, kR - kL, 50), U("За период заправок и сливов не зафиксировано."), 9, false, cGray);
            d.y += 60;
        }
        else
        {
            QVector<QStringList> fr;
            for (const EventRec& e : r.fuelEvents)
                fr.append({QDateTime::fromSecsSinceEpoch(e.t).toString("dd.MM HH:mm") + (e.keyOn ? QString() : "*"),
                           e.type == EvRefuel ? U("ЗАПРАВКА") : U("СЛИВ"),
                           e.keyOn ? U("вкл.") : U("выкл."),
                           fmtNum(e.value2, 0) + U(" → ") + fmtNum(e.value3, 0) + " %",
                           (e.type == EvRefuel ? "+" : QString(QChar(0x2212))) + fmtNum(e.value)});
            table(d, {{U("Дата, время"), 0.2, Qt::AlignLeft}, {U("Событие"), 0.16, Qt::AlignLeft}, {U("Ключ"), 0.3, Qt::AlignLeft},
                      {U("Уровень"), 0.17, Qt::AlignRight}, {U("Объём, л"), 0.17, Qt::AlignRight}}, fr, {},
                  [&](int row, int col) { return (col == 1 || col == 4) ? (r.fuelEvents[row].type == EvRefuel ? cGreen : cRed) : QColor(); },
                  [&](int, int col) { return col == 1 || col == 4; });
            bool any = false;
            for (const EventRec& e : r.fuelEvents) any = any || !e.keyOn;
            if (any)
            {
                d.text(QRectF(kL, d.y + 6, kR - kL, 36), U("* Событие при выключенном ключе: время — момент включения, уровень сравнивается с последним выключением."), 6.5, false, cGray);
                d.y += 44;
            }
        }

        h2(d, r.byShift ? U("Топливный баланс по сменам, л") : U("Топливный баланс по суткам, л"));
        QVector<QStringList> br;
        QVector<bool> badRes;
        const double thr = r.st.residualPct / 100.0 * r.st.tankL;
        for (const PeriodRow& row : r.rows)
        {
            const double res = row.tot.residualL(r.st.tankL);
            br.append({row.label, fmtNum(row.tot.fuelL), row.tot.offCount ? fmtNum(row.tot.offFuelL) : U("—"),
                       row.tot.refuelCount ? fmtNum(row.tot.refuelL) : U("—"), row.tot.drainCount ? fmtNum(row.tot.drainL) : U("—"), fmtNum(res)});
            badRes.append(hasValue(res) && qAbs(res) > thr);
        }
        const double resT = t.residualL(r.st.tankL);
        table(d, {{r.byShift ? U("Смена") : U("Сутки"), 0.24, Qt::AlignLeft}, {U("Расход ЭБУ"), 0.15, Qt::AlignRight}, {U("Без системы"), 0.16, Qt::AlignRight},
                  {U("Заправлено"), 0.15, Qt::AlignRight}, {U("Слито"), 0.15, Qt::AlignRight}, {U("Невязка"), 0.15, Qt::AlignRight}}, br,
              {U("Итого"), fmtNum(t.fuelL), fmtNum(t.offFuelL), fmtNum(t.refuelL), fmtNum(t.drainL), fmtNum(resT)},
              [&](int row, int col) { return ((col == 4 && r.rows[row].tot.drainCount) || (col == 5 && badRes[row])) ? cRed : QColor(); },
              [&](int row, int col) { return (col == 4 && r.rows[row].tot.drainCount) || (col == 5 && badRes[row]); });

        QStringList warn;
        for (const EventRec& e : r.fuelEvents)
            if (e.type == EvFuelDrain)
                warn << U("Слив %1: уровень упал на %2 л (%3 → %4 %). Требует проверки.")
                        .arg(QDateTime::fromSecsSinceEpoch(e.t).toString("dd.MM HH:mm"), fmtNum(e.value), fmtNum(e.value2, 0), fmtNum(e.value3, 0));
        for (int i = 0; i < r.rows.size(); ++i)
            if (badRes[i])
                warn << U("Невязка баланса %1: %2 л при пороге %3 л.").arg(r.rows[i].label, fmtNum(r.rows[i].tot.residualL(r.st.tankL)), fmtNum(thr));
        if (t.timeInvalid)
            warn << U("В периоде есть данные с неверным системным временем (нет RTC/NTP).");
        for (const QString& s : warn)
        {
            d.ensure(70);
            d.y += 14;
            d.fill(QRectF(kL, d.y, kR - kL, 56), QColor(0xff, 0xf4, 0xd6));
            d.fill(QRectF(kL, d.y, 8, 56), cOrange);
            d.text(QRectF(kL + 30, d.y, kR - kL - 40, 56), s, 8.5, false, cInk);
            d.y += 56;
        }
        signatures(d, U("Начальник участка"), U("Механик"));
    };
    if (!render(w, info, body))
    {
        if (err) *err = U("не удалось создать PDF");
        return QString();
    }
    appendManifest(info, path, "period");
    return path;
}

} // namespace WJ
