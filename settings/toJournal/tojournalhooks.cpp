/*
 * Журнал ТО — точки связи с программой Auto_Cleaner (реализация для пульта)
 * Версия: 03, 2026-09-29
 * Изменения от 02: askPassword() — вход на страницы «Процедуры ТО» и «Проведение ТО» по паролю диагностики;
 *   одноразовый секретный пароль после использования меняется, как в ServiceMainRightForm::passwordDiagOk()
 * Изменения от 01: связано с исходником ~/work/Auto_Cleaner_318D4 на пульте
 *  - моточасы: MainWindow::TOCurValues["Engine"] (секунды работы ДВС при > 700 об/мин, тот же счётчик,
 *    что на главном экране); ключ ini TOJournal/engineHoursKey больше не нужен;
 *  - клавиатура: в программе своей текстовой клавиатуры нет (только цифровая Password_Form) —
 *    используется клавиатура модуля ToJ::Keyboard;
 *  - название машины в программе не хранится — остаётся TOJournal/machineTitle в settingsAutoCleaner.ini;
 *  - tick(): суточные точки моточасов без открытого экрана + признак просрочки для знака ТО.
 */
#include "tojournalhooks.h"
#include "tojournalkeyboard.h"
#include "tojournalstore.h"
#include "tojournalcalc.h"

#include <QSettings>

#include <unistd.h>
#include <QCoreApplication>
#include <QDateTime>
#include <QLineEdit>
#include <QDebug>
#include <QtMath>

#include "mainwindow.h"
#include "password_form.h"
#include "settingsreader.h"
#include <QFile>
#include <cstdlib>

namespace ToJ
{
namespace Hooks
{

static QSettings& ini()
{
    static QSettings s(QCoreApplication::applicationDirPath() + "/settingsAutoCleaner.ini", QSettings::IniFormat);
    return s;
}

double engineHours(MainWindow* mw)
{
    if (!mw || !mw->TOCurValues.contains("Engine"))
        return -1;
    return mw->TOCurValues.value("Engine") / 3600.0;
}

QString machineTitle(MainWindow* mw)
{
    Q_UNUSED(mw);
    return ini().value("TOJournal/machineTitle", QString()).toString();
}

QString databasePath(MainWindow* mw)
{
    Q_UNUSED(mw);
    return ini().value("TOJournal/dbPath", "/var/lib/auto_cleaner/to_journal.db").toString();
}

void editText(QLineEdit* edit, MainWindow* mw)
{
    Q_UNUSED(mw);
    Keyboard::open(edit);
}

bool tick(MainWindow* mw, int* milestone)
{
    static Store* store = nullptr;
    static bool   openFailed = false;
    static qint64 lastMs = 0;
    static bool   due = false;
    static int    dueAt = kNoValue;

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (lastMs == 0 || now - lastMs >= 60000 || now < lastMs)
    {
        lastMs = now;
        double h = engineHours(mw);
        if (!store && !openFailed && h >= 0)
        {
            store = new Store;
            if (!store->open(databasePath(mw), h))
            {
                qDebug() << "TO JOURNAL: tick, БД не открыта:" << store->lastError();
                delete store;
                store = nullptr;
                openFailed = true;   // не пытаться каждую минуту; повтор — после перезапуска программы
            }
        }
        if (store && h >= 0)
        {
            store->logHours(h);
            Calc calc(store->procedures(), store->records(), store->startHours());
            QVector<DueWork> works = calc.dueWorks();
            due = !works.isEmpty() && works.first().due <= qFloor(h);
            dueAt = works.isEmpty() ? kNoValue : works.first().due;
        }
    }
    if (milestone)
        *milestone = dueAt;
    return due;
}

void askPassword(MainWindow* mw, QObject* ctx, std::function<void()> onOk)
{
    if (!mw)
        return;
    Password_Form* pw = new Password_Form(mw, true);
    pw->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    pw->setAttribute(Qt::WA_DeleteOnClose, true);
    mw->settings->beginGroup("Global");
    pw->Recieve_pass_name(mw->settings->value("passwordDiag").toInt());
    pw->Recieve_secret_pass_name(mw->settings->value("secretPasswordDiag").toInt());
    mw->settings->endGroup();
    QObject::connect(pw, &Password_Form::Send_correct, ctx, [mw, onOk](int pass)
    {
        // одноразовый секретный пароль — сменить (как ServiceMainRightForm::passwordDiagOk)
        if (pass == mw->getReader()->readSettingsValue("Global/secretPasswordDiag").toString().toInt())
        {
            if (QFile::exists(QCoreApplication::applicationDirPath() + "/settingsAutoCleaner.ini.lock"))
                QFile::remove(QCoreApplication::applicationDirPath() + "/settingsAutoCleaner.ini.lock");
            int random = std::rand() % 9999 + 1;
            mw->settings->setValue("Global/secretPasswordDiag", random);
            mw->settings->sync();
            ::sync();// сбросить файл настроек на диск
            mw->removeBadSettings();
            qDebug() << "TO JOURNAL: вход по секретному паролю, пароль сменён";
        }
        qDebug() << "TO JOURNAL: пароль принят";
        onOk();
    });
    pw->show();
    pw->raise();
}

}
}
