/*
 * Журнал ТО — элементы экранов, которые рисуются кодом (строки списков, карточки)
 * Версия: 02, 2026-09-29
 * Изменения от 01: шрифт макета Manrope (Regular/Bold, OFL 1.1) встроен в модуль и применяется только к этому экрану
 * Зависимости: Qt 5.15 (widgets), tojournaltypes.h
 * Размеры и цвета сняты с макетов 1024×600 (референсы Ивана, 28.09).
 */
#ifndef TOJOURNALWIDGETS_H
#define TOJOURNALWIDGETS_H

#include <QWidget>
#include <QAbstractButton>
#include <QColor>
#include <QFont>
#include "tojournaltypes.h"

class QLineEdit;
class QPushButton;

namespace ToJ
{

// Цвета макета
namespace Color
{
    const QColor page      (0x0d, 0x11, 0x17);
    const QColor panel     (0x15, 0x19, 0x1f);
    const QColor pill      (0x1d, 0x21, 0x27);
    const QColor badgeBg   (0x11, 0x14, 0x18);
    const QColor line      (0x34, 0x37, 0x3c);
    const QColor barBg     (0x31, 0x3b, 0x46);
    const QColor text      (0xe8, 0xe9, 0xea);
    const QColor white     (0xff, 0xff, 0xff);
    const QColor gray      (0x84, 0x85, 0x89);
    const QColor orange    (0xf0, 0xa5, 0x00);
    const QColor green     (0x2f, 0xcb, 0x59);
    const QColor greenBar  (0x2e, 0xa8, 0x4f);
    const QColor blue      (0x4a, 0x90, 0xd9);
    const QColor purple    (0xa8, 0x55, 0xf7);
    const QColor red       (0xd2, 0x28, 0x1f);
}

QColor categoryColor(int category);
QString categoryTitle(int category);          // ЗАМЕНА / ОБСЛУЖИВАНИЕ / ОСМОТР / РЕМОНТ
QFont  pxFont(int px, bool bold = false);     // шрифт модуля (Manrope) заданного размера в пикселях

// Шрифт макета Manrope (Regular + Bold, OFL 1.1) из ресурсов модуля. Повторный вызов ничего не делает.
// Возвращает имя семейства; пусто — не загрузился (тогда шрифт программы)
QString loadFonts();

// Точка категории + текст (с переносом)
class DotLabel : public QWidget
{
    Q_OBJECT
public:
    explicit DotLabel(QWidget* parent = nullptr);
    void setItem(const QString& text, const QColor& dot);
    void setTextColor(const QColor& c) { textColor = c; update(); }
    void setPixelSize(int px) { pixelSize = px; updateGeometry(); update(); }
    void setElide(bool on) { elide = on; updateGeometry(); update(); }   // одна строка с «…» вместо переноса
    int  heightForWidth(int w) const override;
    bool hasHeightForWidth() const override { return true; }
    QSize sizeHint() const override;
protected:
    void paintEvent(QPaintEvent*) override;
private:
    QString txt;
    QColor  dotColor;
    QColor  textColor = Color::text;
    int     pixelSize = 14;
    bool    elide = false;
};

// Строка с флажком: [□] • Текст. Нажатие по всей строке
class CheckRow : public QAbstractButton
{
    Q_OBJECT
public:
    CheckRow(const QString& text, int category, int workId, QWidget* parent = nullptr);
    int workId() const { return wid; }
    int category() const { return cat; }
    int heightForWidth(int w) const override;
    bool hasHeightForWidth() const override { return true; }
    QSize sizeHint() const override;
    void setDotX(int x) { dotX = x; update(); }
protected:
    void paintEvent(QPaintEvent*) override;
private:
    int wid;
    int cat;
    int dotX = 47;
};

// Строка истории работ
class HistoryRow : public QWidget
{
    Q_OBJECT
public:
    explicit HistoryRow(const Record& r, QWidget* parent = nullptr);
    int heightForWidth(int w) const override;
    bool hasHeightForWidth() const override { return true; }
    QSize sizeHint() const override;
protected:
    void paintEvent(QPaintEvent*) override;
private:
    Record rec;
    int layoutHeight(int w, bool paint, class QPainter* p) const;
};

// Карточка «Предстоящие события»
class EventCard : public QWidget
{
    Q_OBJECT
public:
    explicit EventCard(QWidget* parent = nullptr);
    void setEvent(const QString& name, int remaining, int interval);
    void clear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    QString title;
    int     left = 0;
    int     interval = 0;
    bool    empty = true;
};

// Пункт списка процедур
class ProcItem : public QAbstractButton
{
    Q_OBJECT
public:
    ProcItem(const Procedure& p, QWidget* parent = nullptr);
    int procedureId() const { return pid; }
    QSize sizeHint() const override { return QSize(296, 80); }
protected:
    void paintEvent(QPaintEvent*) override;
private:
    int     pid;
    QString name;
    int     interval;
    int     count;
};

// Строка работы в редакторе процедуры: • [название] [×]
class WorkEditRow : public QWidget
{
    Q_OBJECT
public:
    WorkEditRow(const Work& w, bool oneTime, QWidget* parent = nullptr);
    Work work() const;
    QLineEdit* edit() const { return nameEdit; }
    void setOneTime(bool oneTime);
signals:
    void removeRequested(WorkEditRow* row);
    void changed();
protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
private:
    Work         w;
    bool         once;
    QLineEdit*   nameEdit;
    QPushButton* delButton;
};

} // namespace ToJ

#endif // TOJOURNALWIDGETS_H
