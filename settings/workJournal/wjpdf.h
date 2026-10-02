/*
 * Журнал работы — PDF-отчёты «за смену» и «за период» (QPdfWriter + QPainter, A4, шрифт Manrope встраивается)
 * Версия: 01, 2026-09-29
 * Оформление — по референсам RPI-RES_260929_44 (смена), _36 (период 7 дней), _39 (период «Сутки»), _41 (пустые реквизиты)
 * Изменения: первая версия
 */
#ifndef WJPDF_H
#define WJPDF_H

#include "wjreport.h"
#include <QString>
#include <QDateTime>
#include <QRectF>

class QPainter;

namespace WJ
{

struct PdfInfo
{
    QString   machineTitle;      // «УБОРОЧНАЯ МАШИНА 318D4   ПМ-001» (пусто — «—»)
    QString   exportDir;
    QDateTime created;
    QString   fontFamily;        // Manrope (пусто — шрифт по умолчанию)
};

// Возвращают полный путь к файлу или пусто (err — причина)
QString writeShiftPdf(const ShiftReport& r, const PdfInfo& info, QString* err = nullptr);
QString writePeriodPdf(const PeriodReport& r, const PdfInfo& info, QString* err = nullptr);

QString pdfPlate(const QString& gos);   // «А 000 АА 174» → «A000AA174» для имени файла

void   drawLogo(QPainter& p, const QRectF& target);   // вписать справа-сверху в target
QRectF logoBox();

} // namespace WJ

#endif // WJPDF_H
