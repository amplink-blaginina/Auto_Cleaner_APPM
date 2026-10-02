/*
 * Журнал ТО — элементы экранов
 * Версия: 02, 2026-09-29
 * Изменения от 01: шрифт макета Manrope (Regular/Bold, OFL 1.1) встроен в модуль и применяется только к этому экрану
 */
#include "tojournalwidgets.h"
#include "tojournalcalc.h"

#include <QPainter>
#include <QPainterPath>
#include <QFontMetrics>
#include <QLineEdit>
#include <QPushButton>
#include <QMouseEvent>
#include <QTextLayout>
#include <QtMath>
#include <QFontDatabase>
#include <QDebug>

namespace ToJ
{

static const char* kImg = ":/Images/settings/toJournal/";

QColor categoryColor(int category)
{
    switch (category)
    {
    case CatReplace: return Color::orange;
    case CatService: return Color::green;
    case CatInspect: return Color::blue;
    default:         return Color::purple;
    }
}

QString categoryTitle(int category)
{
    switch (category)
    {
    case CatReplace: return QStringLiteral("ЗАМЕНА");
    case CatService: return QStringLiteral("ОБСЛУЖИВАНИЕ");
    case CatInspect: return QStringLiteral("ОСМОТР");
    default:         return QStringLiteral("РЕМОНТ");
    }
}

static QString gFamily;

QString loadFonts()
{
    static bool done = false;
    if (done)
        return gFamily;
    done = true;
    const char* files[] = {":/Fonts/toJournal/Manrope-Regular.ttf", ":/Fonts/toJournal/Manrope-Bold.ttf"};
    for (const char* f : files)
    {
        int id = QFontDatabase::addApplicationFont(QString::fromLatin1(f));
        if (id < 0)
        {
            qDebug() << "TO JOURNAL: шрифт не загружен" << f;
            continue;
        }
        if (gFamily.isEmpty())
            gFamily = QFontDatabase::applicationFontFamilies(id).value(0);
    }
    qDebug() << "TO JOURNAL: шрифт" << (gFamily.isEmpty() ? QStringLiteral("программы") : gFamily);
    return gFamily;
}

QFont pxFont(int px, bool bold)
{
    QFont f;
    if (!gFamily.isEmpty())
        f.setFamily(gFamily);
    f.setPixelSize(px);
    f.setBold(bold);
    return f;
}

// Точка категории 7×7 с центром (cx, cy)
static void drawDot(QPainter& p, int cx, int cy, const QColor& c)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(c);
    p.drawEllipse(QPointF(cx + 0.5, cy + 0.5), 3.5, 3.5);
    p.restore();
}

// Высота текста с переносом по словам
static int wrappedHeight(const QFont& f, const QString& s, int w, int lineStep)
{
    QFontMetrics fm(f);
    QRect r = fm.boundingRect(QRect(0, 0, qMax(10, w), 10000), Qt::TextWordWrap, s);
    int lines = qMax(1, int(qRound(double(r.height()) / fm.lineSpacing())));
    return lines * lineStep;
}

// Текст с переносом, строки через lineStep
static void drawWrapped(QPainter& p, const QRect& r, const QString& s, int lineStep)
{
    QTextLayout layout(s, p.font());
    QTextOption opt;
    opt.setWrapMode(QTextOption::WordWrap);
    layout.setTextOption(opt);
    layout.beginLayout();
    int y = 0;
    for (;;)
    {
        QTextLine line = layout.createLine();
        if (!line.isValid())
            break;
        line.setLineWidth(r.width());
        line.setPosition(QPointF(0, y));
        y += lineStep;
    }
    layout.endLayout();
    // выравнивание: базовая линия шрифта по центру шага строки
    QFontMetrics fm(p.font());
    int shift = (lineStep - fm.height()) / 2;
    layout.draw(&p, QPointF(r.x(), r.y() + shift));
}

//------------------------------------------------------------------ DotLabel
DotLabel::DotLabel(QWidget* parent) : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    QSizePolicy sp(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sp.setHeightForWidth(true);
    setSizePolicy(sp);
}

