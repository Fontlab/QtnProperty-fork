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

#include "PropertyDelegateEnumButtons.h"

#include "QtnProperty/Delegates/PropertyDelegateFactory.h"
#include "QtnProperty/Delegates/Utils/PropertyEditorHandler.h"

#include <QToolButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QButtonGroup>
#include <QMouseEvent>
#include <QPointer>
#include <QPainterPath>
#include <QStyleOptionToolButton>
#include <QApplication>
#include <PropertyView.h>

// Public name of the delegate used in attributes
QByteArray qtnEnumButtonsDelegateName()
{
	return QByteArrayLiteral("EnumButtons");
}

class QtnEnumButtonsEditor : public QWidget
{
public:
	explicit QtnEnumButtonsEditor(QWidget *parent = nullptr)
		: QWidget(parent)
		, layout(new QHBoxLayout(this))
		, group(new QButtonGroup(this))
	{
		layout->setMargin(0);
		layout->setSpacing(4);
		group->setExclusive(true);
		setAutoFillBackground(true);
	}

	QHBoxLayout *layout;
	QButtonGroup *group;
	QVector<QToolButton *> buttons;
	QVector<QLabel *> labels; // when icons used
};

class ClickProxyLabel : public QLabel
{
public:
	explicit ClickProxyLabel(QWidget *parent, QToolButton *target)
		: QLabel(parent), m_target(target) {}

protected:
	void mousePressEvent(QMouseEvent *event) override
	{
		QLabel::mousePressEvent(event);
		if (m_target)
			m_target->click();
	}

private:
	QPointer<QToolButton> m_target;
};

class QtnPropertyEnumButtonsHandler
	: public QtnPropertyEditorHandlerVT<QtnPropertyEnumBase, QtnEnumButtonsEditor>
{
public:
	QtnPropertyEnumButtonsHandler(QtnPropertyDelegate *delegate,
		QtnEnumButtonsEditor &editor)
		: QtnPropertyEditorHandlerVT(delegate, editor)
	{
		updateEditor();
	}

protected:
	virtual void updateEditor() override
	{
		updating++;
		const QtnEnumInfo *info = property().enumInfo();
		editor().setEnabled(stateProperty()->isEditableByUser());

		if (!info)
		{
			updating--;
			return;
		}

		if (stateProperty()->isMultiValue())
		{
			for (auto *btn : editor().buttons)
			{
				if (btn) btn->setChecked(false);
			}
		} else
		{
			QVariant v(property().value());
			for (auto *btn : editor().buttons)
			{
				if (!btn) continue;
				btn->setChecked(btn->property("qtn_enum_value").toInt() == v.toInt());
			}
		}

		updating--;
	}

	// Avoid relying on delegate() pointer; compute reason from state property
	virtual void updateValue() override
	{
		if (this->propertyBase())
		{
			QtnPropertyChangeReason reason = QtnPropertyChangeReasonEdit;
			auto *sp = this->stateProperty();
			if (sp && sp->isMultiValue())
				reason |= QtnPropertyChangeReasonMultiEdit;
			this->property().setValue(newValue, reason);
		}
	}

private:
	void onClicked()
	{
		if (updating > 0)
			return;
		QToolButton *btn = qobject_cast<QToolButton *>(sender());
		if (!btn)
			return;
		bool ok = false;
		int val = btn->property("qtn_enum_value").toInt(&ok);
		if (ok)
			onValueChanged(val);
	}

public:
	void wireButtons()
	{
		for (auto *btn : editor().buttons)
		{
			QObject::connect(btn, &QToolButton::clicked, this,
				&QtnPropertyEnumButtonsHandler::onClicked);
		}
	}
};

QtnPropertyDelegateEnumButtons::QtnPropertyDelegateEnumButtons(
	QtnPropertyEnumBase &owner)
	: QtnPropertyDelegateTyped<QtnPropertyEnumBase>(owner)
{
}

void QtnPropertyDelegateEnumButtons::Register(QtnPropertyDelegateFactory &factory)
{
	factory.registerDelegate(&QtnPropertyEnumBase::staticMetaObject,
		&qtnCreateDelegate<QtnPropertyDelegateEnumButtons, QtnPropertyEnumBase>,
		qtnEnumButtonsDelegateName());
}

