// cantraceview — листинг одной линии CAN с ползунком (экран «GPIO ПУ»)
// версия: GPIO ПУ 2 (RPI-RES_260928_02), 2026-09-28; Qt 5.15.8 widgets
// изменения: переписан под стиль проекта; кладется в рамку frame_canXList из .ui
#ifndef CANTRACEVIEW_H
#define CANTRACEVIEW_H

#include <QWidget>
#include <QPixmap>
#include <QFont>
#include <QSharedPointer>

#include "cantracebuffer.h"

// рисуем только видимые строки прямо из буфера - миллионы кадров не грузят GUI.
// без паузы вид прокручен к последнему кадру; на паузе стоит на месте,
// а прием в буфер продолжается. прокрутка - ползунком или свайпом по тексту,
// прокрутка без паузы просит паузу сигналом pauseRequested()
class CanTraceView : public QWidget
{
    Q_OBJECT
public:
    explicit CanTraceView(QWidget *parent = nullptr);

    // буфер линии и время открытия экрана (от него считаются секунды в строке)
    void setBuffer(QSharedPointer<CanTraceBuffer> buffer_, qint64 originUs_);
    void setPaused(bool paused_);
    bool isPaused();
    // текст, когда кадров нет («нет кадров», «нет интерфейса can0»)
    void setEmptyText(QString text);
    // вызывать по таймеру экрана - перерисует, если в буфере что-то изменилось
    void refresh();

signals:
    void pauseRequested();

protected:
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

private:
    int visibleLines();
    quint64 topLine(quint64 first, quint64 end);
    void scrollToSliderY(int y);
    void requestPause();

    QSharedPointer<CanTraceBuffer> buffer;
    qint64 originUs;
    QPixmap slider;
    QFont font;
    int lineHeight;
    QString emptyText;

    bool paused;
    quint64 pausedTop;      // первая видимая строка на паузе (сквозной номер кадра)

    // перетаскивание
    int dragMode;           // 0 - нет, 1 - ползунок, 2 - текст
    int pressY;
    quint64 pressTop;
    bool textScrolling;

    quint64 lastFirst;
    quint64 lastEnd;
};

#endif // CANTRACEVIEW_H
