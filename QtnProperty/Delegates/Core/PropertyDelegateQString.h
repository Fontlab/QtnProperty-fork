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

#ifndef PROPERTY_DELEGATE_QSTRING_H
#define PROPERTY_DELEGATE_QSTRING_H

#include "QtnProperty/Delegates/PropertyDelegate.h"

class QtnPropertyQStringBase;

class QTN_IMPORT_EXPORT QtnPropertyDelegateQString
	: public QtnPropertyDelegateTyped<QtnPropertyQStringBase>
{
	Q_DISABLE_COPY(QtnPropertyDelegateQString)

public:
	typedef QtnPropertyDelegateTyped<QtnPropertyQStringBase> Inherited;

	QtnPropertyDelegateQString(QtnPropertyQStringBase &owner);

	static bool Register();

protected:
	virtual void applyAttributesImpl(
		const QtnPropertyDelegateAttributes &attributes) override;

	virtual bool acceptKeyPressedForInplaceEditImpl(
		QKeyEvent *keyEvent) const override;

	virtual QWidget *createValueEditorImpl(QWidget *parent, const QRect &rect,
		QtnInplaceInfo *inplaceInfo = nullptr) override;

	virtual bool propertyValueToStr(QString &strValue) const override;

	virtual void drawValueImpl(QStylePainter &painter, const QRect &rect,
		const QStyle::State &state, bool *needTooltip = nullptr) const override;

private:
	int maxLength;
};

class QTN_IMPORT_EXPORT QtnPropertyDelegateQStringInvalidBase
	: public QtnPropertyDelegateQString
{
	Q_DISABLE_COPY(QtnPropertyDelegateQStringInvalidBase)

protected:
	QtnPropertyDelegateQStringInvalidBase(QtnPropertyQStringBase &owner);

	virtual void applyAttributesImpl(
		const QtnPropertyDelegateAttributes &attributes) override;
	virtual void drawValueImpl(QStylePainter &painter, const QRect &rect,
		const QStyle::State &state, bool *needTooltip = nullptr) const override;

	virtual bool isPropertyValid() const = 0;

private:
	QColor m_invalidColor;
};

class QTN_IMPORT_EXPORT QtnPropertyDelegateQStringFile
	: public QtnPropertyDelegateQStringInvalidBase
{
	Q_DISABLE_COPY(QtnPropertyDelegateQStringFile)

public:
	QtnPropertyDelegateQStringFile(QtnPropertyQStringBase &owner);

	static bool Register();

protected:
	virtual void applyAttributesImpl(
		const QtnPropertyDelegateAttributes &attributes) override;
	virtual bool propertyValueToStr(QString &strValue) const override;
	virtual QString toolTipImpl() const override;
	virtual void drawValueImpl(QStylePainter &painter, const QRect &rect,
		const QStyle::State &state, bool *needToolTip = nullptr) const override;
	virtual QWidget *createValueEditorImpl(QWidget *parent, const QRect &rect,
		QtnInplaceInfo *inplaceInfo = nullptr) override;

	virtual bool isPropertyValid() const override;
	QString defaultDirectory() const;
	QString absoluteFilePath() const;
	QString relativeFilePath() const;
	bool shouldShowRelativePath() const;

private:
	QtnPropertyDelegateAttributes m_editorAttributes;
};

class QTN_IMPORT_EXPORT QtnPropertyDelegateQStringList
	: public QtnPropertyDelegateQString
{
	Q_DISABLE_COPY(QtnPropertyDelegateQStringList)

public:
	QtnPropertyDelegateQStringList(QtnPropertyQStringBase &owner);

	static bool Register();

protected:
	virtual void applyAttributesImpl(
		const QtnPropertyDelegateAttributes &attributes) override;

	virtual QWidget *createValueEditorImpl(QWidget *parent, const QRect &rect,
		QtnInplaceInfo *inplaceInfo = nullptr) override;

private:
	QStringList m_items;

	QtnPropertyDelegateAttributes m_editorAttributes;
};

#endif // PROPERTY_DELEGATE_QSTRING_H
