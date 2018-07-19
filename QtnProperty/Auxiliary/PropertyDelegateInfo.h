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

#ifndef QTN_PROPERTY_DELEGATE_INFO_H
#define QTN_PROPERTY_DELEGATE_INFO_H

#include "QtnProperty/CoreAPI.h"
#include <QMap>
#include <QVariant>

QTN_IMPORT_EXPORT QByteArray qtnFieldDelegateName();

typedef QMap<QByteArray, QVariant> QtnPropertyDelegateAttributes;

struct QTN_IMPORT_EXPORT QtnPropertyDelegateInfo
{
	QByteArray name;
	QtnPropertyDelegateAttributes attributes;

	QtnPropertyDelegateInfo();
	QtnPropertyDelegateInfo(const QtnPropertyDelegateInfo &other);
};

template <typename T>
inline bool qtnGetAttribute(const QtnPropertyDelegateAttributes &attributes,
	const QByteArray &attributeName, T &attributeValue)
{
	auto it = attributes.find(attributeName);

	if (it == attributes.end())
		return false;

	attributeValue = it.value().value<T>();
	return true;
}

#endif // QTN_PROPERTY_DELEGATE_INFO_H
