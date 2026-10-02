/*
 * Журнал работы — логотип АМПЛИНК для PDF (контуры из RPI-RES_260929_33_amplink-logo.zip, amplink-logo-light.svg)
 * Версия: 01, 2026-09-29
 * Сгенерировано из SVG: координаты viewBox, толщина линий — как в исходном файле Corel
 */
#include "wjpdf.h"
#include <QPainter>
#include <QPainterPath>

namespace WJ
{

QRectF logoBox() { return QRectF(78.54, 296.32, 562.36, 128.08); }

void drawLogo(QPainter& p, const QRectF& target)
{
    const QRectF b = logoBox();
    const qreal k = qMin(target.width() / b.width(), target.height() / b.height());
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    p.translate(target.right() - b.width() * k, target.top());
    p.scale(k, k);
    p.translate(-b.x(), -b.y());
    QPainterPath path;
    path = QPainterPath();
    path.moveTo(139.6966, 396.7199);
    path.lineTo(139.6966, 352.1587);
    path.cubicTo(139.6966, 337.9911, 128.1054, 326.4000, 113.9382, 326.4000);
    path.lineTo(112.2952, 326.4000);
    path.cubicTo(98.1279, 326.4000, 86.5368, 337.9911, 86.5368, 352.1587);
    path.lineTo(86.5368, 396.7199);
    p.setPen(QPen(QColor("#1b1918"), 10.5599, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);
    path = QPainterPath();
    path.moveTo(87.3767, 372.2399);
    path.lineTo(137.5366, 372.2399);
    p.setPen(QPen(QColor("#1b1918"), 10.5599, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);
    path = QPainterPath();
    path.moveTo(172.0966, 396.7199);
    path.lineTo(172.0966, 323.0398);
    p.setPen(QPen(QColor("#1b1918"), 10.5599, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);
    path = QPainterPath();
    path.moveTo(240.7372, 396.7199);
    path.lineTo(240.7372, 323.0398);
    p.setPen(QPen(QColor("#1b1918"), 10.5599, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);
    path = QPainterPath();
    path.moveTo(173.0567, 325.6800);
    path.lineTo(206.2963, 384.9598);
    path.lineTo(239.8897, 326.0601);
    p.setPen(QPen(QColor("#1b1918"), 10.5599, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);
    path = QPainterPath();
    path.moveTo(273.9769, 396.7199);
    path.lineTo(273.9769, 326.6398);
    path.lineTo(327.2574, 326.6398);
    path.lineTo(327.2574, 396.7199);
    p.setPen(QPen(QColor("#1b1918"), 10.5599, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);
    path = QPainterPath();
    path.moveTo(491.7777, 323.0398);
    path.lineTo(491.7777, 396.7199);
    p.setPen(QPen(QColor("#1b1918"), 10.5599, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);
    path = QPainterPath();
    path.moveTo(544.8168, 323.0398);
    path.lineTo(544.8168, 396.7199);
    p.setPen(QPen(QColor("#1b1918"), 10.5599, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);
    path = QPainterPath();
    path.moveTo(544.8168, 359.8794);
    path.lineTo(493.2869, 359.8794);
    p.setPen(QPen(QColor("#1b1918"), 10.5599, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);
    path = QPainterPath();
    path.moveTo(598.4151, 362.8124);
    path.lineTo(581.7973, 362.8124);
    path.lineTo(581.7973, 395.2799);
    path.lineTo(574.3195, 395.2799);
    path.lineTo(574.3195, 324.4786);
    path.lineTo(581.7973, 324.4786);
    path.lineTo(581.7973, 356.2380);
    path.lineTo(598.4151, 356.2380);
    path.lineTo(623.0297, 324.4786);
    path.lineTo(631.3384, 324.4786);
    path.lineTo(604.3350, 358.8677);
    path.lineTo(632.8966, 395.2799);
    path.lineTo(624.0684, 395.2799);
    path.lineTo(598.4151, 362.8124);
    path.closeSubpath();
    p.setPen(QPen(QColor("#1b1918"), 3.1201, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
    p.setBrush(QColor("#1a1a1a"));
    p.drawPath(path);
    path = QPainterPath();
    path.moveTo(427.2167, 377.5199);
    path.lineTo(427.2167, 340.7973);
    path.cubicTo(427.2167, 320.7344, 411.1244, 304.3198, 391.4568, 304.3198);
    path.lineTo(391.4565, 304.3198);
    path.cubicTo(371.7889, 304.3198, 355.6966, 320.7350, 355.6966, 340.7973);
    path.lineTo(355.6966, 377.5199);
    p.setPen(QPen(QColor("#28cf19"), 10.5599, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);
    path = QPainterPath();
    path.moveTo(392.1765, 343.1998);
    path.lineTo(392.1765, 379.9225);
    path.cubicTo(392.1765, 399.9853, 408.2688, 416.4000, 427.9364, 416.4000);
    path.lineTo(427.9367, 416.4000);
    path.cubicTo(447.6043, 416.4000, 463.6966, 399.9847, 463.6966, 379.9225);
    path.lineTo(463.6966, 343.1998);
    p.setPen(QPen(QColor("#28cf19"), 10.5599, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);
    p.restore();
}

} // namespace WJ
