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

#include "PropertyQSize.h"
#include "PropertyInt.h"

QtnPropertyQSizeBase::QtnPropertyQSizeBase(QObject *parent)
	: ParentClass(parent)
{
	addState(QtnPropertyStateCollapsed);
}

QtnProperty *QtnPropertyQSizeBase::createWidthProperty()
{
	return createFieldProperty(QtnPropertyQSize::widthString(),
		QtnPropertyQSize::widthDescription(), &QSize::width, &QSize::setWidth);
}

QtnProperty *QtnPropertyQSizeBase::createHeightProperty()
{
	return createFieldProperty(QtnPropertyQSize::heightString(),
		QtnPropertyQSize::heightDescription(), &QSize::height,
		&QSize::setHeight);
}

bool QtnPropertyQSizeBase::fromStrImpl(const QString &str, bool edit)
{
	static QRegExp parserSize(
		QStringLiteral("^\\s*QSize\\s*\\(([^\\)]+)\\)\\s*$"),
		Qt::CaseInsensitive);

	if (!parserSize.exactMatch(str))
		return false;

	QStringList params = parserSize.capturedTexts();

	if (params.size() != 2)
		return false;

	static QRegExp parserParams(
		QStringLiteral("^\\s*(-?\\d+)\\s*,\\s*(-?\\d+)\\s*$"),
		Qt::CaseInsensitive);

	if (!parserParams.exactMatch(params[1]))
		return false;

	params = parserParams.capturedTexts();

	if (params.size() != 3)
		return false;

	bool ok = false;
	int width = params[1].toInt(&ok);

	if (!ok)
		return false;

	int height = params[2].toInt(&ok);

	if (!ok)
		return false;

	return setValue(QSize(width, height), edit);
}

bool QtnPropertyQSizeBase::toStrImpl(QString &str) const
{
	QSize v = value();
	str = QString("QSize(%1, %2)").arg(v.width()).arg(v.height());
	return true;
}

QtnPropertyQSize::QtnPropertyQSize(QObject *parent)
	: QtnSinglePropertyValue<QtnPropertyQSizeBase>(parent)
{
}

QString QtnPropertyQSize::getToStringFormat()
{
	return tr("[%1 x %2]");
}

QString QtnPropertyQSize::widthString()
{
	return tr("Width");
}

QString QtnPropertyQSize::widthDescription()
{
	return tr("Width of the %1");
}

QString QtnPropertyQSize::heightString()
{
	return tr("Height");
}

QString QtnPropertyQSize::heightDescription()
{
	return tr("Height of the %1");
}

QtnPropertyQSizeCallback::QtnPropertyQSizeCallback(QObject *parent)
	: QtnSinglePropertyCallback<QtnPropertyQSizeBase>(parent)
{
}