QWidget *QtnPropertyDelegateEnumButtons::createValueEditorImpl(
	QWidget *parent, const QRect &rect, QtnInplaceInfo * /*inplaceInfo*/)
{
	const QtnEnumInfo *info = owner().enumInfo();
	if (!info)
		return nullptr;

	auto *editor = new QtnEnumButtonsEditor(parent);
	editor->setGeometry(rect);

	// Build row of buttons (+labels when icons present)
	info->forEachEnumValue([this, editor](const QtnEnumValueInfo &valueInfo) {
		if (valueInfo.state() != QtnEnumValueStateNone)
			return true; // skip hidden/obsolete

		// Create the toolbutton
		QToolButton *btn = new QToolButton(editor);
    btn->setStyleSheet("QToolButton{background-color:transparent;} QToolButton:checked{background-color:palette(highlight);}");
		btn->setCheckable(true);
		btn->setFocusPolicy(Qt::StrongFocus);
		btn->setProperty("qtn_enum_value", valueInfo.value());

		if (!valueInfo.icon().isNull())
		{
			btn->setIcon(valueInfo.icon());
			btn->setFixedSize(24, 22);
			editor->layout->addWidget(btn);

			if (m_showLabels)
			{
				QLabel *lbl = new ClickProxyLabel(editor, btn);
				lbl->setText(valueInfo.displayName());
				lbl->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
				editor->layout->addWidget(lbl);
				editor->labels.push_back(lbl);
        editor->layout->addSpacerItem(new QSpacerItem(2, 1, QSizePolicy::Fixed));
			}
      editor->layout->addSpacerItem(new QSpacerItem(4, 1, QSizePolicy::Fixed));
		}
		else
		{
			// No icon: put text on the button, 22px height and 4px LR padding
			if (m_showLabels)
			{
				btn->setText(valueInfo.displayName());
			}
			btn->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
			btn->setFixedHeight(22);
			btn->setStyleSheet("QToolButton { padding-left: 4px; padding-right: 4px; }");
			editor->layout->addWidget(btn);
		}
    
		editor->buttons.push_back(btn);
		editor->group->addButton(btn);
		return true;
	});

	// spacing between widgets is 4 px (already set on layout); ensure min height
	editor->layout->setSpacing(3);
  editor->layout->setContentsMargins(4, 0, 0, 0);
  
  editor->layout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));

	// Preselect current value
	int current = owner().value();
	for (auto *btn : editor->buttons)
	{
		if (btn->property("qtn_enum_value").toInt() == current)
		{
			btn->setChecked(true);
			break;
		}
	}

	// Wire handler after building widgets
	auto *handler = new QtnPropertyEnumButtonsHandler(this, *editor);
	handler->wireButtons();

	return editor;
}

bool QtnPropertyDelegateEnumButtons::propertyValueToStrImpl(QString &strValue) const
{
	const QtnEnumInfo *info = owner().enumInfo();
	const QtnEnumValueInfo *valueInfo = info ? info->findByValue(owner().value()) : nullptr;
	if (!valueInfo)
		return false;
	strValue = valueInfo->displayName();
	return true;
}

void QtnPropertyDelegateEnumButtons::drawValueImpl(QStylePainter &painter, const QRect &rect) const
{
	if (stateProperty()->isMultiValue())
	{
		QtnPropertyDelegateWithValueEditor::drawValueImpl(painter, rect);
		return;
	}

	const QtnEnumInfo *info = owner().enumInfo();
	if (!info)
	{
		QtnPropertyDelegateWithValueEditor::drawValueImpl(painter, rect);
		return;
	}

	const int spacing = 4;
	const int iconBoxW = 24;
	const int iconBoxH = 22;
	const int iconPx = 16;
	int x = rect.left() + spacing;
	const int centerY = rect.top() + rect.height() / 2;
	const int current = owner().value();
	const QFontMetrics &fm = painter.fontMetrics();

	painter.save();
	painter.setRenderHint(QPainter::Antialiasing, true);

	// Rebuild hit rectangles during painting to avoid duplicating layout logic
	m_hitRects.clear();

	info->forEachEnumValue([&](const QtnEnumValueInfo &v) -> bool {
		if (v.state() != QtnEnumValueStateNone)
			return true;

		const bool hasIcon = !v.icon().isNull();
		const bool isActive = (v.value() == current);

		if (hasIcon)
		{
			QRect iconBox(x, centerY - iconBoxH / 2, iconBoxW, iconBoxH);

			if (isActive)
			{
				QPen oldPen = painter.pen();
        QColor hl = qApp->palette().color(QPalette::Highlight);
        QColor pen = m_isActiveRow ? hl.darker(m_isDarkMode ? 140 : 70) : hl;
				painter.setPen(pen);
				painter.setBrush(pen);
				painter.drawRoundedRect(iconBox, 4, 4);
				painter.setPen(oldPen);
			}

			v.icon().paint(&painter, QRect(x, centerY - iconPx / 2, iconBoxW, iconPx));

			// clickable: icon area
			m_hitRects.push_back({QRect(iconBox.left(), rect.top(), iconBox.width(), rect.height()), v.value()});

			x = iconBox.right() + spacing;

			// Label: elide to fit remaining width
			if (m_showLabels)
			{
				QString text = v.displayName();
				int remaining = rect.right() - x;
				if (remaining <= 0)
					return false;
				QRect availRect(x, centerY - fm.height() / 2, remaining, fm.height());
				QString elided = qtnElidedText(painter, text, availRect, nullptr);
				if (!elided.isEmpty())
				{
					// compute text width for precise advance
					int w = fm.width(elided);
					QRect labelRect(x, rect.top(), w + 2, rect.height());
					qtnDrawValueText(elided, painter, labelRect);
					// clickable: label area
					m_hitRects.push_back({QRect(labelRect.left(), rect.top(), labelRect.width(), rect.height()), v.value()});
					x = labelRect.right() + spacing * 2;
				}
			}
		}
		else
		{
			if (isActive)
			{
				QRect hi(x, centerY - iconBoxH / 2, iconBoxW, iconBoxH);
				QPen oldPen = painter.pen();
        QColor hl = qApp->palette().color(QPalette::Highlight);
        QColor pen = m_isActiveRow ? hl.darker(m_isDarkMode ? 140 : 70) : hl;
				painter.setPen(pen);
				painter.setBrush(pen);
				painter.drawRoundedRect(hi, 4, 4);
				painter.setPen(oldPen);
				// clickable: highlight box area
				m_hitRects.push_back({QRect(hi.left(), rect.top(), hi.width(), rect.height()), v.value()});
				x = hi.right() + spacing;
			}

			QString text = v.displayName();
			int remaining = rect.right() - x;
			if (remaining <= 0)
				return false;
			QRect availRect(x, centerY - fm.height() / 2, remaining, fm.height());
			QString elided = m_showLabels ? qtnElidedText(painter, text, availRect, nullptr) : QString();
			if (m_showLabels && !elided.isEmpty())
			{
				int w = fm.width(elided);
				QRect labelRect(x, centerY - fm.height() / 2, w, fm.height());
				qtnDrawValueText(elided, painter, labelRect);
				// clickable: label area
				m_hitRects.push_back({QRect(labelRect.left(), rect.top(), labelRect.width(), rect.height()), v.value()});
				x = labelRect.right() + spacing * 2;
			}
		}

		if (x >= rect.right())
			return false;
		return true;
	});

	painter.restore();
}

