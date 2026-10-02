/*
 * Журнал работы — элементы экранов (рисуются кодом): кнопки рейки, текстовые кнопки с подтверждением,
 * стрелки, переключатели периода, списки с прокруткой пальцем
 * Версия: 01, 2026-09-29
 * Зависимости: Qt 5.15 widgets; цвета и шрифт Manrope — из модуля Журнала ТО (tojournalwidgets.h)
 * Изменения: первая версия
 */
#ifndef WJWIDGETS_H
#define WJWIDGETS_H

#include <QAbstractButton>
#include <QWidget>
#include <QTimer>
#include <QColor>
#include <functional>

class QPainter;

namespace WJ
{

// Рисование
void drawPanel(QPainter& p, const QRectF& r, const QColor& c, qreal radius = 10);
void drawText(QPainter& p, const QRectF& r, const QString& s, int px, bool bold, const QColor& c,
              int align = Qt::AlignLeft | Qt::AlignVCenter, qreal letterSpacing = 0);
void drawDot(QPainter& p, qreal cx, qreal cy, const QColor& c, qreal d = 7);
void drawTile(QPainter& p, const QRectF& r, const QString& label, const QString& value, const QString& unit,
              const QString& sub, const QColor& valueColor = QColor());
QString elided(const QString& s, int px, bool bold, int width);

// Кнопка рейки справа: часы / календарь / колонка (рисуются), шестерёнка / выход (картинки Журнала ТО)
class RailButton : public QAbstractButton
{
    Q_OBJECT
public:
    enum Kind { Shift, Period, Fuel, Settings, Exit };
    RailButton(Kind k, QWidget* parent);
    void setActive(bool on) { active = on; update(); }
protected:
    void paintEvent(QPaintEvent*) override;
private:
    Kind kind;
    bool active = false;
};

// Текстовая кнопка (панель, жирный текст). flash() — временная надпись; arm() — «ПОДТВЕРДИТЬ» на 3 с
class TextButton : public QAbstractButton
{
    Q_OBJECT
public:
    TextButton(const QString& text, QWidget* parent);
    void setBaseText(const QString& t) { base = t; if (!flashing && !armed) update(); }
    void flash(const QString& t, const QColor& c, int ms = 2000);
    void setState(const QString& t, const QColor& c);          // до следующего flash / resetState
    void resetState();
    bool isArmed() const { return armed; }
    void arm();                                                // первое нажатие — подтверждение
    void disarm();
    void setPixelSize(int px) { pxSize = px; update(); }
    void setBackground(const QColor& c) { bg = c; update(); }
protected:
    void paintEvent(QPaintEvent*) override;
private:
    QString base, shown;
    QColor  shownColor;
    QColor  bg;
    bool    flashing = false, armed = false, fixed = false;
    int     pxSize = 14;
    QTimer  timer;
};

// Стрелка ◀ ▶ (картинки Журнала ТО btn_left/btn_right, 43×43)
class ArrowButton : public QAbstractButton
{
    Q_OBJECT
public:
    ArrowButton(bool right, QWidget* parent);
protected:
    void paintEvent(QPaintEvent*) override;
private:
    bool right;
};

// Переключатель периода (СУТКИ / 7 ДНЕЙ …)
class Chip : public QAbstractButton
{
    Q_OBJECT
public:
    Chip(const QString& text, QWidget* parent);
    void setOn(bool on) { this->on = on; update(); }
protected:
    void paintEvent(QPaintEvent*) override;
private:
    bool on = false;
};

// Список с прокруткой пальцем. Строки рисует функция paintRow(p, row, rect)
class ScrollList : public QWidget
{
    Q_OBJECT
public:
    explicit ScrollList(QWidget* parent);
    void setRows(int count, int rowHeight, std::function<void(QPainter&, int, const QRectF&)> painter);
    void setEmptyText(const QString& t) { emptyText = t; update(); }
    void scrollToTop() { offset = 0; update(); }
protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
private:
    int  maxOffset() const;
    int  count = 0, rowH = 36, offset = 0, pressY = 0, pressOffset = 0;
    bool dragging = false;
    QString emptyText;
    std::function<void(QPainter&, int, const QRectF&)> paintRow;
};

} // namespace WJ

#endif // WJWIDGETS_H
