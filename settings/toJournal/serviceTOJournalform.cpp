/*
 * Журнал ТО — экран
 * Версия: 03, 2026-09-29
 * Изменения от 02: «Процедуры ТО» и «Проведение ТО» — по паролю диагностики (Hooks::askPassword), один раз за открытие экрана
 * Изменения от 01: шрифт макета Manrope (Regular/Bold, OFL 1.1) встроен в модуль и применяется только к этому экрану
 * Платформа: RPi 4B, Raspbian 12 bookworm armhf, Qt 5.15.8 (widgets, sql + QSQLITE, eglfs)
 *
 * Страницы (правая панель):  шестерёнка — Процедуры ТО, «ТО» — Проведение ТО, документ — Журнал, выход.
 * Всплывающих окон нет: подтверждение удаления и проведения ТО — повторным нажатием той же кнопки (3 с),
 * ошибки ввода — текстом на кнопке (2 с) и красной рамкой поля.
 */
#include "serviceTOJournalform.h"
#include "ui_serviceTOJournalform.h"
#include "tojournalwidgets.h"
#include "tojournalhooks.h"

#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScroller>
#include <QScrollBar>
#include <QLineEdit>
#include <QPointer>
#include <QMouseEvent>
#include <QStyle>
#include <QDebug>
#include <QtMath>

using namespace ToJ;

static const char* kImg = ":/Images/settings/toJournal/";

// Удалить всё содержимое компоновки
static void clearLayout(QLayout* layout)
{
    if (layout == nullptr)
        return;
    while (QLayoutItem* item = layout->takeAt(0))
    {
        if (QWidget* w = item->widget())
            w->deleteLater();
        if (QLayout* l = item->layout())
            clearLayout(l);
        delete item;
    }
}

// Вертикальная компоновка для содержимого прокрутки
static QVBoxLayout* contentLayout(QWidget* contents, int l, int t, int r, int b)
{
    QVBoxLayout* v = qobject_cast<QVBoxLayout*>(contents->layout());
    if (v == nullptr)
    {
        v = new QVBoxLayout(contents);
        v->setSpacing(0);
    }
    v->setContentsMargins(l, t, r, b);
    clearLayout(v);
    return v;
}

ServiceTOJournalForm::ServiceTOJournalForm(MainWindow* mainWindow, QWidget* parent) :
    QWidget(parent),
    ui(new Ui::ServiceTOJournalForm),
    _mainWindow(mainWindow)
{
    // шрифт макета — только для этого экрана (до setupUi, чтобы унаследовали все дочерние виджеты)
    QString family = loadFonts();
    if (!family.isEmpty())
    {
        QFont f = font();
        f.setFamily(family);
        setFont(f);
    }
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_StyledBackground);

    // разрядка заголовков, как на макете (в QSS её нет — задаётся шрифтом виджета)
    QWidget* spaced[] = {ui->label_title, ui->label_nextTitle,
                         ui->label_historyTitle, ui->label_selectTitle, ui->label_checkTitle};
    for (QWidget* w : spaced)
    {
        QFont sf = w->font();
        sf.setLetterSpacing(QFont::AbsoluteSpacing, 0.9);
        w->setFont(sf);
    }

    // легенда категорий
    DotLabel* legend[] = {ui->dotLabel_legend0, ui->dotLabel_legend1, ui->dotLabel_legend2, ui->dotLabel_legend3};
    const char* legendText[] = {"Замена", "Обслуживание", "Осмотр", "Ремонт"};
    for (int i = 0; i < CatCount; ++i)
    {
        legend[i]->setPixelSize(13);
        legend[i]->setItem(QString::fromUtf8(legendText[i]), categoryColor(i));
    }

    setupScroll(ui->scrollArea_history);
    setupScroll(ui->scrollArea_extra);
    setupScroll(ui->scrollArea_checklist);
    setupScroll(ui->scrollArea_procs);
    setupScroll(ui->scrollArea_works);
    watchEdit(ui->lineEdit_executor);
    watchEdit(ui->lineEdit_name);
    connect(ui->lineEdit_name, &QLineEdit::textEdited, this, [this]()
    {// название правили вручную — больше не подставляем «ТО - N»
        nameAuto = false;
    });
    connect(ui->lineEdit_executor, &QLineEdit::textChanged, this, [this]()
    {
        ui->lineEdit_executor->setProperty("error", false);
        ui->lineEdit_executor->style()->unpolish(ui->lineEdit_executor);
        ui->lineEdit_executor->style()->polish(ui->lineEdit_executor);
    });

    // моточасы и БД
    double h = Hooks::engineHours(_mainWindow);
    hoursValid = h >= 0;
    if (!store.open(Hooks::databasePath(_mainWindow), h))
        qDebug() << "TO JOURNAL: БД не открыта:" << store.lastError();
    hours = hoursValid ? h : store.lastKnownHours();
    if (hoursValid)
        store.logHours(hours);

    QString machine = Hooks::machineTitle(_mainWindow);
    ui->label_title->setText(machine.isEmpty() ? QStringLiteral("ТО-ЖУРНАЛ")
                                               : QStringLiteral("ТО-ЖУРНАЛ&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;|&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;")
                                                 + machine.toHtmlEscaped().replace("  ", "&nbsp;&nbsp;&nbsp;"));

    mainProgressTimer = new QTimer(this);
    connect(mainProgressTimer, &QTimer::timeout, this, &ServiceTOJournalForm::mainProgress);
    mainProgressTimer->start(1000);

    confirmTimer = new QTimer(this);
    confirmTimer->setSingleShot(true);
    connect(confirmTimer, &QTimer::timeout, this, &ServiceTOJournalForm::disarm);

    reload();
    refreshHeader();
    showPage(PageJournal);
    qDebug() << "TO JOURNAL: экран открыт, моточасы" << hours << (hoursValid ? "" : "(последние сохранённые)");
}

