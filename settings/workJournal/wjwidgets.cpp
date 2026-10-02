/*
 * Журнал работы — элементы экранов (см. wjwidgets.h)
 * Версия: 01, 2026-09-29
 * Изменения: первая версия
 */
#include "wjwidgets.h"
#include "settings/toJournal/tojournalwidgets.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QFontMetrics>
#include <QLinearGradient>

using ToJ::pxFont;
namespace C = ToJ::Color;

namespace WJ
{

static const char* kImg = ":/Images/settings/toJournal/";

// ============================ рисование ============================
void drawPanel(QPainter& p, const QRectF& r, const QColor& c, qreal radius)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(c);
    p.drawRoundedRect(r, radius, radius);
    p.restore();
}

void drawText(QPainter& p, const QRectF& r, const QString& s, int px, bool bold, const QColor& c, int align, qreal ls)
{
    QFont f = pxFont(px, bold);
    if (ls != 0)
        f.setLetterSpacing(QFont::AbsoluteSpacing, ls);
    p.setFont(f);
    p.setPen(c);
    p.drawText(r, align, s);
}

void drawDot(QPainter& p, qreal cx, qreal cy, const QColor& c, qreal d)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(c);
    p.drawEllipse(QPointF(cx, cy), d / 2, d / 2);
    p.restore();
}

QString elided(const QString& s, int px, bool bold, int width)
{
    return QFontMetrics(pxFont(px, bold)).elidedText(s, Qt::ElideRight, width);
}

void drawTile(QPainter& p, const QRectF& r, const QString& label, const QString& value, const QString& unit,
              const QString& sub, const QColor& valueColor)
{
    drawPanel(p, r, C::panel);
    const qreal x = r.x() + 14, w = r.width() - 28;
    drawText(p, QRectF(x, r.y() + 8, w, 16), label, 11, false, C::gray);
    QFont fv = pxFont(20, true);
    p.setFont(fv);
    p.setPen(valueColor.isValid() ? valueColor : C::text);
    const qreal vy = r.y() + 24;
    p.drawText(QRectF(x, vy, w, 26), Qt::AlignLeft | Qt::AlignVCenter, value);
    const qreal vw = QFontMetricsF(fv).horizontalAdvance(value);
    if (!unit.isEmpty())
        drawText(p, QRectF(x + vw + 4, vy + 3, w - vw - 4, 26), unit, 12, false, C::gray);
    if (!sub.isEmpty())
        drawText(p, QRectF(x, vy + 26, w, 16), elided(sub, 11, false, int(w)), 11, false, C::gray);
}

// ============================ RailButton ============================
RailButton::RailButton(Kind k, QWidget* parent) : QAbstractButton(parent), kind(k)
{
    setFixedSize(57, 57);
    setFocusPolicy(Qt::NoFocus);
}

void RailButton::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    if (kind == Settings || kind == Exit)
    {
        QString name = kind == Settings ? "btn_settings" : "btn_exit";
        if (kind == Settings && active) name += "_on";
        else if (isDown()) name += "_pressed";
        p.drawPixmap(0, 0, QPixmap(QString::fromLatin1(kImg) + name + ".png"));
        return;
    }
    QRectF r(0, 0, 57, 57);
    if (active)
    {
        QLinearGradient g(0, 0, 0, 57);
        g.setColorAt(0, QColor(0x18, 0xc9, 0x36));
        g.setColorAt(1, QColor(0x10, 0xae, 0x2c));
        p.setBrush(g);
    }
    else
        p.setBrush(isDown() ? QColor(0x3a, 0x44, 0x48) : QColor(0x2a, 0x32, 0x36));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(r, 10, 10);

    QPen pen(Qt::white, 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    p.translate(13.5, 13.5);           // иконка 30×30 по центру
    if (kind == Shift)
    {
        p.drawEllipse(QPointF(15, 15), 11, 11);
        p.drawLine(QPointF(15, 8), QPointF(15, 15));
        p.drawLine(QPointF(15, 15), QPointF(20, 18));
    }
    else if (kind == Period)
    {
        p.drawRoundedRect(QRectF(5, 7, 20, 18), 2, 2);
        p.drawLine(QPointF(5, 12), QPointF(25, 12));
        p.drawLine(QPointF(10, 4), QPointF(10, 9));
        p.drawLine(QPointF(20, 4), QPointF(20, 9));
        for (int row = 0; row < 2; ++row)
            for (int col = 0; col < 3; ++col)
                p.drawLine(QPointF(9 + col * 5, 16 + row * 4), QPointF(11 + col * 5, 16 + row * 4));
    }
    else if (kind == Fuel)
    {
        p.drawRoundedRect(QRectF(6, 5, 12, 21), 1.5, 1.5);
        p.drawLine(QPointF(6, 12), QPointF(18, 12));
        QPainterPath hose;
        hose.moveTo(18, 10);
        hose.lineTo(22, 13);
        hose.lineTo(22, 22);
        hose.cubicTo(22, 25, 26, 25, 26, 22);
        hose.lineTo(26, 11);
        hose.lineTo(23, 8);
        p.drawPath(hose);
    }
}

