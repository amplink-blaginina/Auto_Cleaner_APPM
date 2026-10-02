/*
 * Журнал работы — страницы экрана: Смена, Период (он же Топливо), Настройки журнала
 * Версия: 01, 2026-09-29
 * Изменения: первая версия
 */
#ifndef WJPAGES_H
#define WJPAGES_H

#include <QWidget>
#include <QPointer>
#include "wjreport.h"

class ServiceWorkJournalForm;
class QLineEdit;

namespace WJ
{
class ArrowButton;
class TextButton;
class Chip;
class ScrollList;

// Кнопка «PDF»: формирование в фоновом потоке, состояние — надписью на кнопке
class PdfRunner : public QObject
{
public:
    PdfRunner(ServiceWorkJournalForm* form, TextButton* button);
    void runShift(const ShiftReport& r);
    void runPeriod(const PeriodReport& r);
    bool busy() const { return running; }
private:
    void start(std::function<QString(QString*)> job);
    QPointer<ServiceWorkJournalForm> form;
    TextButton* btn;
    bool running = false;
};

class ShiftPage : public QWidget
{
    Q_OBJECT
public:
    explicit ShiftPage(ServiceWorkJournalForm* form);
    void refresh();                      // перечитать данные текущей смены
    void goCurrent();                    // перейти к идущей смене
protected:
    void paintEvent(QPaintEvent*) override;
private:
    ServiceWorkJournalForm* form;
    ShiftSlot    slot;
    ShiftReport  rep;
    ArrowButton* prev;
    ArrowButton* next;
    TextButton*  pdf;
    ScrollList*  events;
    PdfRunner*   runner;
};

class PeriodPage : public QWidget
{
    Q_OBJECT
public:
    PeriodPage(ServiceWorkJournalForm* form, bool fuelMode);
    void refresh();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void select(PeriodKind k);
    void layoutSteppers();
    ServiceWorkJournalForm* form;
    bool         fuel;
    PeriodReport rep;
    Chip*        chips[4];
    ArrowButton* aPrev;
    ArrowButton* aNext;
    ArrowButton* bPrev;          // «ПО» для произвольного периода
    ArrowButton* bNext;
    TextButton*  pdf;
    ScrollList*  list;           // таблица (период) / заправки и сливы (топливо)
    ScrollList*  list2;          // баланс (топливо)
    PdfRunner*   runner;
    int          toMilestone = 0;
    bool         toValid = false;
};

class SettingsPage : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsPage(ServiceWorkJournalForm* form);
    void load();                          // рабочая копия из БД
protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent* e) override;
    bool eventFilter(QObject* obj, QEvent* ev) override;
private:
    struct Param
    {
        QString title, range;
        double* value = nullptr;
        int*    ivalue = nullptr;
        double  min = 0, max = 0, step = 1;
        bool    time = false;             // значение — минуты от 00:00, показывать ЧЧ:ММ
        bool    readOnly = false;
        QString unit;
        QString* text = nullptr;          // текстовое поле
        int     maxLen = 0;
        std::function<QString()> show;    // вычисляемое значение
    };
    void buildGroup(int g);
    void step(int row, int dir);
    void save();
    void defaults();
    QString valueText(const Param& p) const;

    ServiceWorkJournalForm* form;
    int          group = 0;
    Settings     st, stSaved;
    Schedule     sc, scSaved;
    int          shiftCount = 2;
    QVector<Param> params;
    QVector<QWidget*> rowWidgets;
    TextButton*  btnDefaults;
    TextButton*  btnSave;
    QString      note;
};

} // namespace WJ

#endif // WJPAGES_H
