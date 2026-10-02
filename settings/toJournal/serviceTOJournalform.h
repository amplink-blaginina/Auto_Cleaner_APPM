/*
 * Журнал ТО — экран (три страницы: Журнал / Проведение ТО / Процедуры ТО)
 * Версия: 03, 2026-09-29
 * Изменения от 02: «Процедуры ТО» и «Проведение ТО» — по паролю диагностики (Hooks::askPassword), один раз за открытие экрана
 * Изменения от 01: шрифт макета Manrope (Regular/Bold, OFL 1.1) встроен в модуль и применяется только к этому экрану
 * Платформа: RPi 4B, Raspbian 12 bookworm armhf, Qt 5.15.8 (widgets, sql + QSQLITE, eglfs)
 * Зависимости: tojournaltypes.h, tojournalcalc.*, tojournalstore.*, tojournalwidgets.*, tojournalhooks.*,
 *              serviceTOJournalform.ui, Images/settings/toJournal (png), Fonts/toJournal (Manrope ttf)
 */
#ifndef SERVICETOJOURNALFORM_H
#define SERVICETOJOURNALFORM_H

#include <QWidget>
#include <QList>
#include <QSet>
#include "tojournaltypes.h"
#include "tojournalstore.h"
#include "tojournalcalc.h"

class MainWindow;
class QTimer;
class QPushButton;
class QScrollArea;

namespace Ui
{
class ServiceTOJournalForm;
}

namespace ToJ
{
class CheckRow;
class ProcItem;
class WorkEditRow;
}

class ServiceTOJournalForm : public QWidget
{
    Q_OBJECT

public:
    explicit ServiceTOJournalForm(MainWindow* mainWindow, QWidget* parent = nullptr);
    ~ServiceTOJournalForm();

protected:
    bool eventFilter(QObject* obj, QEvent* e) override;

private slots:
    // правая панель
    void on_pushButton_journal_clicked();
    void on_pushButton_to_clicked();
    void on_pushButton_settings_clicked();
    void on_pushButton_exit_clicked();
    // проведение ТО
    void on_comboBox_to_currentIndexChanged(int index);
    void on_pushButton_checkAll_clicked();
    void on_pushButton_perform_clicked();
    // процедуры ТО
    void on_pushButton_create_clicked();
    void on_pushButton_intervalDown_clicked();
    void on_pushButton_intervalUp_clicked();
    void on_pushButton_addWork_clicked();
    void on_pushButton_delete_clicked();
    void on_pushButton_save_clicked();

    void mainProgress();          // 1 с: моточасы, дата, суточная точка
    void disarm();                // сброс ожидания подтверждения

private:
    enum Page
    {
        PageJournal = 0,
        PagePerform = 1,
        PageProcs   = 2
    };

    Ui::ServiceTOJournalForm* ui;
    MainWindow*               _mainWindow;
    ToJ::Store                store;
    QVector<ToJ::Procedure>   procs;
    QVector<ToJ::Record>      recs;
    double                    hours = -1;        // текущие моточасы (последние известные)
    bool                      hoursValid = false;
    bool                      unlocked = false;      // пароль диагностики введён — «Процедуры» и «Проведение ТО» открыты до выхода с экрана
    int                       shownHours = -1;
    QTimer*                   mainProgressTimer;
    QTimer*                   confirmTimer;

    // проведение ТО
    QList<ToJ::CheckRow*> checkRows;
    QList<ToJ::CheckRow*> extraRows;
    QSet<int>             checkedIds;            // отметки сохраняются при смене ТО в списке
    bool                  performArmed = false;

    // редактор процедур
    ToJ::Procedure            draft;
    bool                      nameAuto = true;       // название следует за интервалом, пока его не правили
    bool                      editorLoaded = false;  // в редакторе есть процедура
    QList<ToJ::ProcItem*>     procItems;
    QList<ToJ::WorkEditRow*>  workRows;
    bool                      deleteArmed = false;

    ToJ::Calc calc() const;
    void reload();
    void showPage(int page);
    void refreshHeader();
    void refreshJournal();
    void refreshPerform();
    void fillChecklist();
    void refreshProcs(int selectId);
    void loadDraft(const ToJ::Procedure& p);
    void refreshInterval();
    void addWorkRow(const ToJ::Work& w);
    void collectDraft();
    void flash(QPushButton* button, const QString& text, int ms = 2000);
    void setArmed(QPushButton* button, bool armed, const QString& text);
    void setupScroll(QScrollArea* area);
    void watchEdit(QWidget* edit);
    static QString milestoneName(int m, bool spaced);
};

#endif // SERVICETOJOURNALFORM_H
