// cantraceview — листинг одной линии CAN с ползунком (экран «GPIO ПУ»)
// версия: GPIO ПУ 2 (RPI-RES_260928_02), 2026-09-28
#include "cantraceview.h"

#include <QPainter>
#include <QMouseEvent>
#include <QFontMetrics>
#include <QVector>

// геометрия внутри рамки frame_canXList (290x425, левый верх рамки = 278,145 на экране)
#define VIEW_TEXT_TOP       8       // отступ текста сверху
#define VIEW_TEXT_WIDTH     258     // текст x 278..536 по макету
#define VIEW_SLIDER_X       268     // ползунок x 546 по макету
#define VIEW_TRACK_TOP      19      // дорожка y 164..552 по макету
#define VIEW_TRACK_BOTTOM   407
#define VIEW_LINE_CHARS     42      // «99999.999 18FEF100 FF FF FF FF FF FF FF FF»

CanTraceView::CanTraceView(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);

    originUs = 0;
    paused = false;
    pausedTop = 0;
    dragMode = 0;
    pressY = 0;
    pressTop = 0;
    textScrolling = false;
    lastFirst = ~0ULL;
    lastEnd = ~0ULL;
    emptyText = "нет кадров";

    slider = QPixmap(":/Images/Images/settings/gpioPu/slider.png");

    // моноширинный шрифт: берем самый крупный, при котором строка влезает в ширину
    font = QFont("DejaVu Sans Mono");
    font.setStyleHint(QFont::Monospace);
    for (int px = 13; px >= 9; px--)
    {
        font.setPixelSize(px);
        if (QFontMetrics(font).horizontalAdvance(QString(VIEW_LINE_CHARS, QChar('0'))) <= VIEW_TEXT_WIDTH)
            break;
    }
    lineHeight = QFontMetrics(font).height() + 2;
}

void CanTraceView::setBuffer(QSharedPointer<CanTraceBuffer> buffer_, qint64 originUs_)
{
    buffer = buffer_;
    originUs = originUs_;
    update();
}

void CanTraceView::setEmptyText(QString text)
{
    if (emptyText != text)
    {
        emptyText = text;
        update();
    }
}

bool CanTraceView::isPaused()
{
    return paused;
}

int CanTraceView::visibleLines()
{
    return qMax(1, (height() - VIEW_TEXT_TOP) / lineHeight);
}

quint64 CanTraceView::topLine(quint64 first, quint64 end)
{// первая видимая строка: без паузы - так, чтобы последний кадр был внизу
    quint64 visible = quint64(visibleLines());
    quint64 maxTop = (end - first > visible) ? end - visible : first;
    if (!paused)
        return maxTop;
    return qBound(first, pausedTop, maxTop);
}

void CanTraceView::setPaused(bool paused_)
{
    if (paused == paused_ || buffer.isNull())
        return;

    if (paused_)
    {// фиксируем то, что сейчас на экране
        quint64 first, end;
        buffer->range(first, end);
        pausedTop = topLine(first, end);
    }
    paused = paused_;
    update();
}

void CanTraceView::refresh()
{
    if (buffer.isNull())
        return;

    quint64 first, end;
    buffer->range(first, end);
    if (first != lastFirst || end != lastEnd)
    {// на паузе при этом сдвигается только ползунок
        lastFirst = first;
        lastEnd = end;
        update();
    }
}

void CanTraceView::paintEvent(QPaintEvent *)
{
    if (buffer.isNull())
        return;

    QPainter p(this);
    quint64 first, end;
    buffer->range(first, end);
    int visible = visibleLines();
    quint64 top = topLine(first, end);

    // ---- текст ----
    p.setFont(font);
    if (end == first)
    {
        p.setPen(QColor(140, 146, 152));
        p.drawText(QRect(0, VIEW_TEXT_TOP, VIEW_TEXT_WIDTH, lineHeight * 2), Qt::AlignLeft | Qt::AlignTop, emptyText);
    }
    else
    {
        QVector<CanTraceFrame> frames(visible);
        quint64 seq = top;
        int n = buffer->copy(seq, visible, frames.data());
        QFontMetrics fm(font);

        for (int i = 0; i < n; i++)
        {
            const CanTraceFrame &f = frames[i];
            // время от открытия экрана, ID, данные
            QString id = (f.flags & CAN_TRACE_EXT) ? QString::asprintf("%08X", f.id)
                                                   : QString::asprintf("%03X", f.id);
            QString line = QString::asprintf("%9.3f ", double(f.timeUs - originUs) / 1e6) + id.rightJustified(8) + " ";
            if (f.flags & CAN_TRACE_RTR)
                line += QString("RTR %1").arg(f.dlc);
            else
            {
                for (int b = 0; b < f.dlc; b++)
                    line += QString::asprintf(b ? " %02X" : "%02X", f.data[b]);
            }
            // Tx (ушло с этого RPi) - голубым, Rx - белым
            p.setPen((f.flags & CAN_TRACE_TX) ? QColor(143, 180, 255) : QColor(230, 232, 234));
            p.drawText(0, VIEW_TEXT_TOP + i * lineHeight + fm.ascent(), line);
        }
    }

    // ---- ползунок ----
    int yMin = VIEW_TRACK_TOP;
    int yMax = VIEW_TRACK_BOTTOM - slider.height();
    quint64 total = end - first;
    double pos = 1.0;
    if (total > quint64(visible))
        pos = double(top - first) / double(total - quint64(visible));
    p.drawPixmap(VIEW_SLIDER_X, yMin + int(pos * (yMax - yMin) + 0.5), slider);
}

void CanTraceView::requestPause()
{
    if (!paused)
        emit pauseRequested();// экран включит паузу и кнопку паузы
    if (!paused)
        setPaused(true);
}

void CanTraceView::scrollToSliderY(int y)
{
    quint64 first, end;
    buffer->range(first, end);
    quint64 visible = quint64(visibleLines());
    if (end - first <= visible)
        return;

    int yMin = VIEW_TRACK_TOP;
    int yMax = VIEW_TRACK_BOTTOM - slider.height();
    double pos = qBound(0.0, double(y - slider.height() / 2 - yMin) / double(qMax(1, yMax - yMin)), 1.0);
    pausedTop = first + quint64(pos * double(end - first - visible) + 0.5);
    update();
}

void CanTraceView::mousePressEvent(QMouseEvent *event)
{
    if (buffer.isNull())
        return;

    pressY = event->pos().y();
    textScrolling = false;
    if (event->pos().x() >= VIEW_SLIDER_X - 12)
    {// зона ползунка (с запасом под палец)
        dragMode = 1;
        requestPause();
        scrollToSliderY(event->pos().y());
    }
    else
    {// текст - может быть свайп
        dragMode = 2;
        quint64 first, end;
        buffer->range(first, end);
        pressTop = topLine(first, end);
    }
}

void CanTraceView::mouseMoveEvent(QMouseEvent *event)
{
    if (dragMode == 1)
        scrollToSliderY(event->pos().y());
    else if (dragMode == 2)
    {
        int dy = event->pos().y() - pressY;
        if (!textScrolling && qAbs(dy) < 10)
            return;// это тап, а не свайп
        if (!textScrolling)
        {
            textScrolling = true;
            requestPause();
        }
        quint64 first, end;
        buffer->range(first, end);
        qint64 t = qint64(pressTop) - qint64(dy / lineHeight);
        pausedTop = quint64(qMax<qint64>(qint64(first), t));
        update();
    }
}

void CanTraceView::mouseReleaseEvent(QMouseEvent *)
{
    dragMode = 0;
}
