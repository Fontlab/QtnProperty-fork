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

#include "PropertyDelegateOpacityBox.h"

#include "QtnProperty/PropertyDelegateAttrs.h"
#include "QtnProperty/PropertyView.h"

#include <QPainter>

QByteArray qtnOpacityBoxDelegate()
{
	return QByteArrayLiteral("OpacityBox");
}

QByteArray qtnOpacityBackgroundColorAttr()
{
	return QByteArrayLiteral("opacityBackground");
}

QByteArray qtnOpacityBarColorAttr()
{
	return QByteArrayLiteral("opacityColor");
}

QByteArray qtnOpacityCheckerAttr()
{
	return QByteArrayLiteral("opacityChecker");
}

QtnPropertyDelegateOpacityBoxBase::QtnPropertyDelegateOpacityBoxBase(
	QtnPropertyBase &owner)
	: QtnPropertyDelegateSlideBox(owner)
	, m_backgroundColor()
	, m_opacityColor(Qt::black)
	, m_useCheckerBackground(true)
{
}

void QtnPropertyDelegateOpacityBoxBase::applyAttributesImpl(
	const QtnPropertyDelegateInfo &info)
{
	QtnPropertyDelegateSlideBox::applyAttributesImpl(info);
	info.loadAttribute(qtnOpacityBackgroundColorAttr(), m_backgroundColor);
	info.loadAttribute(qtnOpacityBarColorAttr(), m_opacityColor);
	info.loadAttribute(qtnOpacityCheckerAttr(), m_useCheckerBackground);
	info.loadAttribute(qtnDrawTextAttr(), m_drawText);
}

void QtnPropertyDelegateOpacityBoxBase::draw(
	QtnDrawContext &context, const QtnSubItem &item)
{
	// Use the implementation in the typed class for actual drawing
	QtnPropertyDelegateSlideBox::draw(context, item);
}