ServiceTOJournalForm::~ServiceTOJournalForm()
{
    delete ui;
}

//------------------------------------------------------------------ общее

Calc ServiceTOJournalForm::calc() const
{
    double start = store.startHours();
    return Calc(procs, recs, start >= 0 ? start : hours);
}

void ServiceTOJournalForm::reload()
{
    procs = store.procedures();
    recs = store.records();
}

QString ServiceTOJournalForm::milestoneName(int m, bool spaced)
{
    return spaced ? QString("ТО - %1").arg(m) : QString("ТО-%1").arg(m);
}

void ServiceTOJournalForm::setupScroll(QScrollArea* area)
{
    // прокрутка пальцем; полоса прокрутки — только индикатор
    QScroller::grabGesture(area->viewport(), QScroller::LeftMouseButtonGesture);
    QScrollerProperties sp = QScroller::scroller(area->viewport())->scrollerProperties();
    sp.setScrollMetric(QScrollerProperties::OvershootDragResistanceFactor, 0.0);
    sp.setScrollMetric(QScrollerProperties::OvershootScrollDistanceFactor, 0.0);
    sp.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
    sp.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
    QScroller::scroller(area->viewport())->setScrollerProperties(sp);
}

void ServiceTOJournalForm::watchEdit(QWidget* edit)
{
    edit->installEventFilter(this);
}

bool ServiceTOJournalForm::eventFilter(QObject* obj, QEvent* e)
{
    // нажатие на поле ввода — экранная клавиатура программы
    if (e->type() == QEvent::MouseButtonRelease)
    {
        if (QLineEdit* edit = qobject_cast<QLineEdit*>(obj))
            Hooks::editText(edit, _mainWindow);
    }
    return QWidget::eventFilter(obj, e);
}

void ServiceTOJournalForm::flash(QPushButton* button, const QString& text, int ms)
{
    // временный текст на кнопке вместо всплывающего сообщения
    if (!button->property("baseText").isValid())
        button->setProperty("baseText", button->text());
    button->setText(text);
    QPointer<QPushButton> b(button);
    int serial = button->property("flashSerial").toInt() + 1;
    button->setProperty("flashSerial", serial);
    QTimer::singleShot(ms, this, [b, serial]()
    {
        if (b && b->property("flashSerial").toInt() == serial)
            b->setText(b->property("baseText").toString());
    });
}

