/**
MIT License

Copyright (c) 2018 Michael Scopchanov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "Delegate.h"
#include "Delegate_p.h"
#include <QPainter>

Delegate::Delegate(QObject *parent) :
	QStyledItemDelegate(parent),
	m_ptr(new DelegatePrivate)
{

}

Delegate::~Delegate()
{
	delete m_ptr;
}

QSize Delegate::iconSize() const
{
	return m_ptr->iconSize;
}

void Delegate::setIconSize(int width, int height)
{
	m_ptr->iconSize = QSize(width, height);
}

QMargins Delegate::contentsMargins() const
{
	return m_ptr->margins;
}

void Delegate::setContentsMargins(int left, int top, int right, int bottom)
{
	m_ptr->margins = QMargins(left, top, right, bottom);
}

int Delegate::horizontalSpacing() const
{
	return m_ptr->spacingHorizontal;
}

void Delegate::setHorizontalSpacing(int spacing)
{
	m_ptr->spacingHorizontal = spacing;
}

int Delegate::verticalSpacing() const
{
	return m_ptr->spacingVertical;
}

void Delegate::setVerticalSpacing(int spacing)
{
	m_ptr->spacingVertical = spacing;
}

void Delegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
					 const QModelIndex &index) const
{
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

	const QPalette &palette(opt.palette);
    const QRect &rect(opt.rect);
    const QRect &contentRect(rect.adjusted(m_ptr->margins.left(),
											   m_ptr->margins.top(),
											   -m_ptr->margins.right(),
											   -m_ptr->margins.bottom()));
	const bool lastIndex = (index.model()->rowCount() - 1) == index.row();
	const bool hasIcon = !opt.icon.isNull();
	const int bottomEdge = rect.bottom();
	QFont f(opt.font);

    f.setPointSize(10);
    f.setWeight(QFont::Normal);

	painter->save();
	painter->setClipping(true);
	painter->setClipRect(rect);
	painter->setFont(opt.font);

	// Draw background
    painter->drawPixmap(contentRect.left(), contentRect.top(),
                        QPixmap (":/Images/Images/text.png").scaled(contentRect.width(), contentRect.height()));
//    painter->fillRect(rect, opt.state & QStyle::State_Selected ?
//                          palette.highlight().color() :
//                          palette.light().color());

	// Draw bottom line
//    painter->setPen(lastIndex ? palette.dark().color()
//                              : palette.mid().color());
//    painter->drawLine(lastIndex ? rect.left() : m_ptr->margins.left(),
//                      bottomEdge, rect.right(), bottomEdge);

	// Draw message icon
	if (hasIcon)
		painter->drawPixmap(contentRect.left(), contentRect.top(),
							opt.icon.pixmap(m_ptr->iconSize));

	// Draw timestamp
	QRect timeStampRect(m_ptr->timestampBox(opt, index));

    timeStampRect.moveTo(m_ptr->margins.left() + m_ptr->iconSize.width()
                         , contentRect.top());

	painter->setFont(f);
    painter->setPen(QColor(255,255,255));
	painter->drawText(timeStampRect, Qt::TextSingleLine,
					  index.data(Qt::UserRole).toString());

	// Draw message text
    QRect messageRect(m_ptr->messageBox(opt, index));

    messageRect.moveTo(timeStampRect.right() + m_ptr->spacingHorizontal, timeStampRect.top());

    painter->setFont(opt.font);
    painter->setPen(palette.text().color());
    painter->drawText(messageRect, Qt::TextWordWrap, opt.text);

    painter->restore();
}

QSize Delegate::sizeHint(const QStyleOptionViewItem &option,
						 const QModelIndex &index) const
{
	QStyleOptionViewItem opt(option);
	initStyleOption(&opt, index);

    int textHeight = m_ptr->messageBox(opt, index).height() + m_ptr->spacingVertical;
	int iconHeight = m_ptr->iconSize.height();
	int h = textHeight > iconHeight ? textHeight : iconHeight;

    return QSize(opt.rect.width(), h);
}

DelegatePrivate::DelegatePrivate() :
	iconSize(16, 16),
	margins(0, 0, 0, 0),
	spacingHorizontal(0),
	spacingVertical(0)
{

}

QRect DelegatePrivate::timestampBox(const QStyleOptionViewItem &option,
							 const QModelIndex &index) const
{
	QFont f(option.font);

    f.setPointSize(10);
    f.setWeight(QFont::Normal);

	return QFontMetrics(f).boundingRect(index.data(Qt::UserRole).toString())
			.adjusted(0, 0, 1, 1);
}

qreal DelegatePrivate::timestampFontPointSize(const QFont &f) const
{
	return 0.85*f.pointSize();
}

QRect DelegatePrivate::messageBox(const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const
{
//    QRect r = option.fontMetrics.boundingRect(option.text).adjusted(0, 0, 1, 1);
    QRect r = option.fontMetrics.boundingRect(0, 0, option.widget->width() - timestampBox(option, index).width() - iconSize.width() - spacingHorizontal - margins.left(), 250, Qt::TextWordWrap, option.text).adjusted(0, 0, 1, 1);
//    QRect r = option.fontMetrics.boundingRect(0, 0, option.widget->width() - iconSize.width() - spacingHorizontal * 4, 250, Qt::TextWrapAnywhere, option.text).adjusted(0, 0, 1, 1);
//    r.setWidth(250);
//    r.setHeight(r.height() * 3);
    return r;
}
