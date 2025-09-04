/*******************************************************************************
Copyright (c) 2012-2016 Alex Zhondin <lexxmark.dev@gmail.com>
Copyright (c) 2015-2025 Alexandra Cherdantseva <neluhus.vagus@gmail.com>

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

#pragma once

#include "QtnProperty/Delegates/Utils/PropertyDelegateMisc.h"
#include "QtnProperty/Core/PropertyEnum.h"

class QToolButton;
class QLabel;

// Name helper for the delegate
QTN_IMPORT_EXPORT QByteArray qtnEnumButtonsDelegateName();

class QTN_IMPORT_EXPORT QtnPropertyDelegateEnumButtons
	: public QtnPropertyDelegateTyped<QtnPropertyEnumBase>
{
	Q_DISABLE_COPY(QtnPropertyDelegateEnumButtons)

public:
	QtnPropertyDelegateEnumButtons(QtnPropertyEnumBase &owner);

	static void Register(QtnPropertyDelegateFactory &factory);

protected:
	virtual QWidget *createValueEditorImpl(QWidget *parent, const QRect &rect,
		QtnInplaceInfo *inplaceInfo = nullptr) override;

	virtual bool propertyValueToStrImpl(QString &strValue) const override;

	virtual void drawValueImpl(QStylePainter &painter, const QRect &rect) const override;

	virtual void applyAttributesImpl(const QtnPropertyDelegateInfo &info) override;

	virtual bool createSubItemValueImpl(
		QtnDrawContext &context, QtnSubItem &subItemValue) override;

private:
	bool m_showLabels = true;
	bool m_isActiveRow = false;
	bool m_isDarkMode = false;
};