void ServiceTOJournalForm::setArmed(QPushButton* button, bool armed, const QString& text)
{
    if (!button->property("baseText").isValid())
        button->setProperty("baseText", button->text());
    if (!button->property("baseStyle").isValid())
        button->setProperty("baseStyle", button->styleSheet());
    button->setProperty("flashSerial", button->property("flashSerial").toInt() + 1);
    button->setText(armed ? text : button->property("baseText").toString());
    // ожидание подтверждения — зелёный, как активная кнопка правой панели
    QString base = button->property("baseStyle").toString();
    QString corner = button == ui->pushButton_delete ? QStringLiteral(" border:none; border-bottom-left-radius:10px;") : QString();
    button->setStyleSheet(armed ? base + QStringLiteral(" background:#11b72f; color:#ffffff; font-family: \"Manrope\"; font-weight: bold; font-size: 14px;") + corner : base);
}

void ServiceTOJournalForm::disarm()
{
    if (performArmed)
        setArmed(ui->pushButton_perform, false, QString());
    if (deleteArmed)
        setArmed(ui->pushButton_delete, false, QString());
    performArmed = false;
    deleteArmed = false;
}

void ServiceTOJournalForm::showPage(int page)
{
    disarm();
    ui->stackedWidget->setCurrentIndex(page);
    struct { QPushButton* b; const char* img; int page; } side[] =
    {
        {ui->pushButton_settings, "btn_settings", PageProcs},
        {ui->pushButton_to,       "btn_to",       PagePerform},
        {ui->pushButton_journal,  "btn_journal",  PageJournal},
    };
    for (auto& s : side)
    {
        bool on = s.page == page;
        QString img = QString(kImg) + s.img + (on ? "_on" : "");
        s.b->setStyleSheet(on ? QString("QPushButton{border-image:url(%1.png);}").arg(img)
                              : QString("QPushButton{border-image:url(%1.png);} QPushButton:pressed{border-image:url(%1_pressed.png);}").arg(img));
    }
    if (page == PageJournal)
        refreshJournal();
    else if (page == PagePerform)
        refreshPerform();
    else
        refreshProcs(draft.id);
}

void ServiceTOJournalForm::on_pushButton_journal_clicked()
{
    showPage(PageJournal);
}

void ServiceTOJournalForm::on_pushButton_to_clicked()
{
    // проведение ТО — по паролю диагностики (один раз за открытие экрана)
    if (unlocked)
    {
        showPage(PagePerform);
        return;
    }
    Hooks::askPassword(_mainWindow, this, [this]() { unlocked = true; showPage(PagePerform); });
}

void ServiceTOJournalForm::on_pushButton_settings_clicked()
{
    // процедуры ТО — по паролю диагностики (один раз за открытие экрана)
    if (unlocked)
    {
        showPage(PageProcs);
        return;
    }
    Hooks::askPassword(_mainWindow, this, [this]() { unlocked = true; showPage(PageProcs); });
}

void ServiceTOJournalForm::on_pushButton_exit_clicked()
{
    qDebug() << "TO JOURNAL: экран закрыт";
    close();
}

void ServiceTOJournalForm::mainProgress()
{
    if (!isVisible())
        return;
    double h = Hooks::engineHours(_mainWindow);
    if (h >= 0)
    {
        hours = h;
        hoursValid = true;
        store.logHours(h);
    }
    refreshHeader();
    // целые моточасы сменились — пересчитать остатки на странице журнала
    if (qFloor(hours) != shownHours && ui->stackedWidget->currentIndex() == PageJournal)
        refreshJournal();
}

void ServiceTOJournalForm::refreshHeader()
{
    QString value = hoursValid ? QString::number(qFloor(hours)) : QStringLiteral("—");
    ui->label_hours->setText(QString("МОТОЧАСЫ:&nbsp;&nbsp;&nbsp;&nbsp;<span style=\"color:#f0a500;\">%1</span>&nbsp;&nbsp;м/ч").arg(value));
    ui->label_date->setText(QDate::currentDate().toString("dd.MM.yyyy"));
}

//------------------------------------------------------------------ Журнал

