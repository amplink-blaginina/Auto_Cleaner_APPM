/*
 * Журнал ТО — точки связи с программой Auto_Cleaner
 * Версия: 03, 2026-09-29
 * Изменения от 02: askPassword() — пароль диагностики программы (Password_Form) для страниц «Процедуры ТО» и «Проведение ТО».
 * Изменения от 01: связано с исходником на пульте — моточасы из MainWindow::TOCurValues["Engine"],
 *   экранная клавиатура модуля (tojournalkeyboard.*), tick() для суточных точек и знака ТО на главном экране.
 */
#ifndef TOJOURNALHOOKS_H
#define TOJOURNALHOOKS_H

#include <QString>
#include <functional>

class MainWindow;
class QLineEdit;
class QObject;

namespace ToJ
{
namespace Hooks
{
    // Текущие моточасы (дробные). < 0 — значение недоступно
    double engineHours(MainWindow* mw);

    // Тип и номер машины для шапки: «ЭКСКАВАТОР CAT 336   БОР-001»
    QString machineTitle(MainWindow* mw);

    // Файл БД журнала
    QString databasePath(MainWindow* mw);

    // Открыть экранную клавиатуру для поля ввода (вызывается по нажатию на поле)
    void editText(QLineEdit* edit, MainWindow* mw);

    // Вызывать раз в секунду из MainWindow::oneSecond(). Раз в 60 с пишет суточную точку моточасов
    // и пересчитывает просрочку. Возвращает true, если есть периодическая работа со сроком <= текущих м/ч;
    // milestone — самый ранний такой срок (м/ч)
    bool tick(MainWindow* mw, int* milestone = nullptr);

    // Запросить пароль диагностики (Global/passwordDiag или одноразовый secretPasswordDiag) формой Password_Form программы.
    // onOk вызывается только при верном пароле и только пока жив ctx. Отмена/неверный пароль — ничего не происходит
    void askPassword(MainWindow* mw, QObject* ctx, std::function<void()> onOk);
}
}

#endif // TOJOURNALHOOKS_H
