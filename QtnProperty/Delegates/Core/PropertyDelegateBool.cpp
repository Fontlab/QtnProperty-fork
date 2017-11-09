/*******************************************************************************
Copyright 2012-2015 Alex Zhondin <qtinuum.team@gmail.com>
Copyright 2015-2017 Alexandra Cherdantseva <neluhus.vagus@gmail.com>

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

#include "PropertyDelegateBool.h"

#include "Core/PropertyBool.h"
#include "Delegates/PropertyDelegateFactory.h"
#include "Delegates/PropertyEditorHandler.h"
#include "PropertyDelegateAttrs.h"

#include <QStyleOption>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>

QByteArray qtnCheckBoxDelegate()
{
	returnQByteArrayLiteral("CheckBox");
}

QByteArray qtnLabelFalseAttr()
{
	returnQByteArrayLiteral("labelFalse");
}

QByteArray qtnLabelTrueAttr()
{
	returnQByteArrayLiteral("labelTrue");
}

class QtnPropertyBoolComboBoxHandler
	: public QtnPropertyEditorHandlerVT<QtnPropertyBoolBase, QComboBox>
{
public:
	QtnPropertyBoolComboBoxHandler(
		QtnPropertyBoolBase &property, QComboBox &editor);

protected:
	virtual void updateEditor() override;

private:
	void onCurrentIndexChanged(int index);
};

QtnPropertyDelegateBoolCheck::QtnPropertyDelegateBoolCheck(
	QtnPropertyBoolBase &owner)
	: QtnPropertyDelegateTyped<QtnPropertyBoolBase>(owner)
{
}

bool QtnPropertyDelegateBoolCheck::Register()
{
	return QtnPropertyDelegateFactory::staticInstance().registerDelegateDefault(
		&QtnPropertyBoolBase::staticMetaObject,
		&qtnCreateDelegate<QtnPropertyDelegateBoolCheck, QtnPropertyBoolBase>,
		qtnCheckBoxDelegate());
}

void QtnPropertyDelegateBoolCheck::drawValueImpl(QStylePainter &painter,
	const QRect &rect, const QStyle::State &state, bool *) const
{
	QStyleOptionButton opt;
	opt.rect = rect;
	opt.state = state;

	opt.state |= owner().value() ? QStyle::State_On : QStyle::State_Off;

	painter.drawControl(QStyle::CE_CheckBox, opt);
}

QWidget *QtnPropertyDelegateBoolCheck::createValueEditorImpl(
	QWidget *, const QRect &, QtnInplaceInfo *)
{
	if (owner().isEditableByUser())
		owner().edit(!owner().value());

	return nullptr;
}

QtnPropertyDelegateBoolCombobox::QtnPropertyDelegateBoolCombobox(
	QtnPropertyBoolBase &owner)
	: QtnPropertyDelegateTyped<QtnPropertyBoolBase>(owner)
{
	m_labels[0] = QtnPropertyBool::getBoolText(false, false);
	m_labels[1] = QtnPropertyBool::getBoolText(true, false);
}

bool QtnPropertyDelegateBoolCombobox::Register()
{
	return QtnPropertyDelegateFactory::staticInstance().registerDelegate(
		&QtnPropertyBoolBase::staticMetaObject,
		&qtnCreateDelegate<QtnPropertyDelegateBoolCombobox,
			QtnPropertyBoolBase>,
		qtnComboBoxDelegate());
}

void QtnPropertyDelegateBoolCombobox::applyAttributesImpl(
	const QtnPropertyDelegateAttributes &attributes)
{
	qtnGetAttribute(attributes, qtnLabelFalseAttr(), m_labels[0]);
	qtnGetAttribute(attributes, qtnLabelTrueAttr(), m_labels[1]);
}

QWidget *QtnPropertyDelegateBoolCombobox::createValueEditorImpl(
	QWidget *parent, const QRect &rect, QtnInplaceInfo *inplaceInfo)
{
	if (owner().isEditableByUser())
	{
		QComboBox *comboBox = new QComboBox(parent);
		comboBox->addItem(m_labels[0], false);
		comboBox->addItem(m_labels[1], true);

		comboBox->setGeometry(rect);

		// connect widget and property
		new QtnPropertyBoolComboBoxHandler(owner(), *comboBox);

		if (inplaceInfo)
			comboBox->showPopup();

		return comboBox;
	}

	return createValueEditorLineEdit(parent, rect, true, inplaceInfo);
}

bool QtnPropertyDelegateBoolCombobox::propertyValueToStr(
	QString &strValue) const
{
	strValue = m_labels[owner().value() ? 1 : 0];
	return true;
}

QtnPropertyBoolComboBoxHandler::QtnPropertyBoolComboBoxHandler(
	QtnPropertyBoolBase &property, QComboBox &editor)
	: QtnPropertyEditorHandlerVT(property, editor)
{
	updateEditor();

	if (!property.isEditableByUser())
		editor.setDisabled(true);

	QObject::connect(&editor,
		static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
		this, &QtnPropertyBoolComboBoxHandler::onCurrentIndexChanged);
}

void QtnPropertyBoolComboBoxHandler::updateEditor()
{
	updating++;

	if (property().valueIsHidden())
		editor().setCurrentIndex(-1);
	else
	{
		int index = editor().findData(property().value());

		if (index >= 0)
			editor().setCurrentIndex(index);
	}

	updating--;
}

void QtnPropertyBoolComboBoxHandler::onCurrentIndexChanged(int index)
{
	if (index >= 0)
	{
		auto data = editor().itemData(index);

		if (data.canConvert<bool>())
		{
			onValueChanged(data.toBool());
		}
	}
}
