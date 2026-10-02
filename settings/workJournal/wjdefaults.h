/*
 * Журнал работы — подписи оборудования, цвета режимов, пути по умолчанию
 * Версия: 02, 2026-09-29
 * Изменения от 01: подписи D1–D3 по исходнику, 7 позиций оборудования
 *
 * Оборудование — по логике программы, а не по ДКП (датчики на машине ненадёжны, решение 29.09):
 *   опущено, если модуль в состоянии «опускается» или ниже (CentralBroom/FrontRail/BackMagnet/Blower::state);
 *   D1..D3 — StateValveD1..D3 != 0 (ЧИМ), как в mainwindow.cpp addElement: вращение щётки, в обратную сторону, вентилятор
 */
#ifndef WJDEFAULTS_H
#define WJDEFAULTS_H

#include "wjtypes.h"
#include <QColor>

namespace WJ
{

inline QString equipName(int i)
{
    switch (i)
    {
    case EqBroomDown:  return QString::fromUtf8("Щётка опущена");
    case EqDumpDown:   return QString::fromUtf8("Отвал опущен");
    case EqMagnetDown: return QString::fromUtf8("Магнитная плита опущена");
    case EqBlowerDown: return QString::fromUtf8("Воздуходувка опущена");
    case EqRotD1:      return QString::fromUtf8("Вращение щётки (D1)");
    case EqRotD2:      return QString::fromUtf8("Вращение щётки в обратную сторону (D2)");
    case EqRotD3:      return QString::fromUtf8("Вращение вентилятора (D3)");
    default:          return QString();
    }
}

// Цвета режимов: экран (тёмная тема)
inline QColor modeColor(int mode)
{
    switch (mode)
    {
    case ModeClean:  return QColor(0x2f, 0xcb, 0x59);
    case ModeMove:   return QColor(0x4a, 0x90, 0xd9);
    case ModeIdle:   return QColor(0xf0, 0xa5, 0x00);
    case ModeOff:    return QColor(0x6b, 0x6e, 0x74);
    case ModeNoLink: return QColor(0xd2, 0x28, 0x1f);
    default:         return QColor(0x31, 0x3b, 0x46);
    }
}

// Цвета режимов: PDF (светлый лист)
inline QColor modeColorPrint(int mode)
{
    switch (mode)
    {
    case ModeClean:  return QColor(0x22, 0xa4, 0x47);
    case ModeMove:   return QColor(0x3b, 0x7f, 0xc4);
    case ModeIdle:   return QColor(0xe0, 0x9a, 0x00);
    case ModeOff:    return QColor(0x8a, 0x8d, 0x92);
    case ModeNoLink: return QColor(0xc9, 0x28, 0x1f);
    default:         return QColor(0xdf, 0xe3, 0xe8);
    }
}

const char* const kDefaultDbPath    = "/var/lib/auto_cleaner/work_journal.db";
const char* const kDefaultExportDir = "/var/lib/auto_cleaner/export/work_journal";

} // namespace WJ

#endif // WJDEFAULTS_H
