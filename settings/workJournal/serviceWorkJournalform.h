/*
 * Журнал работы — экран 1024×600: Смена / Период / Топливо / Настройки журнала
 * Версия: 01, 2026-09-29
 * Платформа: RPi 4B, Raspbian 12 bookworm armhf, Qt 5.15.8 (widgets, sql, gui/QPdfWriter), eglfs
 * Эскизы: RPI-RES_260929_26 (_27), _38, _43; спецификация RPI-RES_260929_42
 * Изменения: первая версия
 */
#ifndef SERVICEWORKJOURNALFORM_H
#define SERVICEWORKJOURNALFORM_H

#include <QWidget>
#include <QTimer>
#include "wjstore.h"
#include "wjreport.h"

class MainWindow;

namespace WJ
{
class RailButton;
class ShiftPage;
class PeriodPage;
class SettingsPage;
}

class ServiceWorkJournalForm : public QWidget
{
    Q_OBJECT
public:
    explicit ServiceWorkJournalForm(MainWindow* mainWindow, QWidget* parent = nullptr);
    ~ServiceWorkJournalForm() override;

    MainWindow*        mainWindow() const { return _mainWindow; }
    WJ::Store&         store() { return uiStore; }
    WJ::PeriodSel&     period() { return sel; }
    QDateTime          now() const;                 // текущее время (для проверки на ПК можно подменить)
    void               setNowOverride(const QDateTime& t) { nowOverride = t; }
    void               showPage(int page);          // 0 смена, 1 период, 2 топливо, 3 настройки
    void               refreshAll();
    QString            machineTitle() const;

    enum Page { PageShift = 0, PagePeriod = 1, PageFuel = 2, PageSettings = 3 };

protected:
    void paintEvent(QPaintEvent*) override;

private:
    MainWindow*        _mainWindow;
    WJ::Store          uiStore;
    WJ::PeriodSel      sel;
    QDateTime          nowOverride;
    bool               unlocked = false;
    int                page = PageShift;
    WJ::RailButton*    rail[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};
    WJ::ShiftPage*     shiftPage = nullptr;
    WJ::PeriodPage*    periodPage = nullptr;
    WJ::PeriodPage*    fuelPage = nullptr;
    WJ::SettingsPage*  settingsPage = nullptr;
    QTimer             refreshTimer;
};

#endif // SERVICEWORKJOURNALFORM_H
