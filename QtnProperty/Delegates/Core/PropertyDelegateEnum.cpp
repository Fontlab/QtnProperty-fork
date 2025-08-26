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