bool QtnPropertyDelegateEnumButtons::createSubItemValueImpl(
	QtnDrawContext &context, QtnSubItem &subItemValue)
{
	// Let the base set up event handling/tooltip and default drawing
	bool ok = QtnPropertyDelegateWithValueEditor::createSubItemValueImpl(context, subItemValue);
	if (!ok)
		return false;

	// Wrap the original draw handler to capture QtnDrawContext and then delegate
	auto origDraw = subItemValue.drawHandler;
	auto origEvent = subItemValue.eventHandler;
	subItemValue.drawHandler = [this, origDraw](QtnDrawContext &ctx, const QtnSubItem &item) {
		m_isActiveRow = ctx.isActive;
		m_isDarkMode = ctx.isDarkMode;
		if (origDraw)
			origDraw(ctx, item);
	};

	// Replace event handler to enable direct toggling without creating the editor
	subItemValue.eventHandler = [this, origEvent](QtnEventContext &ctx,
		const QtnSubItem &item, QtnPropertyToEdit *toEdit) -> bool {
		// Only handle simple clicks when editable and not multi-value
		if (!stateProperty()->isEditableByUser() || stateProperty()->isMultiValue())
		{
			return origEvent ? origEvent(ctx, item, toEdit) : false;
		}

		if (ctx.eventType() == QEvent::MouseButtonDblClick)
		{
			// Suppress editor activation on double-click for this delegate
			return true;
		}

		if (ctx.eventType() == QEvent::MouseButtonRelease)
		{
			QMouseEvent *me = ctx.eventAs<QMouseEvent>();
			if (me->button() == Qt::LeftButton)
			{
				// Use hit rects computed during drawing
				if (m_hitRects.isEmpty())
					return true; // consume click, do not open editor

				int clickedValue = std::numeric_limits<int>::min();
				for (const auto &h : m_hitRects)
				{
					if (h.rect.intersects(item.rect) && h.rect.contains(me->pos()))
					{
						clickedValue = h.value;
						break;
					}
				}

				if (clickedValue != std::numeric_limits<int>::min())
				{
					toEdit->setup(property(), [this, clickedValue]() -> QWidget * {
						owner().setValue(clickedValue, editReason());
						return nullptr;
					});
					return true;
				}

				// No hit: consume the click to avoid opening the editor
				return true;
			}
		}

		// Fallback to original behavior for other events
		return origEvent ? origEvent(ctx, item, toEdit) : false;
	};
	return true;
}

void QtnPropertyDelegateEnumButtons::applyAttributesImpl(const QtnPropertyDelegateInfo &info)
{
	QtnPropertyDelegateTyped<QtnPropertyEnumBase>::applyAttributesImpl(info);
	info.loadAttribute(qtnEnumButtonsShowLabelsAttr(), m_showLabels);
}