void DotLabel::setItem(const QString& text, const QColor& dot)
{
    txt = text;
    dotColor = dot;
    updateGeometry();
    update();
}

int DotLabel::heightForWidth(int w) const
{
    if (elide)
        return pixelSize + 5;
    return wrappedHeight(pxFont(pixelSize), txt, w - 18, pixelSize + 5);
}

QSize DotLabel::sizeHint() const
{
    QFontMetrics fm(pxFont(pixelSize));
    return QSize(18 + fm.horizontalAdvance(txt), pixelSize + 5);
}

void DotLabel::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    int step = pixelSize + 5;
    drawDot(p, 3, step / 2, dotColor);
    p.setFont(pxFont(pixelSize));
    p.setPen(textColor);
    QString s = elide ? QFontMetrics(p.font()).elidedText(txt, Qt::ElideRight, width() - 18) : txt;
    drawWrapped(p, QRect(18, 0, width() - 18, height()), s, step);
}

//------------------------------------------------------------------ CheckRow
CheckRow::CheckRow(const QString& text, int category, int workId, QWidget* parent)
    : QAbstractButton(parent), wid(workId), cat(category)
{
    setText(text);
    setCheckable(true);
    setFocusPolicy(Qt::NoFocus);
    QSizePolicy sp(QSizePolicy::Preferred, QSizePolicy::Fixed);
    sp.setHeightForWidth(true);
    setSizePolicy(sp);
}

int CheckRow::heightForWidth(int w) const
{
    // флажок 32×32; текст до двух и более строк — строка выше
    int textH = wrappedHeight(pxFont(14), text(), w - (dotX + 18) - 6, 16);
    return qMax(32, textH);
}

QSize CheckRow::sizeHint() const
{
    return QSize(260, heightForWidth(260));
}

void CheckRow::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    QPixmap box(QString(kImg) + (isChecked() ? "checkbox_on.png" : "checkbox.png"));
    int boxY = 0;
    p.drawPixmap(0, boxY, 32, 32, box);
    int textH = height();
    int lines = textH / 16;
    int textTop = (lines <= 1) ? 8 : (32 - lines * 16) / 2;
    if (textTop < 0)
        textTop = 0;
    drawDot(p, dotX + 3, (lines <= 1) ? 16 : textTop + 8, categoryColor(cat));
    p.setFont(pxFont(14));
    p.setPen(Color::text);
    drawWrapped(p, QRect(dotX + 18, textTop, width() - dotX - 18 - 6, textH), text(), 16);
}

//------------------------------------------------------------------ HistoryRow
// Колонки строки (от левого края таблицы, макет: таблица x=19)
static const int kColDate  = 17;
static const int kColHours = 113;
static const int kColSep   = 157;
static const int kColDot   = 181;
static const int kColWork  = 198;
static const int kColExec  = 542;
static const int kTop      = 17;
static const int kStep     = 21;   // шаг пунктов работ
static const int kLine     = 17;   // шаг строк внутри пункта

HistoryRow::HistoryRow(const Record& r, QWidget* parent) : QWidget(parent), rec(r)
{
    QSizePolicy sp(QSizePolicy::Preferred, QSizePolicy::Fixed);
    sp.setHeightForWidth(true);
    setSizePolicy(sp);
}

int HistoryRow::heightForWidth(int w) const
{
    return layoutHeight(w, false, nullptr);
}

QSize HistoryRow::sizeHint() const
{
    return QSize(686, heightForWidth(686));
}

int HistoryRow::layoutHeight(int w, bool paint, QPainter* p) const
{
    QFont f = pxFont(14);
    int workW = qMin(kColExec - kColWork - 12, w - kColWork - 8);
    int y = kTop;
    for (const RecordItem& it : rec.items)
    {
        int h = wrappedHeight(f, it.name, workW, kLine);
        if (paint)
        {
            drawDot(*p, kColDot + 3, y + kLine / 2, categoryColor(it.periodic ? it.category : CatRepair));
            p->setFont(f);
            p->setPen(Color::text);
            drawWrapped(*p, QRect(kColWork, y, workW, h), it.name, kLine);
        }
        y += h + (kStep - kLine);
    }
    int execH = wrappedHeight(f, rec.executor, w - kColExec - 10, kLine);
    int h = qMax(y, kTop + execH + 4);
    h = qMax(h, 50 + 21 + 4);          // дата + метка
    return h + 16 + 2;                 // нижний отступ + разделитель
}