void ServiceTOJournalForm::refreshJournal()
{
    shownHours = qFloor(hours);
    const int now = qFloor(hours);
    Calc c = calc();
    double rate = Calc::ratePerDay(store.hoursSamples(40), hours, QDate::currentDate());

    // ближайшее плановое ТО
    QVBoxLayout* cols = nullptr;
    QHBoxLayout* row = qobject_cast<QHBoxLayout*>(ui->widget_nextWorks->layout());
    if (row == nullptr)
    {
        row = new QHBoxLayout(ui->widget_nextWorks);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(13);
    }
    clearLayout(row);
    int planned = c.plannedMilestone(hours);
    if (planned == kNoValue)
    {
        ui->label_nextTitle->setText(QStringLiteral("Ближайшее плановое ТО: —"));
        ui->label_nextLeft->setText(QStringLiteral("Процедуры ТО не заданы"));
        ui->label_nextLeft->setStyleSheet("font-family: \"Manrope\"; font-weight: bold; font-size: 15px; color:#848589;");
    }
    else
    {
        int left = planned - now;
        ui->label_nextTitle->setText(QStringLiteral("Ближайшее плановое ТО: ") + milestoneName(planned, false));
        QString txt;
        if (left >= 0)
        {
            txt = QString("Осталось %1 м/ч").arg(left);
            int days = Calc::daysTo(left, rate);
            if (days != kNoValue)
                txt += QString("   ( %1 %2 )").arg(days).arg(plural(days, "день", "дня", "дней"));
        }
        else
        {
            txt = QString("Просрочено на %1 м/ч").arg(-left);
        }
        ui->label_nextLeft->setText(txt);
        ui->label_nextLeft->setStyleSheet(left >= 0 ? "font-family: \"Manrope\"; font-weight: bold; font-size: 15px; color:#f0a500;" : "font-family: \"Manrope\"; font-weight: bold; font-size: 15px; color:#d2281f;");

        // состав: 2 колонки по 3 пункта; не поместилось — последний пункт «и ещё N работ»
        QVector<DueWork> list = c.checklist(planned);
        QVector<QPair<QString, QColor>> items;
        int shown = list.size() > 6 ? 5 : list.size();
        for (int i = 0; i < shown; ++i)
            items.append(qMakePair(list[i].work.name, categoryColor(list[i].work.category)));
        if (list.size() > 6)
        {
            int more = list.size() - 5;
            items.append(qMakePair(QString("и ещё %1 %2").arg(more).arg(plural(more, "работа", "работы", "работ")), Color::gray));
        }
        for (int i = 0; i < items.size(); ++i)
        {
            if (i % 3 == 0)
            {
                if (cols != nullptr)
                    cols->addStretch(1);
                cols = new QVBoxLayout();
                cols->setContentsMargins(0, 0, 0, 0);
                cols->setSpacing(5);
                row->addLayout(cols, 1);
            }
            DotLabel* d = new DotLabel(ui->widget_nextWorks);
            d->setItem(items[i].first, items[i].second);
            cols->addWidget(d);
        }
        if (cols != nullptr)
            cols->addStretch(1);
        // колонка не помещается по высоте — пункты в одну строку с «…»
        const int colW = (ui->widget_nextWorks->width() - row->spacing()) / 2;
        for (int k = 0; k < row->count(); ++k)
        {
            QLayout* col = row->itemAt(k)->layout();
            if (col == nullptr)
                continue;
            int total = 0;
            for (int n = 0; n < col->count(); ++n)
            {
                if (DotLabel* d = qobject_cast<DotLabel*>(col->itemAt(n)->widget()))
                    total += d->heightForWidth(colW) + col->spacing();
            }
            for (int n = 0; total > ui->widget_nextWorks->height() && n < col->count(); ++n)
            {
                if (DotLabel* d = qobject_cast<DotLabel*>(col->itemAt(n)->widget()))
                    d->setElide(true);
            }
        }
        if (items.size() <= 3)
            row->addStretch(1);      // одна колонка — вторая половина пустая
    }

    // история
    QVBoxLayout* v = contentLayout(ui->scrollArea_historyContents, 0, 0, 0, 0);
    for (const Record& r : recs)
        v->addWidget(new HistoryRow(r, ui->scrollArea_historyContents));
    v->addStretch(1);

    // предстоящие события — 4 ближайшие периодические работы (просроченные с минусом)
    QVector<DueWork> due = c.dueWorks();
    EventCard* cards[] = {ui->eventCard_1, ui->eventCard_2, ui->eventCard_3, ui->eventCard_4};
    for (int i = 0; i < 4; ++i)
    {
        if (i < due.size())
            cards[i]->setEvent(due[i].work.name, due[i].due - now, due[i].interval);
        else
            cards[i]->clear();
    }

    // счётчики
    int inTime = 0, missed = 0;
    c.countStats(hours, inTime, missed);
    ui->label_statInTime->setText(QString::number(inTime));
    ui->label_statMissed->setText(QString::number(missed));
    QString r = rate < 0 ? QStringLiteral("—") : (rate < 10 ? QString::number(rate, 'f', 1).replace('.', ',') : QString::number(qRound(rate)));
    ui->label_statRate->setText(rate < 0 ? r : r + QStringLiteral(" <span style=\"font-size:11px;\">м/ч</span>"));
}

