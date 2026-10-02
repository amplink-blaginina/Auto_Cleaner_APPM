/*
 * Журнал работы — страницы экрана (см. wjpages.h)
 * Версия: 02, 2026-09-29
 * Изменения от 01: оборудование 7 позиций; пробег и расход без системы
 * Геометрия и цвета — по эскизам RPI-RES_260929_43 (смена), _38 (период), _26 (топливо, настройки)
 */
#include "wjpages.h"
#include "wjwidgets.h"
#include "wjdefaults.h"
#include "wjhooks.h"
#include "wjpdf.h"
#include "wjcollector.h"
#include "serviceWorkJournalform.h"
#include "settings/toJournal/tojournalwidgets.h"

#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <QTimer>
#include <QMouseEvent>
#include <QLineEdit>
#include <QThread>
#include <QDebug>

namespace C = ToJ::Color;

namespace WJ
{

static QString U(const char* s) { return QString::fromUtf8(s); }

// ============================ PdfRunner ============================
PdfRunner::PdfRunner(ServiceWorkJournalForm* f, TextButton* b) : QObject(f), form(f), btn(b) {}

void PdfRunner::start(std::function<QString(QString*)> job)
{
    if (running)
        return;
    running = true;
    btn->setState(U("ФОРМИРУЕТСЯ"), C::orange);
    QPointer<ServiceWorkJournalForm> guard = form;
    TextButton* b = btn;
    PdfRunner* self = this;
    QThread* th = QThread::create([job, guard, b, self]()
    {
        QString err;
        const QString path = job(&err);
        QMetaObject::invokeMethod(qApp, [guard, b, self, path, err]()
        {
            if (!guard)
                return;                   // экран закрыт — файл всё равно сохранён
            self->running = false;
            b->resetState();
            if (path.isEmpty())
            {
                qDebug() << "WORK JOURNAL: PDF не создан:" << err;
                b->flash(U("ОШИБКА"), C::red, 3000);
            }
            else
            {
                qDebug() << "WORK JOURNAL: PDF" << path;
                b->flash(U("СОХРАНЁН"), C::green, 2500);
            }
        }, Qt::QueuedConnection);
    });
    th->setPriority(QThread::LowPriority);
    QObject::connect(th, &QThread::finished, th, &QObject::deleteLater);
    th->start(QThread::LowPriority);
}

static PdfInfo pdfInfo(ServiceWorkJournalForm* f)
{
    PdfInfo i;
    i.machineTitle = f->machineTitle();
    i.exportDir = Hooks::exportDir();
    i.created = f->now();
    i.fontFamily = ToJ::loadFonts();
    return i;
}

void PdfRunner::runShift(const ShiftReport& r)
{
    const PdfInfo info = pdfInfo(form);
    start([r, info](QString* err) { return writeShiftPdf(r, info, err); });
}

void PdfRunner::runPeriod(const PeriodReport& r)
{
    const PdfInfo info = pdfInfo(form);
    start([r, info](QString* err) { return writePeriodPdf(r, info, err); });
}

// ============================ общие ============================
static QColor evColor(const EventRec& e)
{
    if (isEndMark(e)) return C::gray;
    if (e.severity == SevAlarm) return C::red;
    if (e.severity == SevWarn) return C::orange;
    if (e.type == EvCleanStart || e.type == EvRefuel) return C::green;
    return C::gray;
}

static void selectorText(QPainter& p, const QRectF& r, const QString& s)
{
    drawText(p, r, s, 15, true, C::text, Qt::AlignCenter);
}

// ============================ Смена ============================
ShiftPage::ShiftPage(ServiceWorkJournalForm* f) : QWidget(f), form(f)
{
    setGeometry(0, 0, 940, 600);
    prev = new ArrowButton(false, this);
    prev->move(20, 52);
    next = new ArrowButton(true, this);
    next->move(497, 52);
    pdf = new TextButton(U("PDF"), this);
    pdf->setGeometry(805, 52, 120, 43);
    events = new ScrollList(this);
    events->setGeometry(690, 226, 235, 356);
    events->setEmptyText(U("Событий нет"));
    runner = new PdfRunner(form, pdf);

    connect(prev, &QAbstractButton::clicked, this, [this]() { ReportBuilder rb(form->store()); slot = rb.schedules().prevShift(slot); refresh(); });
    connect(next, &QAbstractButton::clicked, this, [this]() { ReportBuilder rb(form->store()); slot = rb.schedules().nextShift(slot); refresh(); });
    connect(pdf, &QAbstractButton::clicked, this, [this]() { if (!runner->busy()) runner->runShift(rep); });
}

void ShiftPage::goCurrent()
{
    ReportBuilder rb(form->store());
    slot = rb.schedules().shiftAt(form->now());
    refresh();
}

void ShiftPage::refresh()
{
    ReportBuilder rb(form->store());
    if (!slot.isValid())
        slot = rb.schedules().shiftAt(form->now());
    rep = rb.shift(slot, form->now());
    next->setEnabled(slot.end <= form->now());
    const QVector<EventRec> ev = rep.events;
    events->setRows(ev.size(), 40, [ev](QPainter& p, int i, const QRectF& r)
    {
        const EventRec& e = ev[i];
        QRectF card(r.x(), r.y(), r.width() - 6, 36);
        drawPanel(p, card, C::panel, 8);
        drawText(p, QRectF(card.x() + 10, card.y(), 44, 36), QDateTime::fromSecsSinceEpoch(e.t).toString("HH:mm"), 12, false, C::orange);
        drawDot(p, card.x() + 60, card.center().y(), evColor(e));
        drawText(p, QRectF(card.x() + 70, card.y(), card.width() - 76, 36), elided(eventText(e), 12, false, int(card.width() - 78)), 12, false, C::text);
    });
    events->scrollToTop();
    update();
}

void ShiftPage::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const Totals& t = rep.tot;

