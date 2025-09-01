/*******************************************************************************
Copyright (c) 2012-2016 Alex Zhondin <lexxmark.dev@gmail.com>
Copyright (c) 2015-2019 Alexandra Cherdantseva <neluhus.vagus@gmail.com>

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*******************************************************************************/

#include "PropertyDelegateEnum.h"
#include "QtnProperty/Delegates/PropertyDelegateFactory.h"
#include "QtnProperty/Delegates/Utils/PropertyEditorHandler.h"
#include "QtnProperty/Delegates/Utils/PropertyEditorAux.h"

#include <QComboBox>
#include <QLineEdit>
#include <QListView>
#include <QAbstractItemView>
#include <QItemDelegate>
#include <QStyle>

class QtnPropertyEnumComboBoxHandler
	: public QtnPropertyEditorHandlerVT<QtnPropertyEnumBase, QComboBox>
{
public:
	QtnPropertyEnumComboBoxHandler(
		QtnPropertyDelegate *delegate, QComboBox &editor);

protected:
	virtual void updateEditor() override;

private:
	void onCurrentIndexChanged(int index);
};

void QtnPropertyDelegateEnum::Register(QtnPropertyDelegateFactory &factory)
{
	factory.registerDelegateDefault(&QtnPropertyEnumBase::staticMetaObject,
		&qtnCreateDelegate<QtnPropertyDelegateEnum, QtnPropertyEnumBase>,
		qtnComboBoxDelegate());
}

QtnPropertyDelegateEnum::QtnPropertyDelegateEnum(QtnPropertyEnumBase &owner)
	: QtnPropertyDelegateTyped<QtnPropertyEnumBase>(owner)
{
}

class QtnComboBoxDelegate : public QItemDelegate
{
public:
	QtnComboBoxDelegate(int requiredHeight, QWidget *parent)
		: QItemDelegate(parent), m_requiredHeight(requiredHeight)
	{}

	virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
	{
		QSize size = QItemDelegate::sizeHint(option, index);
		size.setHeight(m_requiredHeight);
		return size;
	}

private:
	int m_requiredHeight;
};

QWidget *QtnPropertyDelegateEnum::createValueEditorImpl(
	QWidget *parent, const QRect &rect, QtnInplaceInfo *inplaceInfo)
{
	const QtnEnumInfo *info = owner().enumInfo();

	if (!info)
		return 0;

	QComboBox *combo = new QtnPropertyComboBox(this, parent);
	// Apply popup item height from owning view if provided
	int itemHeightPx = 0;
	for (QWidget *w = parent; w; w = w->parentWidget())
	{
		// viewport carries the dynamic property; view may also expose it
		QVariant v = w->property("qtnComboPopupItemHeightPx");
		if (v.isValid()) { itemHeightPx = v.toInt(); break; }
	}
	if (itemHeightPx > 0)
	{
		QListView *lv = qobject_cast<QListView *>(combo->view());
		if (!lv)
		{
			lv = new QListView(combo);
			combo->setView(lv);
		}

		if (!lv->itemDelegate() || !dynamic_cast<QtnComboBoxDelegate *>(lv->itemDelegate()))
		{
			lv->setItemDelegate(new QtnComboBoxDelegate(itemHeightPx, combo));
		}
	}
	info->forEachEnumValue([combo](const QtnEnumValueInfo &value) -> bool {
		if (!value.icon().isNull())
			combo->addItem(value.icon(), value.displayName(), QVariant(value.value()));
		else
			combo->addItem(value.displayName(), QVariant(value.value()));
		return true;
	});

	combo->setGeometry(rect.adjusted(0, 0, 0, -1));

	new QtnPropertyEnumComboBoxHandler(this, *combo);

	if (inplaceInfo && stateProperty()->isEditableByUser())
		combo->showPopup();

	return combo;
}

bool QtnPropertyDelegateEnum::propertyValueToStrImpl(QString &strValue) const
{
	const QtnEnumInfo *info = owner().enumInfo();
	const QtnEnumValueInfo *valueInfo =
		info ? info->findByValue(owner().value()) : 0;

	if (!valueInfo)
		return false;

	strValue = valueInfo->displayName();
	return true;
}

void QtnPropertyDelegateEnum::drawValueImpl(QStylePainter &painter, const QRect &rect) const
{
	// draw the value text with the default logic first
	QtnPropertyDelegateWithValueEditor::drawValueImpl(painter, rect);

	// do not draw underline for multi-value placeholder
	if (stateProperty()->isMultiValue())
		return;

	QString strValue;
	if (!propertyValueToStrImpl(strValue) || strValue.isEmpty())
		return;

	QRect textRect = rect;
	if (auto style = painter.style())
	{
		textRect.adjust(style->pixelMetric(QStyle::PM_ButtonMargin), 0, 0, 0);
	}

	const QFontMetrics &fm = painter.fontMetrics();
	const QString elided = qtnElidedText(painter, strValue, textRect, nullptr);
	if (elided.isEmpty())
		return;

	int textWidth = fm.width(elided);
	int x1 = textRect.left();
	int x2 = x1 + textWidth;
	if (x2 > textRect.right())
		x2 = textRect.right();
	if (x2 <= x1)
		return;

	int top = textRect.top() + (textRect.height() - fm.height()) / 2;
	int baseline = top + fm.ascent();
	int underlineY = baseline + fm.underlinePos();

	QPen oldPen = painter.pen();
	QPen pen = oldPen;
	QColor c = pen.color();
	c.setAlphaF(c.alphaF() * 0.5);
	pen.setColor(c);
	painter.setPen(pen);
	painter.drawLine(x1, underlineY, x2, underlineY);
	painter.setPen(oldPen);
}

QtnPropertyEnumComboBoxHandler::QtnPropertyEnumComboBoxHandler(
	QtnPropertyDelegate *delegate, QComboBox &editor)
	: QtnPropertyEditorHandlerVT(delegate, editor)
{
	updateEditor();

	QObject::connect(&editor,
		static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
		this, &QtnPropertyEnumComboBoxHandler::onCurrentIndexChanged);
}

void QtnPropertyEnumComboBoxHandler::updateEditor()
{
	updating++;
	editor().setEnabled(stateProperty()->isEditableByUser());

	if (stateProperty()->isMultiValue())
		editor().setCurrentIndex(-1);
	else
	{
		int index = editor().findData(property().value());
		editor().setCurrentIndex(index);
	}

	updating--;
}

void QtnPropertyEnumComboBoxHandler::onCurrentIndexChanged(int index)
{
	if (index >= 0)
	{
		QVariant data = editor().itemData(index);

		if (data.canConvert<int>())
			onValueChanged(data.toInt());
	}
}