//------------------------------------------------------------------ Проведение ТО

void ServiceTOJournalForm::refreshPerform()
{
    Calc c = calc();
    QVector<int> options = c.milestoneOptions(hours);
    int planned = c.plannedMilestone(hours);
    ui->comboBox_to->blockSignals(true);
    ui->comboBox_to->clear();
    for (int m : options)
        ui->comboBox_to->addItem(milestoneName(m, true), m);
    int idx = ui->comboBox_to->findData(planned);
    ui->comboBox_to->setCurrentIndex(idx < 0 ? 0 : idx);
    ui->comboBox_to->setEnabled(!options.isEmpty());
    ui->comboBox_to->blockSignals(false);

    // разовые работы (процедуры с интервалом 0)
    QVBoxLayout* v = contentLayout(ui->scrollArea_extraContents, 26, 5, 16, 5);
    extraRows.clear();
    QVector<Work> once = c.oneTimeWorks();
    for (int i = 0; i < once.size(); ++i)
    {
        CheckRow* r = new CheckRow(once[i].name, CatRepair, once[i].id, ui->scrollArea_extraContents);
        extraRows.append(r);
        v->addWidget(r);
        if (i + 1 < once.size())
            v->addSpacing(18);
    }
    v->addStretch(1);
    fillChecklist();
}

void ServiceTOJournalForm::on_comboBox_to_currentIndexChanged(int)
{
    disarm();
    fillChecklist();
}

void ServiceTOJournalForm::fillChecklist()
{
    // сохранить отметки текущего списка
    for (CheckRow* r : checkRows)
    {
        if (r->isChecked())
            checkedIds.insert(r->workId());
        else
            checkedIds.remove(r->workId());
    }
    checkRows.clear();

    QVBoxLayout* v = contentLayout(ui->scrollArea_checklistContents, 26, 14, 30, 18);
    int m = ui->comboBox_to->currentData().isValid() ? ui->comboBox_to->currentData().toInt() : kNoValue;
    QVector<DueWork> list = calc().checklist(m);
    int cat = -1;
    for (const DueWork& d : list)
    {
        if (d.work.category != cat)
        {
            if (cat != -1)
                v->addSpacing(30);
            cat = d.work.category;
            QLabel* h = new QLabel(categoryTitle(cat), ui->scrollArea_checklistContents);
            h->setStyleSheet("font-family: \"Manrope\"; font-size: 14px; color:#d6d7d9;");
            h->setFixedHeight(18);
            v->addWidget(h);
            v->addSpacing(13);
        }
        else
        {
            v->addSpacing(18);
        }
        CheckRow* r = new CheckRow(d.work.name, d.work.category, d.work.id, ui->scrollArea_checklistContents);
        r->setChecked(checkedIds.contains(d.work.id));
        checkRows.append(r);
        v->addWidget(r);
    }
    v->addStretch(1);
}

void ServiceTOJournalForm::on_pushButton_checkAll_clicked()
{
    // повторное нажатие при всех отмеченных — снять отметки
    bool all = !checkRows.isEmpty();
    for (CheckRow* r : checkRows)
        all = all && r->isChecked();
    for (CheckRow* r : checkRows)
        r->setChecked(!all);
}

