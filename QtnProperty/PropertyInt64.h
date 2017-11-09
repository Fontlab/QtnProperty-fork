/*******************************************************************************
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

#pragma once

#include "Auxiliary/PropertyTemplates.h"
#include "Delegates/PropertyDelegate.h"

class QTN_IMPORT_EXPORT QtnPropertyInt64Base
	: public QtnNumericPropertyBase<QtnSinglePropertyBase<qint64>>
{
	Q_OBJECT

private:
	QtnPropertyInt64Base(const QtnPropertyInt64Base &other) Q_DECL_EQ_DELETE;

public:
	explicit QtnPropertyInt64Base(QObject *parent);

protected:
	// string conversion implementation
	virtual bool fromStrImpl(const QString &str, bool edit) override;
	virtual bool toStrImpl(QString &str) const override;

	// variant conversion implementation
	virtual bool fromVariantImpl(const QVariant &var, bool edit) override;

	P_PROPERTY_DECL_MEMBER_OPERATORS(QtnPropertyInt64Base)
};

class QTN_IMPORT_EXPORT QtnPropertyInt64
	: public QtnSinglePropertyValue<QtnPropertyInt64Base>
{
	Q_OBJECT

private:
	QtnPropertyInt64(const QtnPropertyInt64 &other) Q_DECL_EQ_DELETE;

public:
	explicit QtnPropertyInt64(QObject *parent);

	P_PROPERTY_DECL_MEMBER_OPERATORS2(QtnPropertyInt64, QtnPropertyInt64Base)

	static bool Register();
};

P_PROPERTY_DECL_ALL_OPERATORS(QtnPropertyInt64Base, qint64)

class QTN_IMPORT_EXPORT QtnPropertyInt64Callback
	: public QtnSinglePropertyCallback<QtnPropertyInt64Base>
{
	Q_OBJECT

private:
	QtnPropertyInt64Callback(
		const QtnPropertyInt64Callback &other) Q_DECL_EQ_DELETE;

public:
	explicit QtnPropertyInt64Callback(QObject *parent);

	P_PROPERTY_DECL_MEMBER_OPERATORS2(
		QtnPropertyInt64Callback, QtnPropertyInt64Base)
};

class QLineEdit;
class QTN_IMPORT_EXPORT QtnPropertyDelegateInt64
	: public QtnPropertyDelegateTyped<QtnPropertyInt64Base>
{
	Q_DISABLE_COPY(QtnPropertyDelegateInt64)

	QString suffix;

public:
	QtnPropertyDelegateInt64(QtnPropertyInt64Base &owner);

protected:
	virtual bool acceptKeyPressedForInplaceEditImpl(
		QKeyEvent *keyEvent) const override;

	virtual QWidget *createValueEditorImpl(QWidget *parent, const QRect &rect,
		QtnInplaceInfo *inplaceInfo = nullptr) override;

	virtual void applyAttributesImpl(
		const QtnPropertyDelegateAttributes &attributes) override;

	virtual bool propertyValueToStr(QString &strValue) const override;
};