    selectorText(p, QRectF(63, 52, 434, 43), QString("%1  ·  %2 %3  ·  %4–%5").arg(slot.day.toString("dd.MM.yyyy"), U("СМЕНА"))
                 .arg(slot.no).arg(slot.begin.toString("HH:mm"), slot.end.toString("HH:mm")));
    // бейдж
    {
        const QString b = rep.open ? U("ИДЁТ · ") + form->now().toString("HH:mm") : U("ЗАКРЫТА");
        const QColor c = rep.open ? C::green : C::gray;
        QFont f = ToJ::pxFont(10, true);
        const int w = QFontMetrics(f).horizontalAdvance(b) + 16;
        p.setPen(QPen(c, 1));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(QRectF(556.5, 63.5, w, 20), 4, 4);
        drawText(p, QRectF(556, 63, w, 21), b, 10, true, c, Qt::AlignCenter);
    }

    // шкала режимов
    drawPanel(p, QRectF(20, 104, 905, 88), C::panel);
    drawText(p, QRectF(40, 110, 200, 18), U("РЕЖИМЫ ПО МИНУТАМ"), 13, false, C::text, Qt::AlignLeft | Qt::AlignVCenter, 0.4);
    qreal lx = 245;
    for (int m = 0; m <= ModeKeyOff; ++m)
    {
        const QString n = modeName(m);
        drawDot(p, lx + 16 + 3, 120, modeColor(m));
        drawText(p, QRectF(lx + 26, 110, 200, 20), n, 12, false, C::text);
        lx += 26 + QFontMetrics(ToJ::pxFont(12)).horizontalAdvance(n) + 16;
    }
    const qreal X0 = 40, W = 865, Y = 138;
    const qint64 b = slot.begin.toSecsSinceEpoch(), e = slot.end.toSecsSinceEpoch();
    const qreal span = qMax<qint64>(60, e - b);
    const qint64 nowS = form->now().toSecsSinceEpoch();
    const qreal nowX = X0 + qBound<qreal>(0, (nowS - b) / span, 1) * W;
    p.fillRect(QRectF(X0, Y, nowX - X0, 22), modeColor(ModeKeyOff));          // прошлое без данных — ключ выключен
    QColor future = modeColor(ModeKeyOff);
    future.setAlphaF(0.35);
    p.fillRect(QRectF(nowX, Y, X0 + W - nowX, 22), future);
    for (const MinuteRec& m : rep.minutes)
    {
        const qreal x = X0 + (m.t - b) / span * W;
        p.fillRect(QRectF(x, Y, W / span * 60 + 0.6, 22), modeColor(m.dominantMode()));
    }
    if (rep.open)
        p.fillRect(QRectF(nowX, Y - 4, 2, 30), Qt::white);
    const int hours = int(span / 3600);
    const int stepH = hours > 12 ? 2 : 1;
    for (int i = 0; i <= hours; i += stepH)
        drawText(p, QRectF(X0 + i * 3600.0 / span * W - 14, Y + 26, 28, 16), slot.begin.addSecs(i * 3600).toString("HH"), 11, false, C::gray, Qt::AlignCenter);

    // режимы
    drawPanel(p, QRectF(20, 202, 420, 222), C::panel);
    drawText(p, QRectF(40, 214, 300, 18), U("ВРЕМЯ ПО РЕЖИМАМ"), 13, false, C::text, Qt::AlignLeft | Qt::AlignVCenter, 0.4);
    const int order[] = {ModeClean, ModeMove, ModeIdle, ModeOff, ModeNoLink};
    qreal y = 238;
    const int eng = t.secEngine();
    for (int m : order)
    {
        drawDot(p, 34, y + 15, modeColor(m));
        drawText(p, QRectF(45, y, 150, 30), modeName(m), 14, false, C::text);
        drawText(p, QRectF(195, y, 50, 30), fmtHM(t.secMode[m]), 14, true, C::text, Qt::AlignRight | Qt::AlignVCenter);
        const bool ofEngine = m == ModeClean || m == ModeMove || m == ModeIdle;
        drawText(p, QRectF(247, y, 38, 30), ofEngine ? fmtPct(t.secMode[m], eng) : U("—"), 14, false, C::gray, Qt::AlignRight | Qt::AlignVCenter);
        drawPanel(p, QRectF(296, y + 12, 134, 6), C::barBg, 3);
        const qreal frac = t.secKey > 0 ? qreal(t.secMode[m]) / t.secKey : 0;
        if (frac > 0)
            drawPanel(p, QRectF(296, y + 12, qMax<qreal>(3, 134 * frac), 6), modeColor(m), 3);
        y += 30;
    }
    p.fillRect(QRectF(20, y + 2, 420, 1), C::line);
    drawText(p, QRectF(30, y + 4, 160, 30), U("Ключ включён"), 14, false, C::text);
    drawText(p, QRectF(195, y + 4, 50, 30), fmtHM(t.secKey), 14, true, C::text, Qt::AlignRight | Qt::AlignVCenter);
    drawText(p, QRectF(255, y + 4, 180, 30), U("% — от работы ДВС (") + fmtHM(eng) + ")", 11, false, C::gray);

    // оборудование
    drawPanel(p, QRectF(20, 434, 420, 148), C::panel);
    drawText(p, QRectF(40, 442, 300, 18), U("ОБОРУДОВАНИЕ"), 13, false, C::text, Qt::AlignLeft | Qt::AlignVCenter, 0.4);
    y = 462;
    auto kv = [&](const QString& a, const QString& v)
    {
        drawText(p, QRectF(30, y, 280, 17), a, 12, false, C::gray);
        drawText(p, QRectF(250, y, 180, 17), v, 12, true, C::text, Qt::AlignRight | Qt::AlignVCenter);
        y += 17;
    };
    kv(U("Щётка / отвал опущены"), fmtHM(t.secEq[EqBroomDown]) + " / " + fmtHM(t.secEq[EqDumpDown]));
    kv(U("Магнит. плита / воздуходувка опущены"), fmtHM(t.secEq[EqMagnetDown]) + " / " + fmtHM(t.secEq[EqBlowerDown]));
    kv(U("Вращение щётки / обратное (D1 / D2)"), fmtHM(t.secEq[EqRotD1]) + " / " + fmtHM(t.secEq[EqRotD2]));
    kv(U("Вращение вентилятора (D3)"), fmtHM(t.secEq[EqRotD3]));
    kv(U("Т гидробака макс. / выше %1 °C").arg(fmtNum(rep.st.hydroOver, 0)),
       (hasValue(t.hydroMax) ? fmtNum(t.hydroMax, 0) + U(" °C") : U("—")) + " / " + QString::number((t.hydroOverSec + 30) / 60) + U(" мин"));
    kv(U("Т ОЖ макс. / давл. масла мин."), (hasValue(t.coolantMax) ? fmtNum(t.coolantMax, 0) + U(" °C") : U("—")) + " / "
       + (hasValue(t.oilPMin) ? fmtNum(t.oilPMin / 100.0) + U(" бар") : U("—")));