void ServiceTOJournalForm::on_pushButton_perform_clicked()
{
    Record rec;
    for (CheckRow* r : checkRows)
    {
        if (!r->isChecked())
            continue;
        RecordItem it;
        it.workId = r->workId();
        it.name = r->text();
        it.category = r->category();
        it.periodic = true;
        rec.items.append(it);
    }
    bool periodic = !rec.items.isEmpty();
    for (CheckRow* r : extraRows)
    {
        if (!r->isChecked())
            continue;
        RecordItem it;
        it.workId = r->workId();
        it.name = r->text();
        it.category = CatRepair;
        it.periodic = false;
        rec.items.append(it);
    }
    rec.executor = ui->lineEdit_executor->text().trimmed();

    // проверки — текстом на кнопке
    if (rec.items.isEmpty())
    {
        disarm();
        flash(ui->pushButton_perform, QStringLiteral("ОТМЕТЬТЕ РАБОТЫ"));
        return;
    }
    if (rec.executor.isEmpty())
    {
        disarm();
        ui->lineEdit_executor->setProperty("error", true);
        ui->lineEdit_executor->style()->unpolish(ui->lineEdit_executor);
        ui->lineEdit_executor->style()->polish(ui->lineEdit_executor);
        flash(ui->pushButton_perform, QStringLiteral("УКАЖИТЕ ИСПОЛНИТЕЛЯ"));
        return;
    }
    if (!hoursValid)
    {
        disarm();
        flash(ui->pushButton_perform, QStringLiteral("НЕТ МОТОЧАСОВ"));
        return;
    }
    if (!performArmed)
    {// первое нажатие — ждать подтверждения
        performArmed = true;
        setArmed(ui->pushButton_perform, true, QStringLiteral("ПОДТВЕРДИТЬ"));
        confirmTimer->start(3000);
        return;
    }
    confirmTimer->stop();
    disarm();

    rec.time = QDateTime::currentDateTime();
    rec.hours = hours;
    rec.milestone = periodic ? ui->comboBox_to->currentData().toInt() : kNoValue;
    if (store.addRecord(rec) == kNoValue)
    {
        flash(ui->pushButton_perform, QStringLiteral("ОШИБКА ЗАПИСИ"));
        return;
    }
    checkedIds.clear();
    checkRows.clear();
    ui->lineEdit_executor->clear();
    reload();
    showPage(PageJournal);
}

//------------------------------------------------------------------ Процедуры ТО

void ServiceTOJournalForm::refreshProcs(int selectId)
{
    QVBoxLayout* v = contentLayout(ui->scrollArea_procsContents, 0, 0, 0, 0);
    procItems.clear();
    const Procedure* sel = nullptr;
    for (const Procedure& p : procs)
    {
        ProcItem* item = new ProcItem(p, ui->scrollArea_procsContents);
        procItems.append(item);
        v->addWidget(item);
        int id = p.id;
        connect(item, &QAbstractButton::clicked, this, [this, id]()
        {
            disarm();
            for (const Procedure& x : procs)
            {
                if (x.id == id)
                    loadDraft(x);
            }
        });
        if (p.id == selectId)
            sel = &p;
    }
    v->addStretch(1);
    bool keepNew = selectId == kNoValue && draft.id == kNoValue && editorLoaded;
    if (sel != nullptr)
        loadDraft(*sel);
    else if (keepNew)
        ;                                       // несохранённая новая процедура остаётся в редакторе
    else if (!procs.isEmpty())
        loadDraft(procs.first());
    else
        on_pushButton_create_clicked();
}

void ServiceTOJournalForm::loadDraft(const Procedure& p)
{
    draft = p;
    editorLoaded = true;
    nameAuto = p.name.isEmpty() || p.name == milestoneName(p.interval, true);
    for (ProcItem* item : procItems)
        item->setChecked(p.id != kNoValue && item->procedureId() == p.id);
    ui->lineEdit_name->setText(p.name);

    QVBoxLayout* v = contentLayout(ui->scrollArea_worksContents, 0, 0, 0, 0);
    workRows.clear();
    for (const Work& w : p.works)
        addWorkRow(w);
    v->addStretch(1);
    refreshInterval();
}

