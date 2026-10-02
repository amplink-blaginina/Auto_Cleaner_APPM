/*
 * Журнал работы — точки связи с программой Auto_Cleaner (MainWindow, CAN, пароль, клавиатура, Журнал ТО)
 * Версия: 01, 2026-09-29
 * Реализация для пульта — wjhooks.cpp (нужен исходник Auto_Cleaner); для проверки на ПК — tests/wjhooks_stub.cpp
 * Изменения: первая версия
 */
#ifndef WJHOOKS_H
#define WJHOOKS_H

#include <QString>
#include <functional>

class MainWindow;
class QLineEdit;
class QObject;

namespace WJ
{
class Collector;

namespace Hooks
{
    // Вызывать раз в секунду из MainWindow::oneSecond(): при первом вызове создаёт сборщик и подключает
    // canj1939 (j1939Deivce, ЭБУ ДВС) и canj1939Main (canDeivce, шасси); далее передаёт состояние пульта
    void tick(MainWindow* mw);

    Collector* collector();                 // nullptr — ещё не запущен
    void    settingsChanged();              // настройки журнала сохранены — перечитать пороги

    QString databasePath();                 // ini WorkJournal/dbPath
    QString exportDir();                    // ini WorkJournal/exportDir
    QString machineTitle(MainWindow* mw);   // как в шапке Журнала ТО (TOJournal/machineTitle)
    double  engineHours(MainWindow* mw);    // моточасы программы; < 0 — нет

    // Ближайшая работа ТО из Журнала ТО: milestone — срок (м/ч), false — нет данных
    bool    nextTo(MainWindow* mw, int* milestone);

    // Пароль — тот же, что у Журнала ТО (пароль диагностики, Password_Form программы)
    void    askPassword(MainWindow* mw, QObject* ctx, std::function<void()> onOk);

    // Экранная клавиатура (модуль Журнала ТО)
    void    editText(QLineEdit* edit);
}
}

#endif // WJHOOKS_H