    // плитки
    const double engH = t.engineH();
    struct T { QString a, v, u, s; } tiles[] = {
        {U("Пробег"), fmtNum(t.distM / 1000.0), U("км"), U("в уборке ") + fmtNum(t.distCleanM / 1000.0) + U(" · трансп. ") + fmtNum((t.distM - t.distCleanM) / 1000.0)
                                                              + (t.offCount ? U(" · без системы ") + fmtNum(t.offKm) : QString())},
        {U("Скорость уборки, ср."), fmtNum(t.speedCleanAvg), U("км/ч"), U("макс. ") + fmtNum(t.speedMax)},
        {U("Топливо"), fmtNum(t.fuelL), U("л"), U("в уборке ") + fmtNum(t.fuelCleanL) + U(" л")},
        {U("Расход в уборке"), fmtNum(t.fuelPerCleanHour()), U("л/ч"), (hasValue(engH) && engH > 0.05 && hasValue(t.fuelL)) ? fmtNum(t.fuelL / engH) + U(" л/м·ч общий") : QString()},
        {U("Моточасы за смену"), fmtNum(engH), U("м/ч"), fmtNum(t.engineHStart) + U(" → ") + fmtNum(t.engineHEnd)}};
    y = 202;
    for (const T& tt : tiles)
    {
        drawTile(p, QRectF(450, y, 230, 68), tt.a, tt.v, tt.u, tt.s);
        y += 76;
    }
    drawText(p, QRectF(690, 202, 235, 20), U("Лента событий:"), 14, false, C::text);
}

// ============================ Период / Топливо ============================
PeriodPage::PeriodPage(ServiceWorkJournalForm* f, bool fuelMode) : QWidget(f), form(f), fuel(fuelMode)
{
    setGeometry(0, 0, 940, 600);
    const char* names[] = {"СУТКИ", "7 ДНЕЙ", "МЕСЯЦ", "ПРОИЗВОЛЬНЫЙ"};
    int x = 20;
    for (int i = 0; i < 4; ++i)
    {
        chips[i] = new Chip(U(names[i]), this);
        chips[i]->move(x, 56);
        x += chips[i]->width() + 8;
        connect(chips[i], &QAbstractButton::clicked, this, [this, i]() { select(PeriodKind(i)); });
    }
    pdf = new TextButton(U("PDF"), this);
    pdf->setGeometry(805, 52, 120, 43);
    aPrev = new ArrowButton(false, this);
    aNext = new ArrowButton(true, this);
    bPrev = new ArrowButton(false, this);
    bNext = new ArrowButton(true, this);
    runner = new PdfRunner(form, pdf);

    auto shiftSel = [this](int which, int dir)
    {
        PeriodSel& s = form->period();
        const QDate today = ReportBuilder(form->store()).schedules().reportDay(form->now());
        if (s.kind == PerCustom)
        {
            QDate a = s.from, b = s.to;
            if (which == 0) a = a.addDays(dir); else b = b.addDays(dir);
            if (b > today) b = today;
            if (a > b) { if (which == 0) a = b; else b = a; }
            if (a.daysTo(b) + 1 > PeriodSel::kMaxDays) { if (which == 0) b = a.addDays(PeriodSel::kMaxDays - 1); else a = b.addDays(-(PeriodSel::kMaxDays - 1)); }
            s = PeriodSel::custom(a, b);
        }
        else
        {
            PeriodSel n = s.shifted(dir);
            if (n.from > today)
                return;
            s = n;
        }
        form->refreshAll();
    };
    connect(aPrev, &QAbstractButton::clicked, this, [shiftSel]() { shiftSel(0, -1); });
    connect(aNext, &QAbstractButton::clicked, this, [shiftSel]() { shiftSel(0, 1); });
    connect(bPrev, &QAbstractButton::clicked, this, [shiftSel]() { shiftSel(1, -1); });
    connect(bNext, &QAbstractButton::clicked, this, [shiftSel]() { shiftSel(1, 1); });
    for (ArrowButton* a : {aPrev, aNext, bPrev, bNext})
        a->setAutoRepeat(true), a->setAutoRepeatDelay(500), a->setAutoRepeatInterval(120);
    connect(pdf, &QAbstractButton::clicked, this, [this]() { if (!runner->busy()) runner->runPeriod(rep); });

    list = new ScrollList(this);
    list2 = new ScrollList(this);
    if (fuel)
    {
        list->setGeometry(30, 296, 480, 280);
        list2->setGeometry(540, 296, 375, 280);
        list->setEmptyText(U("Заправок и сливов нет"));
    }
    else
    {
        list->setGeometry(30, 190, 620, 346);
        list2->hide();
    }
}

void PeriodPage::select(PeriodKind k)
{
    PeriodSel& s = form->period();
    const QDate today = ReportBuilder(form->store()).schedules().reportDay(form->now());
    const QDate anchor = s.to.isValid() ? s.to : today;
    switch (k)
    {
    case PerDay:    s = PeriodSel::day(anchor); break;
    case PerWeek:   s = PeriodSel::week(anchor); break;
    case PerMonth:  s = PeriodSel::month(anchor); break;
    case PerCustom: s = PeriodSel::custom(s.from.isValid() ? s.from : today.addDays(-6), anchor); break;
    }
    form->refreshAll();
}