void ServiceTOJournalForm::addWorkRow(const Work& w)
{
    QVBoxLayout* v = qobject_cast<QVBoxLayout*>(ui->scrollArea_worksContents->layout());
    WorkEditRow* row = new WorkEditRow(w, draft.interval == 0, ui->scrollArea_worksContents);
    watchEdit(row->edit());
    connect(row, &WorkEditRow::removeRequested, this, [this](WorkEditRow* r)
    {
        workRows.removeAll(r);
        r->deleteLater();
    });
    workRows.append(row);
    v->insertWidget(workRows.size() - 1, row);    // перед растяжкой
}

void ServiceTOJournalForm::refreshInterval()
{
    ui->label_interval->setText(QString("<span style=\"font-size:22px; font-weight:bold; color:#f0a500;\">%1</span>"
                                        "&nbsp;<span style=\"font-size:13px; color:#e8e9ea;\">м/ч</span>").arg(draft.interval));
    if (nameAuto)
        ui->lineEdit_name->setText(milestoneName(draft.interval, true));
    for (WorkEditRow* r : workRows)
        r->setOneTime(draft.interval == 0);
}

void ServiceTOJournalForm::on_pushButton_create_clicked()
{
    disarm();
    Procedure p;
    p.interval = kIntervalStep;
    p.name = milestoneName(p.interval, true);
    loadDraft(p);
    for (ProcItem* item : procItems)
        item->setChecked(false);
}

void ServiceTOJournalForm::on_pushButton_intervalDown_clicked()
{
    disarm();
    draft.interval = qMax(0, draft.interval - kIntervalStep);
    refreshInterval();
}

void ServiceTOJournalForm::on_pushButton_intervalUp_clicked()
{
    disarm();
    draft.interval = qMin(kMaxInterval, draft.interval + kIntervalStep);
    refreshInterval();
}

void ServiceTOJournalForm::on_pushButton_addWork_clicked()
{
    disarm();
    Work w;
    w.category = draft.interval == 0 ? CatRepair : CatReplace;
    addWorkRow(w);
    WorkEditRow* row = workRows.last();
    // прокрутить к новой строке и открыть ввод
    QTimer::singleShot(0, this, [this, row]()
    {
        if (!workRows.contains(row))
            return;
        ui->scrollArea_works->ensureWidgetVisible(row, 0, 0);
        Hooks::editText(row->edit(), _mainWindow);
    });
}

void ServiceTOJournalForm::collectDraft()
{
    draft.name = ui->lineEdit_name->text().trimmed();
    draft.works.clear();
    for (WorkEditRow* r : workRows)
    {
        Work w = r->work();
        if (!w.name.isEmpty())
            draft.works.append(w);
    }
}

void ServiceTOJournalForm::on_pushButton_save_clicked()
{
    disarm();
    collectDraft();
    if (draft.name.isEmpty())
    {
        flash(ui->pushButton_save, QStringLiteral("ВВЕДИТЕ НАЗВАНИЕ"));
        return;
    }
    int id = store.saveProcedure(draft, hours);
    if (id == kNoValue)
    {
        flash(ui->pushButton_save, QStringLiteral("ОШИБКА ЗАПИСИ"));
        return;
    }
    reload();
    draft.id = id;
    refreshProcs(id);
    flash(ui->pushButton_save, QStringLiteral("СОХРАНЕНО"), 1500);
}

void ServiceTOJournalForm::on_pushButton_delete_clicked()
{
    if (!deleteArmed)
    {// первое нажатие — ждать подтверждения
        disarm();
        deleteArmed = true;
        setArmed(ui->pushButton_delete, true, QStringLiteral("ПОДТВЕРДИТЬ"));
        confirmTimer->start(3000);
        return;
    }
    confirmTimer->stop();
    disarm();
    if (draft.id != kNoValue && !store.deleteProcedure(draft.id))
    {
        flash(ui->pushButton_delete, QStringLiteral("ОШИБКА"));
        return;
    }
    reload();
    draft = Procedure();
    editorLoaded = false;
    refreshProcs(kNoValue);       // выбрать первую
}
