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

#include "PropertyDelegateSliderBox.h"

// OpacityBox delegate: visually represents 0..1 range as a checkerboard
// background with a left-to-right alpha gradient of a configurable color.

class QTN_IMPORT_EXPORT QtnPropertyDelegateOpacityBoxBase
	: public QtnPropertyDelegateSlideBox
{
	Q_DISABLE_COPY(QtnPropertyDelegateOpacityBoxBase)

protected:
	QtnPropertyDelegateOpacityBoxBase(QtnPropertyBase &owner);

	virtual void applyAttributesImpl(const QtnPropertyDelegateInfo &info) override;
	virtual void draw(QtnDrawContext &context, const QtnSubItem &item) override;

	QColor m_backgroundColor;
	QColor m_opacityColor;
	bool m_useCheckerBackground;
	bool m_drawText = true;
};

template <typename PropertyClass>
class QtnPropertyDelegateOpacityBox
	: public QtnPropertyDelegateSlideBoxTyped<PropertyClass>
{
	Q_DISABLE_COPY(QtnPropertyDelegateOpacityBox)

	using ParentClass =
		QtnPropertyDelegateSlideBoxTyped<PropertyClass>;

public:
	QtnPropertyDelegateOpacityBox(PropertyClass &owner)
		: ParentClass(owner)
		, m_backgroundColor()
		, m_opacityColor(0, 0, 0)
		, m_useCheckerBackground(true)
	{
	}

protected:
	virtual void applyAttributesImpl(const QtnPropertyDelegateInfo &info) override
	{
		ParentClass::applyAttributesImpl(info);
		info.loadAttribute(qtnOpacityBackgroundColorAttr(), m_backgroundColor);
		info.loadAttribute(qtnOpacityBarColorAttr(), m_opacityColor);
		info.loadAttribute(qtnOpacityCheckerAttr(), m_useCheckerBackground);
		info.loadAttribute(qtnDrawTextAttr(), this->m_drawText);
	}

	virtual void draw(QtnDrawContext &context, const QtnSubItem &item) override
	{
		// Determine current value part similar to SlideBox
		double valuePart =
			(item.state() == QtnSubItemStatePushed || this->m_animate)
				? this->dragValuePart()
				: this->propertyValuePart();
		if (valuePart < 0.0)
			return;

		auto boxRect = item.rect;
		boxRect.adjust(2, 2, -2, -2);

		auto &painter = *context.painter;
		painter.save();

		// Background (checkerboard or solid color)
		if (m_useCheckerBackground && !boxRect.isEmpty())
		{
			QImage img(boxRect.size(), QImage::Format_ARGB32_Premultiplied);
			// two grays like typical transparency pattern, adapted for dark mode
			const bool isDark = context.isDarkMode;
			QColor c1 = isDark ? QColor("#1e1e1e") : QColor("#fafafa");
			QColor c2 = isDark ? QColor("#323334") : QColor("#f4f4f4");
			for (int y = 0; y < img.height(); ++y)
			{
				QRgb *scan = reinterpret_cast<QRgb *>(img.scanLine(y));
				for (int x = 0; x < img.width(); ++x)
				{
					bool alt = (((x >> 3) ^ (y >> 3)) & 0x1) != 0;
					scan[x] = (alt ? c2 : c1).rgb();
				}
			}
			painter.drawImage(boxRect.topLeft(), img);
		} else
		{
			QColor bg = m_backgroundColor.isValid()
					? m_backgroundColor
					: painter.style()->standardPalette().color(
							this->stateProperty()->isEditableByUser() ? QPalette::Active
																			: QPalette::Disabled,
							QPalette::Background);
			painter.fillRect(boxRect, bg);
		}

		// Foreground gradient from transparent to m_opacityColor
		QLinearGradient grad(boxRect.topLeft(), boxRect.topRight());
		QColor c0 = m_opacityColor.isValid() ? m_opacityColor : QColor(0, 0, 0);
		// consult optional color callback
		QColor c1 = c0;
		c0.setAlpha(0);
		c1.setAlpha(255);
		grad.setColorAt(0.0, c0);
		grad.setColorAt(1.0, c1);
		painter.fillRect(boxRect, grad);

		// Marker at current value
		int x = boxRect.left() + int((boxRect.width() - 1) * valuePart);
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setPen(QColor(255, 255, 255, 200));
		painter.drawLine(x - 1, boxRect.top(), x - 1, boxRect.bottom());
		painter.drawLine(x + 1, boxRect.top(), x + 1, boxRect.bottom());
		QPen redPen(Qt::red, 1.5, Qt::DotLine);
		redPen.setDashPattern({0.17, 1.7});
		painter.setPen(redPen);
		painter.drawLine(x, boxRect.top(), x, boxRect.bottom());

		// Border
		if (this->m_drawBorder)
		{
			painter.setPen(context.textColorFor(this->stateProperty()->isEditableByUser()));
			painter.drawRect(boxRect);
		}

		painter.restore();

		if (this->m_drawText)
			qtnDrawValueText(
				this->valuePartToStr(valuePart), painter, boxRect, context.style());
	}

private:
	QColor m_backgroundColor;
	QColor m_opacityColor;
	bool m_useCheckerBackground;
};