void PeriodPage::layoutSteppers()
{
    const bool custom = form->period().kind == PerCustom;
    if (custom)
    {
        aPrev->move(54, 104); aNext->move(261, 104);
        bPrev->move(354, 104); bNext->move(561, 104);
        bPrev->show(); bNext->show();
    }
    else
    {
        const int w = form->period().kind == PerDay ? 300 : 340;
        aPrev->move(20, 104); aNext->move(20 + w - 43, 104);
        bPrev->hide(); bNext->hide();
    }
}

void PeriodPage::refresh()
{
    ReportBuilder rb(form->store());
    PeriodSel& s = form->period();
    if (!s.from.isValid())
        s = PeriodSel::week(rb.schedules().reportDay(form->now()));
    for (int i = 0; i < 4; ++i)
        chips[i]->setOn(int(s.kind) == i);
    layoutSteppers();
    rep = rb.period(s);
    toValid = Hooks::nextTo(form->mainWindow(), &toMilestone);

    if (!fuel)
    {
        const PeriodReport r = rep;
        list->setRows(r.rows.size(), 38, [r](QPainter& p, int i, const QRectF& rc)
        {
            const PeriodRow& row = r.rows[i];
            p.fillRect(QRectF(rc.x(), rc.y(), rc.width() - 8, 1), C::line);
            drawText(p, QRectF(rc.x() + 10, rc.y(), 100, rc.height()), row.label, 13, false, C::text);
            const int maxSec = r.byShift ? 12 * 3600 : 16 * 3600;
            const qreal frac = qMin(1.0, qreal(row.tot.secMode[ModeClean]) / maxSec);
            drawPanel(p, QRectF(rc.x() + 115, rc.center().y() - 4, 120, 8), C::barBg, 4);
            if (frac > 0)
                drawPanel(p, QRectF(rc.x() + 115, rc.center().y() - 4, qMax<qreal>(4, 120 * frac), 8), C::green, 4);
            drawText(p, QRectF(rc.x() + 235, rc.y(), 45, rc.height()), fmtHM(row.tot.secMode[ModeClean]), 13, true, C::text, Qt::AlignRight | Qt::AlignVCenter);
            drawText(p, QRectF(rc.x() + 280, rc.y(), 75, rc.height()), fmtNum(row.tot.distM / 1000.0), 13, false, C::text, Qt::AlignRight | Qt::AlignVCenter);
            drawText(p, QRectF(rc.x() + 355, rc.y(), 80, rc.height()), fmtNum(row.tot.fuelL), 13, false, C::text, Qt::AlignRight | Qt::AlignVCenter);
            drawText(p, QRectF(rc.x() + 435, rc.y(), 85, rc.height()), fmtNum(row.tot.fuelPerCleanHour()), 13, false, C::text, Qt::AlignRight | Qt::AlignVCenter);
            drawText(p, QRectF(rc.x() + 520, rc.y(), 85, rc.height()), QString::number(row.tot.events), 13, false, C::text, Qt::AlignRight | Qt::AlignVCenter);
        });
    }
    else
    {
        const QVector<EventRec> fe = rep.fuelEvents;
        list->setRows(fe.size(), 52, [fe](QPainter& p, int i, const QRectF& rc)
        {
            const EventRec& e = fe[i];
            const bool refuel = e.type == EvRefuel;
            const QColor c = refuel ? C::green : C::red;
            p.fillRect(QRectF(rc.x(), rc.y(), rc.width() - 8, 1), C::line);
            drawText(p, QRectF(rc.x() + 6, rc.y(), 115, rc.height()), QDateTime::fromSecsSinceEpoch(e.t).toString("dd.MM  HH:mm"), 13, false, C::text);
            const QString tag = refuel ? U("ЗАПРАВКА") : U("СЛИВ");
            const int w = QFontMetrics(ToJ::pxFont(10, true)).horizontalAdvance(tag) + 14;
            p.setPen(QPen(c, 1));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(QRectF(rc.x() + 125.5, rc.y() + (e.keyOn ? 15.5 : 8.5), w, 20), 4, 4);
            drawText(p, QRectF(rc.x() + 125, rc.y() + (e.keyOn ? 15 : 8), w, 21), tag, 10, true, c, Qt::AlignCenter);
            if (!e.keyOn)
                drawText(p, QRectF(rc.x() + 125, rc.y() + 30, 120, 16), U("ключ выкл."), 10, false, C::gray);
            drawText(p, QRectF(rc.x() + 250, rc.y(), 110, rc.height()), fmtNum(e.value2, 0) + U(" → ") + fmtNum(e.value3, 0) + " %", 13, false, C::text);
            drawText(p, QRectF(rc.x() + 360, rc.y(), 100, rc.height()), (refuel ? "+" : QString(QChar(0x2212))) + fmtNum(e.value), 13, true, c, Qt::AlignRight | Qt::AlignVCenter);
        });
        const PeriodReport r = rep;
        list2->setRows(r.rows.size(), 36, [r](QPainter& p, int i, const QRectF& rc)
        {
            const PeriodRow& row = r.rows[i];
            const double thr = r.st.residualPct / 100.0 * r.st.tankL;
            const double res = row.tot.residualL(r.st.tankL);
            p.fillRect(QRectF(rc.x(), rc.y(), rc.width() - 8, 1), C::line);
            drawText(p, QRectF(rc.x() + 6, rc.y(), 95, rc.height()), r.byShift ? U("Смена %1").arg(row.shiftNo) : row.day.toString("dd.MM"), 13, false, C::text);
            drawText(p, QRectF(rc.x() + 95, rc.y(), 62, rc.height()), fmtNum(row.tot.fuelL), 13, false, C::text, Qt::AlignRight | Qt::AlignVCenter);
            drawText(p, QRectF(rc.x() + 160, rc.y(), 70, rc.height()), row.tot.refuelCount ? fmtNum(row.tot.refuelL) : U("—"), 13, false, C::text, Qt::AlignRight | Qt::AlignVCenter);
            drawText(p, QRectF(rc.x() + 232, rc.y(), 55, rc.height()), row.tot.drainCount ? fmtNum(row.tot.drainL) : U("—"), 13, false,
                     row.tot.drainCount ? QColor(0xff, 0x5a, 0x4f) : C::text, Qt::AlignRight | Qt::AlignVCenter);
            drawText(p, QRectF(rc.x() + 290, rc.y(), 65, rc.height()), fmtNum(res), 13, false,
                     hasValue(res) && qAbs(res) > thr ? QColor(0xff, 0x5a, 0x4f) : C::text, Qt::AlignRight | Qt::AlignVCenter);
        });
    }
    list->scrollToTop();
    list2->scrollToTop();
    update();
}

