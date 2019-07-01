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

#include "PropertyDelegate.h"
#include "Utils/PropertyEditorHandler.h"

QtnPropertyDelegate::QtnPropertyDelegate(QtnPropertyBase &ownerProperty)
	: m_ownerProperty(&ownerProperty)
	, m_stateProperty(nullptr)
{
}

QtnPropertyDelegate::~QtnPropertyDelegate()
{
	if (m_editorHandler)
		m_editorHandler->cleanup();
}

void QtnPropertyDelegate::init()
{
	// do nothing
}

QtnPropertyChangeReason QtnPropertyDelegate::editReason() const
{
	QtnPropertyChangeReason result = QtnPropertyChangeReasonEditValue;
	if (stateProperty()->isMultiValue())
		result |= QtnPropertyChangeReasonEditMultiValue;
	return result;
}

void QtnPropertyDelegate::applySubPropertyInfo(
	const QtnPropertyDelegateInfo &info, const QtnSubPropertyInfo &subInfo)
{
	auto p = subProperty(subInfo.id);
	p->setName(subInfo.key);
	info.storeAttributeValue(subInfo.displayNameAttr, p,
		&QtnPropertyBase::displayName, &QtnPropertyBase::setDisplayName);
	info.storeAttributeValue(subInfo.descriptionAttr, p,
		&QtnPropertyBase::description, &QtnPropertyBase::setDescription);
}

int QtnPropertyDelegate::subPropertyCountImpl() const
{
	return 0;
}

QtnPropertyBase *QtnPropertyDelegate::subPropertyImpl(int index)
{
	Q_UNUSED(index);
	return nullptr;
}

void QtnPropertyDelegate::applyAttributesImpl(
	const QtnPropertyDelegateInfo &info)
{
	Q_UNUSED(info);
}

QStyle::State QtnPropertyDelegate::state(
	bool isActive, const QtnSubItem &subItem) const
{
	auto subState = subItem.state();
	QStyle::State state = QStyle::State_Active;
	if (stateProperty()->isEditableByUser())
		state |= QStyle::State_Enabled;
	if (isActive)
	{
		state |= QStyle::State_Selected;
		state |= QStyle::State_HasFocus;
	}

	if (subState == QtnSubItemStateUnderCursor)
		state |= QStyle::State_MouseOver;
	else if (subState == QtnSubItemStatePushed)
		state |= QStyle::State_Sunken;

	return state;
}
