/*
 * Журнал работы — экран 1024×600 (см. serviceWorkJournalform.h)
 * Версия: 01, 2026-09-29
 * Изменения: первая версия
 *
 * Рейка справа: Смена / Период / Топливо / Настройки журнала (пароль — как у Журнала ТО) / Выход.
 * Всплывающих окон нет (кроме формы пароля программы и экранной клавиатуры Журнала ТО).
 */
#include "serviceWorkJournalform.h"
#include "wjpages.h"
#include "wjwidgets.h"
#include "wjhooks.h"
#include "settings/toJournal/tojournalwidgets.h"

#include <QPainter>
#include <QDebug>

using namespace WJ;
namespace C = ToJ::Color;

ServiceWorkJournalForm::ServiceWorkJournalForm(MainWindow* mainWindow, QWidget* parent) :
    QWidget(parent), _mainWindow(mainWindow)
{
    const QString family = ToJ::loadFonts();
    if (!family.isEmpty())
    {
        QFont f = font();
        f.setFamily(family);
        setFont(f);
    }
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setFixedSize(1024, 600);

    static int seq = 0;
    if (!uiStore.open(Hooks::databasePath(), QString("wj_ui_%1").arg(++seq)))
        qDebug() << "WORK JOURNAL: экран — БД не открыта:" << uiStore.lastError();

    shiftPage = new ShiftPage(this);
    periodPage = new PeriodPage(this, false);
    fuelPage = new PeriodPage(this, true);
    settingsPage = new SettingsPage(this);

    const RailButton::Kind kinds[] = {RailButton::Shift, RailButton::Period, RailButton::Fuel, RailButton::Settings, RailButton::Exit};
    const int ys[] = {92, 172, 252, 332, 505};
    for (int i = 0; i < 5; ++i)
    {
        rail[i] = new RailButton(kinds[i], this);
        rail[i]->move(947, ys[i]);
    }
    connect(rail[0], &QAbstractButton::clicked, this, [this]() { showPage(PageShift); });
    connect(rail[1], &QAbstractButton::clicked, this, [this]() { showPage(PagePeriod); });
    connect(rail[2], &QAbstractButton::clicked, this, [this]() { showPage(PageFuel); });
    connect(rail[3], &QAbstractButton::clicked, this, [this]()
    {
        if (unlocked)
        {
            showPage(PageSettings);
            return;
        }
        Hooks::askPassword(_mainWindow, this, [this]()
        {
            unlocked = true;                 // до закрытия экрана журнала
            showPage(PageSettings);
        });
    });
    connect(rail[4], &QAbstractButton::clicked, this, &QWidget::close);

    // смена «идёт» — обновлять раз в 30 с
    refreshTimer.setInterval(30000);
    connect(&refreshTimer, &QTimer::timeout, this, [this]()
    {
        if (page == PageShift)
            shiftPage->refresh();
        update();
    });
    refreshTimer.start();

    showPage(PageShift);
    qDebug() << "WORK JOURNAL: экран открыт";
}

ServiceWorkJournalForm::~ServiceWorkJournalForm()
{
    qDebug() << "WORK JOURNAL: экран закрыт";
}

QDateTime ServiceWorkJournalForm::now() const
{
    return nowOverride.isValid() ? nowOverride : QDateTime::currentDateTime();
}

QString ServiceWorkJournalForm::machineTitle() const
{
    return Hooks::machineTitle(_mainWindow);
}

void ServiceWorkJournalForm::showPage(int p)
{
    page = p;
    shiftPage->setVisible(p == PageShift);
    periodPage->setVisible(p == PagePeriod);
    fuelPage->setVisible(p == PageFuel);
    settingsPage->setVisible(p == PageSettings);
    for (int i = 0; i < 4; ++i)
        rail[i]->setActive(i == p);
    if (p == PageShift) shiftPage->goCurrent();
    if (p == PagePeriod) periodPage->refresh();
    if (p == PageFuel) fuelPage->refresh();
    if (p == PageSettings) settingsPage->load();
    update();
}

void ServiceWorkJournalForm::refreshAll()
{
    if (page == PagePeriod) periodPage->refresh();
    if (page == PageFuel) fuelPage->refresh();
    if (page == PageShift) shiftPage->refresh();
}

void ServiceWorkJournalForm::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.fillRect(rect(), C::page);
    const QString title = machineTitle();
    drawText(p, QRectF(23, 12, 180, 26), QString::fromUtf8("ЖУРНАЛ РАБОТЫ"), 16, false, C::text);
    if (!title.isEmpty())
    {
        drawText(p, QRectF(178, 12, 20, 26), "|", 16, false, C::text);
        drawText(p, QRectF(202, 12, 400, 26), elided(title, 16, false, 400), 16, false, C::text);
    }
    const double h = Hooks::engineHours(_mainWindow);
    const QString hs = h >= 0 ? QString::number(int(h)) : QString::fromUtf8("—");
    QFont f16 = ToJ::pxFont(16);
    QFontMetrics fm(f16);
    const QString date = now().toString("dd.MM.yyyy");
    qreal x = 924 - fm.horizontalAdvance(date);
    drawText(p, QRectF(x, 12, 120, 26), date, 16, false, C::text);
    x -= 42;
    const QString unit = QString::fromUtf8(" м/ч");
    x -= fm.horizontalAdvance(unit);
    drawText(p, QRectF(x, 12, 60, 26), unit, 16, false, C::text);
    x -= fm.horizontalAdvance(hs);
    drawText(p, QRectF(x, 12, 80, 26), hs, 16, false, C::orange);
    const QString lab = QString::fromUtf8("МОТОЧАСЫ:  ");
    x -= fm.horizontalAdvance(lab);
    drawText(p, QRectF(x, 12, 140, 26), lab, 16, false, C::text);
}