void PeriodPage::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const PeriodSel& s = form->period();
    const Totals& t = rep.total;

    // переключатели периода
    if (s.kind == PerCustom)
    {
        drawText(p, QRectF(20, 104, 30, 43), U("С"), 12, false, C::gray);
        drawText(p, QRectF(97, 104, 164, 43), s.from.toString("dd.MM.yyyy"), 16, true, C::orange, Qt::AlignCenter);
        drawText(p, QRectF(318, 104, 36, 43), U("ПО"), 12, false, C::gray);
        drawText(p, QRectF(397, 104, 164, 43), s.to.toString("dd.MM.yyyy"), 16, true, C::orange, Qt::AlignCenter);
        drawText(p, QRectF(620, 104, 180, 43), U("%1 суток · до %2").arg(s.days()).arg(PeriodSel::kMaxDays), 11, false, C::gray);
    }
    else
    {
        const int w = s.kind == PerDay ? 300 : 340;
        drawText(p, QRectF(63, 104, w - 86, 43), s.title(), 16, true, C::orange, Qt::AlignCenter);
        if (s.kind == PerDay)
            drawText(p, QRectF(340, 104, 400, 43), U("отчётные сутки %1 — %2 · строки по сменам").arg(rep.begin.toString("HH:mm"), rep.end.toString("HH:mm")), 11, false, C::gray);
    }

    const int days = qMax(1, rep.daysWithWork);
    if (!fuel)
    {
        // таблица
        drawPanel(p, QRectF(20, 156, 640, 426), C::panel);
        const qreal hy = 162;
        const QString first = rep.byShift ? U("Смена") : U("Сутки");
        struct H { QString t; qreal x, w; bool right; } hs[] = {
            {first, 40, 100, false}, {U("Уборка"), 145, 120, false}, {U("Пробег, км"), 310, 75, true}, {U("Топливо, л"), 385, 80, true},
            {U("л/ч уборки"), 465, 85, true}, {U("События"), 550, 85, true}};
        for (const H& h : hs)
            drawText(p, QRectF(h.x, hy, h.w, 26), h.t, 12, false, C::gray, (h.right ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignVCenter);
        // итог
        QPainterPath path;
        path.addRoundedRect(QRectF(20, 538, 640, 44), 10, 10);
        p.save();
        p.setClipRect(QRectF(20, 538, 640, 44));
        p.fillPath(path, C::pill);
        p.restore();
        p.fillRect(QRectF(20, 538, 640, 10), C::pill);
        drawText(p, QRectF(40, 538, 100, 44), U("Итого"), 13, true, C::text);
        drawText(p, QRectF(255, 538, 55, 44), fmtHM(t.secMode[ModeClean]), 13, true, C::green, Qt::AlignRight | Qt::AlignVCenter);
        drawText(p, QRectF(310, 538, 75, 44), fmtNum(t.distM / 1000.0), 13, true, C::text, Qt::AlignRight | Qt::AlignVCenter);
        drawText(p, QRectF(385, 538, 80, 44), fmtNum(t.fuelL), 13, true, C::text, Qt::AlignRight | Qt::AlignVCenter);
        drawText(p, QRectF(465, 538, 85, 44), fmtNum(t.fuelPerCleanHour()), 13, true, C::text, Qt::AlignRight | Qt::AlignVCenter);
        drawText(p, QRectF(550, 538, 85, 44), QString::number(t.events), 13, true, C::text, Qt::AlignRight | Qt::AlignVCenter);

        // плитки
        struct T { QString a, v, u, s; } tiles[4];
        const double engH = t.engineH();
        tiles[0] = {U("Моточасы"), (hasValue(engH) ? "+" : "") + fmtNum(engH), U("м/ч"), fmtNum(t.engineHStart) + U(" → ") + fmtNum(t.engineHEnd)};
        if (rep.byShift)
        {
            tiles[1] = {U("Работа ДВС"), fmtHM(t.secEngine()), "", U("уборка ") + fmtPct(t.secMode[ModeClean], t.secEngine())};
            tiles[2] = {U("Пробег"), fmtNum(t.distM / 1000.0), U("км"), U("в уборке ") + fmtNum(t.distCleanM / 1000.0)};
            tiles[3] = {U("Топливо"), fmtNum(t.fuelL), U("л"), fmtNum(t.fuelPerCleanHour()) + U(" л/ч в уборке")};
        }
        else
        {
            tiles[1] = {U("Уборка в день, ср."), fmtHM(t.secMode[ModeClean] / days), "", U("дней с работой: %1 из %2").arg(rep.daysWithWork).arg(rep.rows.size())};
            tiles[2] = {U("Пробег в день, ср."), fmtNum(t.distM / 1000.0 / days), U("км"), U("в уборке ") + fmtNum(t.distCleanM / 1000.0 / days)};
            tiles[3] = {U("Топливо"), fmtNum(t.fuelL), U("л"), (hasValue(engH) && engH > 0.05 && hasValue(t.fuelL)) ? fmtNum(t.fuelL / engH) + U(" л/м·ч") : QString()};
        }
        qreal y = 156;
        for (const T& tt : tiles)
        {
            drawTile(p, QRectF(670, y, 255, 72), tt.a, tt.v, tt.u, tt.s);
            y += 80;
        }
        // Ближайшее ТО
        drawPanel(p, QRectF(670, y, 255, 582 - y), C::panel);
        p.fillRect(QRectF(670, y + 4, 4, 582 - y - 8), C::orange);
        const double h = Hooks::engineHours(form->mainWindow());
        if (toValid)
        {
            drawText(p, QRectF(692, y + 12, 230, 22), U("Ближайшее ТО: на %1 м/ч").arg(toMilestone), 14, true, C::text);
            const double left = h >= 0 ? toMilestone - h : noValue();
            drawText(p, QRectF(692, y + 36, 230, 22), hasValue(left) ? (left >= 0 ? U("Осталось %1 м/ч").arg(fmtNum(left)) : U("Просрочено на %1 м/ч").arg(fmtNum(-left))) : QString(),
                     14, true, hasValue(left) && left < 0 ? QColor(0xff, 0x5a, 0x4f) : C::orange);
        }
        else
            drawText(p, QRectF(692, y + 12, 230, 44), U("Ближайшее ТО: нет данных\nЖурнала ТО"), 13, false, C::gray, Qt::AlignLeft | Qt::AlignTop);
    }
    else
    {
        const double thr = rep.st.residualPct / 100.0 * rep.st.tankL;
        const double res = t.residualL(rep.st.tankL);
        const double engH = t.engineH();
        struct T { QString a, v, u, s; QColor c; } tiles[] = {
            {U("Расход по ЭБУ"), fmtNum(t.fuelL), U("л"), t.offCount ? U("+ без системы ") + fmtNum(t.offFuelL) + U(" л") :
                 ((hasValue(engH) && engH > 0.05 && hasValue(t.fuelL)) ? fmtNum(t.fuelL / engH) + U(" л/м·ч") : QString()), QColor()},
            {U("Заправлено"), fmtNum(t.refuelL), U("л"), U("заправок: %1").arg(t.refuelCount), QColor()},
            {U("Слито"), fmtNum(t.drainL), U("л"), U("событий: %1").arg(t.drainCount), t.drainCount ? QColor(0xff, 0x5a, 0x4f) : QColor()},
            {U("Невязка баланса"), fmtNum(res), U("л"), U("порог %1 л (%2 % бака)").arg(fmtNum(thr)).arg(fmtNum(rep.st.residualPct, 0)),
             hasValue(res) && qAbs(res) > thr ? QColor(0xff, 0x5a, 0x4f) : QColor()}};
        qreal x = 20;
        for (const T& tt : tiles)
        {
            drawTile(p, QRectF(x, 156, 220, 70), tt.a, tt.v, tt.u, tt.s, tt.c);
            x += 228.3;
        }
        drawPanel(p, QRectF(20, 236, 500, 346), C::panel);
        drawText(p, QRectF(40, 242, 300, 22), U("ЗАПРАВКИ И СЛИВЫ"), 13, false, C::text, Qt::AlignLeft | Qt::AlignVCenter, 0.4);
        drawPanel(p, QRectF(530, 236, 395, 346), C::panel);
        drawText(p, QRectF(550, 242, 300, 22), rep.byShift ? U("БАЛАНС ПО СМЕНАМ, Л") : U("БАЛАНС ПО СУТКАМ, Л"), 13, false, C::text, Qt::AlignLeft | Qt::AlignVCenter, 0.4);
        struct H { QString t; qreal x, w; bool right; } hs[] = {
            {rep.byShift ? U("Смена") : U("Сутки"), 546, 80, false}, {U("Расход"), 625, 62, true}, {U("Заправл."), 690, 70, true},
            {U("Слив"), 762, 55, true}, {U("Невязка"), 820, 65, true},
            {U("Дата, время"), 36, 110, false}, {U("Событие"), 155, 90, false}, {U("Уровень"), 280, 100, false}, {U("Объём, л"), 390, 100, true}};
        for (const H& h : hs)
            drawText(p, QRectF(h.x, 270, h.w, 22), h.t, 11, false, C::gray, (h.right ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignVCenter);
    }
}

// ============================ Настройки ============================
static const char* kGroups[] = {"МАШИНА", "СМЕНЫ", "РЕЖИМЫ РАБОТЫ", "ПЕРЕГРЕВ", "ТОПЛИВО"};

SettingsPage::SettingsPage(ServiceWorkJournalForm* f) : QWidget(f), form(f)
{
    setGeometry(0, 0, 940, 600);
    btnDefaults = new TextButton(U("ПО УМОЛЧАНИЮ"), this);
    btnDefaults->setGeometry(330, 517, 285, 65);
    btnSave = new TextButton(U("СОХРАНИТЬ"), this);
    btnSave->setGeometry(621, 517, 303, 65);
    connect(btnDefaults, &QAbstractButton::clicked, this, [this]()
    {
        if (!btnDefaults->isArmed()) { btnDefaults->arm(); return; }
        btnDefaults->disarm();
        defaults();
    });
    connect(btnSave, &QAbstractButton::clicked, this, [this]()
    {
        if (!btnSave->isArmed()) { btnSave->arm(); return; }
        btnSave->disarm();
        save();
    });
}

void SettingsPage::load()
{
    st = stSaved = form->store().settings();
    ScheduleSet ss = form->store().schedules();
    const QDate today = ss.reportDay(form->now());
    // график, который будет действовать с завтрашних отчётных суток (последний сохранённый)
    sc = scSaved = ss.forDay(today.addDays(1));
    note.clear();
    buildGroup(group);
}

QString SettingsPage::valueText(const Param& p) const
{
    if (p.show) return p.show();
    if (p.text) return *p.text;
    const double v = p.value ? *p.value : (p.ivalue ? *p.ivalue : 0);
    if (p.time) return QString("%1:%2").arg(int(v) / 60, 2, 10, QChar('0')).arg(int(v) % 60, 2, 10, QChar('0'));
    return fmtNum(v, p.step < 1 ? 1 : 0);
}

void SettingsPage::buildGroup(int g)
{
    group = g;
    for (QWidget* w : rowWidgets)
    {
        w->hide();
        w->deleteLater();
    }
    rowWidgets.clear();
    params.clear();
    shiftCount = sc.count;
    auto P = [this](const QString& t, const QString& r, double* v, double mn, double mx, double stp, const QString& u)
    {
        Param p; p.title = t; p.range = r; p.value = v; p.min = mn; p.max = mx; p.step = stp; p.unit = u; params.append(p);
    };
    switch (g)
    {
    case 0:
    {
        Param a; a.title = U("ГОС. НОМЕР"); a.range = U("до 12 символов"); a.text = &st.gosNumber; a.maxLen = 12; params.append(a);
        Param b; b.title = U("ОРГАНИЗАЦИЯ"); b.range = U("до 60 символов"); b.text = &st.organization; b.maxLen = 60; params.append(b);
        break;
    }
    case 1:
    {
        Param c; c.title = U("ЧИСЛО СМЕН"); c.range = "1–3"; c.ivalue = &sc.count; c.min = 1; c.max = 3; c.step = 1; params.append(c);
        for (int i = 0; i < sc.count; ++i)
        {
            Param s; s.title = U("НАЧАЛО СМЕНЫ %1").arg(i + 1); s.range = U("шаг 30 мин"); s.ivalue = &sc.start[i];
            s.min = 0; s.max = 1410; s.step = 30; s.time = true; params.append(s);
        }
        break;
    }
    case 2:
        P(U("ПОРОГ «ДВС РАБОТАЕТ»"), U("100–800 об/мин, шаг 50"), &st.rpmOn, 100, 800, 50, U("об/мин"));
        P(U("ПОРОГ «ДВИЖЕНИЕ»"), U("1–10 км/ч"), &st.speedMove, 1, 10, 1, U("км/ч"));
        break;
    case 3:
        P(U("ПЕРЕГРЕВ ГИДРОСИСТЕМЫ"), U("50–110 °C, шаг 5"), &st.hydroOver, 50, 110, 5, U("°C"));
        P(U("ПЕРЕГРЕВ ОЖ ДВС"), U("80–120 °C"), &st.coolantOver, 80, 120, 1, U("°C"));
        break;
    case 4:
    {
        P(U("ОБЪЁМ ТОПЛИВНОГО БАКА"), U("50–1000 л, шаг 10"), &st.tankL, 50, 1000, 10, U("л"));
        P(U("ПОРОГ ЗАПРАВКИ"), U("3–50 % бака"), &st.refuelPct, 3, 50, 1, "%");
        P(U("ПОРОГ СЛИВА"), U("2–30 % бака"), &st.drainPct, 2, 30, 1, "%");
        Param w; w.title = U("ОКНО ЗАПРАВКИ / СЛИВА"); w.range = U("1–30 мин"); w.ivalue = &st.fuelWindowMin; w.min = 1; w.max = 30; w.step = 1; w.unit = U("мин"); params.append(w);
        P(U("ПОРОГ НЕВЯЗКИ БАЛАНСА"), U("2–30 % бака"), &st.residualPct, 2, 30, 1, "%");
        break;
    }
    }
    int y = 96;
    for (int i = 0; i < params.size(); ++i, y += 76)
    {
        Param& p = params[i];
        if (p.text)
        {
            QLineEdit* e = new QLineEdit(*p.text, this);
            e->setGeometry(560, y + 2, 350, 40);
            e->setMaxLength(p.maxLen);
            e->setFont(ToJ::pxFont(16, true));
            e->setStyleSheet("QLineEdit{background:#1d2127;color:#f0a500;border:1px solid #34373c;border-radius:8px;padding:0 10px;}");
            QString* dst = p.text;
            connect(e, &QLineEdit::textChanged, this, [dst](const QString& s) { *dst = s.trimmed(); });
            e->installEventFilter(this);          // клавиатура — по нажатию на поле (eventFilter)
            e->setFocusPolicy(Qt::ClickFocus);
            e->setContextMenuPolicy(Qt::NoContextMenu);
            e->show();
            rowWidgets.append(e);
            continue;
        }
        if (p.readOnly)
            continue;
        ArrowButton* l = new ArrowButton(false, this);
        l->move(670, y);
        ArrowButton* r = new ArrowButton(true, this);
        r->move(846, y);
        for (ArrowButton* a : {l, r})
            a->setAutoRepeat(true), a->setAutoRepeatDelay(500), a->setAutoRepeatInterval(120), a->show();
        connect(l, &QAbstractButton::clicked, this, [this, i]() { step(i, -1); });
        connect(r, &QAbstractButton::clicked, this, [this, i]() { step(i, 1); });
        rowWidgets.append(l);
        rowWidgets.append(r);
    }
    update();
}

void SettingsPage::step(int row, int dir)
{
    if (row < 0 || row >= params.size())
        return;
    Param& p = params[row];
    if (p.value)
        *p.value = qBound(p.min, *p.value + dir * p.step, p.max);
    else if (p.ivalue)
    {
        int v = *p.ivalue + dir * int(p.step);
        if (p.time)
            v = (v + 1440) % 1440;         // время по кругу
        else
            v = qBound(int(p.min), v, int(p.max));
        *p.ivalue = v;
    }
    if (group == 1)
    {
        if (sc.count != shiftCount)
        {
            // новое число смен — начала по умолчанию равными частями от смены 1
            for (int i = 1; i < sc.count; ++i)
                sc.start[i] = (sc.start[0] + i * (1440 / sc.count)) % 1440;
            buildGroup(1);
        }
        const QDate from = ReportBuilder(form->store()).schedules().reportDay(form->now()).addDays(1);
        note = U("Новый график действует с %1 %2. Закрытые смены не пересчитываются.")
               .arg(from.toString("dd.MM.yyyy"), QString("%1:%2").arg(sc.start[0] / 60, 2, 10, QChar('0')).arg(sc.start[0] % 60, 2, 10, QChar('0')));
    }
    update();
}

void SettingsPage::defaults()
{
    const Settings d;
    switch (group)
    {
    case 0: st.gosNumber.clear(); st.organization.clear(); break;
    case 1: sc = Schedule(); break;
    case 2: st.rpmOn = d.rpmOn; st.speedMove = d.speedMove; break;
    case 3: st.hydroOver = d.hydroOver; st.coolantOver = d.coolantOver; break;
    case 4: st.tankL = d.tankL; st.refuelPct = d.refuelPct; st.drainPct = d.drainPct; st.fuelWindowMin = d.fuelWindowMin; st.residualPct = d.residualPct; break;
    }
    buildGroup(group);
}

void SettingsPage::save()
{
    Schedule n = sc;
    if (!n.valid())
    {
        btnSave->flash(U("НЕВЕРНЫЙ ГРАФИК"), QColor(0xff, 0x5a, 0x4f));
        return;
    }
    bool ok = form->store().saveSettings(st);
    const bool schedChanged = n.count != scSaved.count || n.start[0] != scSaved.start[0]
                              || (n.count > 1 && n.start[1] != scSaved.start[1]) || (n.count > 2 && n.start[2] != scSaved.start[2]);
    if (ok && schedChanged)
    {
        n.fromDay = ReportBuilder(form->store()).schedules().reportDay(form->now()).addDays(1);
        ok = form->store().saveSchedule(n);
    }
    if (!ok)
    {
        qDebug() << "WORK JOURNAL: настройки не сохранены:" << form->store().lastError();
        btnSave->flash(U("ОШИБКА"), QColor(0xff, 0x5a, 0x4f));
        return;
    }
    qDebug() << "WORK JOURNAL: настройки сохранены" << (schedChanged ? "(новый график смен)" : "");
    Hooks::settingsChanged();
    stSaved = st;
    scSaved = n;
    btnSave->flash(U("СОХРАНЕНО"), C::green);
}

bool SettingsPage::eventFilter(QObject* obj, QEvent* ev)
{
    if (ev->type() == QEvent::MouseButtonRelease)
        if (QLineEdit* e = qobject_cast<QLineEdit*>(obj))
            Hooks::editText(e);
    return QWidget::eventFilter(obj, ev);
}

void SettingsPage::mousePressEvent(QMouseEvent* e)
{
    const QPoint pt = e->pos();
    if (QRect(23, 80, 296, 400).contains(pt))
    {
        const int g = (pt.y() - 80) / 80;
        if (g >= 0 && g < 5 && g != group)
        {
            note.clear();
            buildGroup(g);
        }
    }
}

void SettingsPage::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    drawText(p, QRectF(25, 52, 300, 24), U("НАСТРОЙКИ ЖУРНАЛА"), 13, false, C::text, Qt::AlignLeft | Qt::AlignVCenter, 0.4);
    drawPanel(p, QRectF(23, 80, 296, 502), C::panel);
    auto hm = [](int m) { return QString("%1:%2").arg(m / 60, 2, 10, QChar('0')).arg(m % 60, 2, 10, QChar('0')); };
    QString subs[5];
    subs[0] = st.gosNumber.isEmpty() ? U("гос. номер не задан") : st.gosNumber;
    subs[1] = U("%1 смен%2 · ").arg(sc.count).arg(sc.count == 1 ? U("а") : U("ы"));
    for (int i = 0; i < sc.count; ++i)
        subs[1] += (i ? " / " : "") + hm(sc.start[i]);
    subs[2] = U("ДВС %1 об/мин · %2 км/ч").arg(fmtNum(st.rpmOn, 0), fmtNum(st.speedMove, 0));
    subs[3] = U("гидробак %1 °C · ОЖ %2 °C").arg(fmtNum(st.hydroOver, 0), fmtNum(st.coolantOver, 0));
    subs[4] = U("бак %1 л · пороги").arg(fmtNum(st.tankL, 0));
    for (int g = 0; g < 5; ++g)
    {
        const qreal y = 80 + g * 80;
        if (g == group)
        {
            p.save();
            QPainterPath clip;
            clip.addRoundedRect(QRectF(23, 80, 296, 502), 10, 10);
            p.setClipPath(clip);
            p.fillRect(QRectF(23, y, 296, 80), C::pill);
            p.restore();
        }
        p.fillRect(QRectF(23, y + 78, 296, 2), C::page);
        drawText(p, QRectF(55, y + 18, 250, 22), U(kGroups[g]), 14, true, C::text);
        drawText(p, QRectF(55, y + 42, 250, 20), elided(subs[g], 12, false, 250), 12, false, C::gray);
    }

    drawPanel(p, QRectF(330, 80, 594, 430), C::panel);
    int y = 96;
    for (const Param& prm : params)
    {
        drawText(p, QRectF(356, y + 4, 300, 20), prm.title, 13, false, C::text, Qt::AlignLeft | Qt::AlignVCenter, 0.3);
        drawText(p, QRectF(356, y + 26, prm.text ? 200 : 300, 18), prm.range, 11, false, C::gray);
        if (!prm.text)
        {
            const QString v = valueText(prm);
            QFont fv = ToJ::pxFont(22, true);
            const int vw = QFontMetrics(fv).horizontalAdvance(v);
            const int uw = prm.unit.isEmpty() ? 0 : QFontMetrics(ToJ::pxFont(12)).horizontalAdvance(prm.unit) + 4;
            const qreal x0 = 713 + (133 - vw - uw) / 2.0;
            drawText(p, QRectF(x0, y, vw + 2, 43), v, 22, true, prm.readOnly ? C::gray : C::orange);
            if (uw)
                drawText(p, QRectF(x0 + vw + 4, y + 4, uw, 43), prm.unit, 12, false, C::text);
        }
        y += 76;
    }
    if (group == 0)
        drawText(p, QRectF(356, y + 4, 550, 20), U("Пустое поле печатается в PDF графой для записи от руки."), 12, false, C::gray);
    if (group == 1)
    {
        // длительность смен (вычисляется)
        QString d;
        for (int i = 0; i < sc.count; ++i)
        {
            const int a = sc.offset(i), b = i + 1 < sc.count ? sc.offset(i + 1) : 1440;
            d += (i ? " / " : "") + hm(qMax(0, b - a));
        }
        drawText(p, QRectF(356, y + 4, 300, 20), U("ДЛИТЕЛЬНОСТЬ"), 13, false, C::text, Qt::AlignLeft | Qt::AlignVCenter, 0.3);
        drawText(p, QRectF(356, y + 26, 300, 18), U("считается"), 11, false, C::gray);
        drawText(p, QRectF(600, y, 310, 43), sc.valid() ? d : U("неверный порядок смен"), 18, true,
                 sc.valid() ? C::gray : QColor(0xff, 0x5a, 0x4f), Qt::AlignRight | Qt::AlignVCenter);
        y += 60;
        if (!note.isEmpty())
            drawText(p, QRectF(356, y, 550, 20), note, 12, false, C::orange);
    }
}

} // namespace WJ