// ============================ TextButton ============================
TextButton::TextButton(const QString& text, QWidget* parent) : QAbstractButton(parent), base(text), bg(C::panel)
{
    setFocusPolicy(Qt::NoFocus);
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, this, [this]() { flashing = false; armed = false; update(); });
}

void TextButton::flash(const QString& t, const QColor& c, int ms)
{
    armed = false;
    fixed = false;
    flashing = true;
    shown = t;
    shownColor = c;
    timer.start(ms);
    update();
}

void TextButton::setState(const QString& t, const QColor& c)
{
    timer.stop();
    armed = false;
    flashing = false;
    fixed = true;
    shown = t;
    shownColor = c;
    update();
}

void TextButton::resetState()
{
    fixed = false;
    update();
}

void TextButton::arm()
{
    flashing = false;
    fixed = false;
    armed = true;
    timer.start(3000);
    update();
}

void TextButton::disarm()
{
    armed = false;
    timer.stop();
    update();
}

void TextButton::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    QColor b = armed ? QColor(0x1f, 0x9e, 0x40) : bg;
    if (isDown())
        b = b.lighter(130);
    drawPanel(p, rect(), b);
    QString t = base;
    QColor c = C::text;
    if (armed) { t = QString::fromUtf8("ПОДТВЕРДИТЬ"); c = Qt::white; }
    else if (flashing || fixed) { t = shown; c = shownColor; }
    if (!isEnabled())
        c = C::gray;
    drawText(p, rect(), t, pxSize, true, c, Qt::AlignCenter, 0.4);
}

// ============================ ArrowButton ============================
ArrowButton::ArrowButton(bool r, QWidget* parent) : QAbstractButton(parent), right(r)
{
    setFixedSize(43, 43);
    setFocusPolicy(Qt::NoFocus);
}

void ArrowButton::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    QString name = right ? "btn_right" : "btn_left";
    if (isDown())
        name += "_pressed";
    p.drawPixmap(0, 0, QPixmap(QString::fromLatin1(kImg) + name + ".png"));
    if (!isEnabled())
        p.fillRect(rect(), QColor(13, 17, 23, 150));
}

// ============================ Chip ============================
Chip::Chip(const QString& text, QWidget* parent) : QAbstractButton(parent)
{
    setText(text);
    setFocusPolicy(Qt::NoFocus);
    QFont f = pxFont(12, true);
    f.setLetterSpacing(QFont::AbsoluteSpacing, 0.3);
    setFixedSize(QFontMetrics(f).horizontalAdvance(text) + 34, 36);
}

void Chip::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    p.setPen(on ? QPen(C::green, 1) : Qt::NoPen);
    p.setBrush(isDown() ? C::line : C::pill);
    p.drawRoundedRect(r, 18, 18);
    drawText(p, rect(), text(), 12, true, on ? C::white : C::gray, Qt::AlignCenter, 0.3);
}

// ============================ ScrollList ============================
ScrollList::ScrollList(QWidget* parent) : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent, false);
}

void ScrollList::setRows(int c, int h, std::function<void(QPainter&, int, const QRectF&)> painter)
{
    count = c;
    rowH = h;
    paintRow = painter;
    offset = qBound(0, offset, maxOffset());
    update();
}

int ScrollList::maxOffset() const
{
    return qMax(0, count * rowH - height());
}

void ScrollList::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setClipRect(rect());
    if (count == 0)
    {
        if (!emptyText.isEmpty())
            drawText(p, QRectF(0, 0, width(), 40), emptyText, 13, false, C::gray, Qt::AlignCenter);
        return;
    }
    const int first = offset / rowH;
    for (int i = first; i < count; ++i)
    {
        const qreal y = i * rowH - offset;
        if (y > height())
            break;
        if (paintRow)
            paintRow(p, i, QRectF(0, y, width(), rowH));
    }
    // индикатор прокрутки
    if (maxOffset() > 0)
    {
        const qreal total = count * rowH;
        const qreal h = qMax<qreal>(30, height() * height() / total);
        const qreal y = (height() - h) * offset / maxOffset();
        drawPanel(p, QRectF(width() - 4, y, 4, h), C::gray, 2);
    }
}

void ScrollList::mousePressEvent(QMouseEvent* e)
{
    dragging = true;
    pressY = e->pos().y();
    pressOffset = offset;
}

void ScrollList::mouseMoveEvent(QMouseEvent* e)
{
    if (!dragging)
        return;
    offset = qBound(0, pressOffset - (e->pos().y() - pressY), maxOffset());
    update();
}

void ScrollList::mouseReleaseEvent(QMouseEvent*)
{
    dragging = false;
}

} // namespace WJ