void HistoryRow::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    int h = height();
    // разделители
    p.fillRect(kColSep, 0, 2, h, Color::page);
    p.fillRect(0, h - 2, width(), 2, Color::page);

    QFont f = pxFont(14);
    p.setFont(f);
    p.setPen(Color::text);
    QRect dateRect(kColDate, kTop, 95, kLine);
    drawWrapped(p, dateRect, rec.time.isValid() ? rec.time.toString("dd.MM.yyyy") : QStringLiteral("—"), kLine);
    p.setPen(Color::orange);
    drawWrapped(p, QRect(kColHours, kTop, 44, kLine), QString::number(qFloor(rec.hours)), kLine);

    // метка: ПО ПЛАНУ (рубеж ТО) / РЕМОНТ (только разовые работы)
    bool plan = rec.milestone != kNoValue;
    QColor c = plan ? Color::green : Color::purple;
    QRectF badge(kColDate + 0.5, 50.5, plan ? 68 : 69, 21);
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    QColor border = c;
    border.setAlpha(110);
    p.setPen(QPen(border, 1));
    p.setBrush(Color::badgeBg);
    p.drawRoundedRect(badge, 3, 3);
    p.setPen(c);
    p.setFont(pxFont(10, true));
    p.drawText(badge, Qt::AlignCenter, plan ? QStringLiteral("ПО ПЛАНУ") : QStringLiteral("РЕМОНТ"));
    p.restore();

    layoutHeight(width(), true, &p);

    p.setFont(f);
    p.setPen(Color::text);
    int execH = wrappedHeight(f, rec.executor, width() - kColExec - 10, kLine);
    drawWrapped(p, QRect(kColExec, kTop, width() - kColExec - 10, execH), rec.executor, kLine);
}

//------------------------------------------------------------------ EventCard
EventCard::EventCard(QWidget* parent) : QWidget(parent)
{
}

void EventCard::setEvent(const QString& name, int remaining, int intervalHours)
{
    title = name;
    left = remaining;
    interval = intervalHours;
    empty = false;
    update();
}

void EventCard::clear()
{
    empty = true;
    update();
}

void EventCard::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(Color::panel);
    p.drawRoundedRect(rect(), 10, 10);
    if (empty)
        return;

    // цвет по остатку (как на макете: 7 — красный, 12 — оранжевый, 22 — зелёный), полоска — доля интервала
    double frac = interval > 0 ? qBound(0.0, double(left) / interval, 1.0) : 0.0;
    QColor c = Color::green;
    QColor bar = Color::greenBar;
    if (left <= kLateLimit)
        c = bar = Color::red;
    else if (left <= 2 * kLateLimit)
        c = bar = Color::orange;

    int w = width();
    p.setFont(pxFont(14, true));
    p.setPen(Color::white);
    p.drawText(QRect(12, 7, w - 20, 20), Qt::AlignLeft | Qt::AlignVCenter,
               QFontMetrics(p.font()).elidedText(title, Qt::ElideRight, w - 20));
    p.setFont(pxFont(13));
    p.setPen(Color::gray);
    p.drawText(QRect(12, 30, 100, 18), Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("осталось"));
    p.setPen(c);
    p.drawText(QRect(w - 120, 30, 106, 18), Qt::AlignRight | Qt::AlignVCenter, QString("%1 м/ч").arg(left));

    QRectF track(15, 51, w - 29, 5);
    p.setBrush(Color::barBg);
    p.drawRoundedRect(track, 1.5, 1.5);
    if (frac > 0)
    {
        p.setBrush(bar);
        p.drawRoundedRect(QRectF(track.x(), track.y(), track.width() * frac, track.height()), 1.5, 1.5);
    }
}

//------------------------------------------------------------------ ProcItem
ProcItem::ProcItem(const Procedure& p, QWidget* parent)
    : QAbstractButton(parent), pid(p.id), name(p.name), interval(p.interval), count(p.works.size())
{
    setCheckable(true);
    setFocusPolicy(Qt::NoFocus);
    setFixedHeight(80);
}

void ProcItem::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    // выбранная процедура — фон как у кнопок-«пилюль»
    p.fillRect(QRect(0, 0, width(), 78), isChecked() ? Color::pill : Color::panel);
    p.fillRect(QRect(0, 78, width(), 2), Color::page);
    p.setFont(pxFont(15, true));
    p.setPen(Color::white);
    p.drawText(QRect(32, 18, width() - 40, 22), Qt::AlignLeft | Qt::AlignVCenter,
               QFontMetrics(p.font()).elidedText(name, Qt::ElideRight, width() - 40));
    p.setFont(pxFont(13));
    p.setPen(Color::gray);
    p.drawText(QRect(33, 44, 170, 18), Qt::AlignLeft | Qt::AlignVCenter,
               interval > 0 ? QString("Интервал: %1 м/ч").arg(interval) : QStringLiteral("Разовые работы"));
    p.drawText(QRect(190, 44, 90, 18), Qt::AlignLeft | Qt::AlignVCenter,
               QString("%1 %2").arg(count).arg(plural(count, "работа", "работы", "работ")));
}

//------------------------------------------------------------------ WorkEditRow
WorkEditRow::WorkEditRow(const Work& work, bool oneTime, QWidget* parent)
    : QWidget(parent), w(work), once(oneTime)
{
    setFixedHeight(54);
    nameEdit = new QLineEdit(this);
    nameEdit->setObjectName("lineEdit_workName");
    nameEdit->setText(w.name);
    nameEdit->setPlaceholderText(QStringLiteral("...введите название работы"));
    nameEdit->setFrame(false);
    nameEdit->setFont(pxFont(14));
    nameEdit->setMaxLength(80);
    nameEdit->setStyleSheet("QLineEdit{background:transparent;color:#e8e9ea;border:none;padding:0;}");
    delButton = new QPushButton(this);
    delButton->setObjectName("pushButton_delWork");
    delButton->setFocusPolicy(Qt::NoFocus);
    delButton->setStyleSheet(QString("QPushButton{border-image:url(%1btn_del.png);}"
                                     "QPushButton:pressed{border-image:url(%1btn_del_pressed.png);}").arg(kImg));
    connect(delButton, &QPushButton::clicked, this, [this]() { emit removeRequested(this); });
    connect(nameEdit, &QLineEdit::textChanged, this, [this](const QString& s) { w.name = s; emit changed(); });
    if (once)
        w.category = CatRepair;
}

Work WorkEditRow::work() const
{
    Work x = w;
    x.name = nameEdit->text().trimmed();
    return x;
}

void WorkEditRow::setOneTime(bool oneTime)
{
    if (once == oneTime)
        return;
    once = oneTime;
    // разовые — всегда «ремонт», при возврате к периодической — «замена»
    w.category = once ? CatRepair : (w.category == CatRepair ? CatReplace : w.category);
    update();
}

void WorkEditRow::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    // макет: точка x=360, текст x=378, кнопка × x=858 (от левого края панели x=330)
    drawDot(p, 30 + 3, 27, categoryColor(w.category));
}

void WorkEditRow::resizeEvent(QResizeEvent*)
{
    const int cy = 27;
    nameEdit->setGeometry(48, cy - 12, width() - 48 - 80, 24);
    delButton->setGeometry(width() - 37 - 25, cy - 18, 37, 37);
}

void WorkEditRow::mouseReleaseEvent(QMouseEvent* e)
{
    // нажатие по точке — смена категории (Замена → Обслуживание → Осмотр); у разовых не меняется
    if (!once && e->pos().x() < 46)
    {
        w.category = (w.category + 1) % CatRepair;
        update();
        emit changed();
    }
    QWidget::mouseReleaseEvent(e);
}

} // namespace ToJ
