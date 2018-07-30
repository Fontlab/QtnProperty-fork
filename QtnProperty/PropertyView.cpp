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

#include "PropertyWidget.h"

#include "Utils/InplaceEditing.h"

#include <QtGui>
#include <QApplication>
#include <QScrollBar>
#include <QStyleOptionFrameV3>
#include <QHelpEvent>
#include <QToolTip>

#include <memory>

class QtnPainterState
{
public:
	QtnPainterState(QPainter &p);
	~QtnPainterState();

private:
	QPainter &m_p;
};

QtnPropertyView::QtnPropertyView(QWidget *parent, QtnPropertySet *propertySet)
	: QAbstractScrollArea(parent)
	, m_propertySet(propertySet)
	, m_activeProperty(nullptr)
	, m_delegateFactory(&QtnPropertyDelegateFactory::staticInstance())
	, m_visibleItemsValid(false)
	, m_style(QtnPropertyViewStyleLiveSplit)
	, m_itemHeight(0)
	, m_itemHeightSpacing(6)
	, m_leadMargin(0)
	, m_splitRatio(0.5f)
	, m_rubberBand(nullptr)
	, m_accessibilityProxy(nullptr)
{
	setFocusPolicy(Qt::StrongFocus);
	viewport()->setMouseTracking(true);

	updateStyleStuff();

	updateItemsTree();
}

QtnPropertyView::~QtnPropertyView() {}

QtnAccessibilityProxy *QtnPropertyView::accessibilityProxy()
{
	if (!m_accessibilityProxy)
		m_accessibilityProxy = new QtnAccessibilityProxy(this);

	return m_accessibilityProxy;
}

void QtnPropertyView::onActivePropertyDestroyed()
{
	m_activeProperty = nullptr;
	emit activePropertyChanged(m_activeProperty);
	viewport()->update();
}

void QtnPropertyView::onEditedPropertyWillChange(
	QtnPropertyChangeReason reason, QtnPropertyValuePtr newValue, int typeId)
{
	if (0 != (reason & QtnPropertyChangeReasonEditValue))
	{
		auto property = qobject_cast<QtnPropertyBase *>(sender());
		Q_ASSERT(nullptr != property);

		auto rootProperty = property->getRootProperty()->asProperty();
		Q_ASSERT(nullptr != rootProperty);

		emit beforePropertyEdited(rootProperty, newValue, typeId);
	}
}

void QtnPropertyView::onEditedPropertyDidChange(QtnPropertyChangeReason reason)
{
	if (0 != (reason & QtnPropertyChangeReasonEditValue))
	{
		auto property = qobject_cast<QtnPropertyBase *>(sender());
		Q_ASSERT(nullptr != property);

		auto rootProperty = property->getRootProperty()->asProperty();
		Q_ASSERT(nullptr != rootProperty);

		emit propertyEdited(rootProperty);
	}
}

void QtnPropertyView::setPropertySet(QtnPropertySet *newPropertySet)
{
	if (newPropertySet == m_propertySet)
		return;

	m_propertySet = newPropertySet;
	updateItemsTree();
	viewport()->update();
}

QtnPropertyBase *QtnPropertyView::getPropertyParent(
	const QtnPropertyBase *property) const
{
	auto item = findItem(m_itemsTree.get(), property);

	if (nullptr != item && nullptr != item->parent)
		return item->parent->property;

	return nullptr;
}

bool QtnPropertyView::setActiveProperty(
	QtnPropertyBase *newActiveProperty, bool ensureVisible)
{
	if (m_activeProperty == newActiveProperty)
		return false;

	qtnStopInplaceEdit();

	if (ensureVisible)
		this->ensureVisible(newActiveProperty);

	if (!newActiveProperty)
	{
		setActivePropertyInternal(nullptr);
		return true;
	}

	int index = visibleItemIndexByProperty(newActiveProperty);

	if (index < 0)
		return false;

	setActivePropertyInternal(newActiveProperty);
	return true;
}

bool QtnPropertyView::setActiveProperty(int index, bool ensureVisible)
{
	if (index < 0)
		index = 0;

	if (nullptr == m_propertySet)
		return false;

	auto &cp = m_propertySet->childProperties();
	if (cp.isEmpty())
		return false;

	if (index >= cp.size())
		index = cp.size() - 1;

	return setActiveProperty(cp.at(index), ensureVisible);
}

bool QtnPropertyView::ensureVisible(const QtnPropertyBase *property)
{
	if (!property)
		return false;

	int index = visibleItemIndexByProperty(property);
	return ensureVisibleItemByIndex(index);
}

bool QtnPropertyView::setItemHeightSpacing(quint32 itemHeightSpacing)
{
	m_itemHeightSpacing = itemHeightSpacing;
	updateStyleStuff();
	return true;
}

void QtnPropertyView::setPropertyViewStyle(QtnPropertyViewStyle style)
{
	m_style = style;
}

void QtnPropertyView::addPropertyViewStyle(QtnPropertyViewStyle style)
{
	setPropertyViewStyle(propertyViewStyle() | style);
}

void QtnPropertyView::removePropertyViewStyle(QtnPropertyViewStyle style)
{
	setPropertyViewStyle(propertyViewStyle() & ~style);
}

QtnPropertyBase *QtnPropertyView::getPropertyAt(
	const QPoint &position, QRect *out_rect)
{
	int index = visibleItemIndexByPoint(position);

	if (index >= 0)
	{
		if (nullptr != out_rect)
		{
			*out_rect = visibleItemRect(index);
		}

		return m_visibleItems[index].item->property;
	}

	return nullptr;
}

void QtnPropertyView::connectPropertyToEdit(
	QtnPropertyBase *property, QtnConnections &outConnections)
{
	Q_ASSERT(nullptr != property);

	outConnections.push_back(
		QObject::connect(property, &QtnPropertyBase::propertyWillChange, this,
			&QtnPropertyView::onEditedPropertyWillChange));

	outConnections.push_back(
		QObject::connect(property, &QtnPropertyBase::propertyDidChange, this,
			&QtnPropertyView::onEditedPropertyDidChange));
}

void QtnPropertyView::paintEvent(QPaintEvent *e)
{
	Q_UNUSED(e);

	validateVisibleItems();

	if (m_visibleItems.isEmpty())
	{
		return;
	}

	int firstVisibleItemIndex =
		qMin(verticalScrollBar()->value() / m_itemHeight,
			(m_visibleItems.size() - 1));
	int lastVisibleItemIndex = qMin(
		((verticalScrollBar()->value() + viewport()->height()) / m_itemHeight) +
			1,
		(m_visibleItems.size() - 1));

	QRect itemRect = viewport()->rect();
	itemRect.setTop(
		firstVisibleItemIndex * m_itemHeight - verticalScrollBar()->value());
	itemRect.setBottom(itemRect.top() + m_itemHeight);

	QStylePainter painter(viewport());

	for (int i = firstVisibleItemIndex; i <= lastVisibleItemIndex; ++i)
	{
		const VisibleItem &vItem = m_visibleItems[i];

		if (!vItem.item->delegate)
			drawPropertySetItem(painter, itemRect, vItem);
		else
			drawPropertyItem(painter, itemRect, vItem);

		// mark as valid actions after any painting
		vItem.actionsValid = true;

		itemRect.translate(0, m_itemHeight);
	}
}

void QtnPropertyView::drawBranchNode(
	QStylePainter &painter, QRect &rect, const VisibleItem &vItem)
{
	if (!vItem.hasChildren)
		return;

	QRectF branchRect(rect.left(), rect.top(), rect.height(), rect.height());
	//painter.drawRect(branchRect);
	qreal side = branchRect.height() / 3.5f;

	QPainterPath branchPath;

	if (vItem.item->collapsed())
	{
		branchPath.moveTo(branchRect.left() + side, branchRect.top() + side);
		branchPath.lineTo(branchRect.right() - side - 1,
			branchRect.top() + branchRect.height() / 2.f);
		branchPath.lineTo(branchRect.left() + side, branchRect.bottom() - side);
		branchPath.closeSubpath();
	} else
	{
		branchPath.moveTo(branchRect.left() + side, branchRect.top() + side);
		branchPath.lineTo(branchRect.right() - side, branchRect.top() + side);
		branchPath.lineTo(branchRect.left() + branchRect.width() / 2.f,
			branchRect.bottom() - side - 1);
		branchPath.closeSubpath();
	}

	if (painter.testRenderHint(QPainter::Antialiasing))
	{
		painter.fillPath(branchPath, palette().color(QPalette::Text));
	} else
	{
		painter.setRenderHint(QPainter::Antialiasing, true);
		painter.fillPath(branchPath, palette().color(QPalette::Text));
		painter.setRenderHint(QPainter::Antialiasing, false);
	}

	// add action to collapse/expand branch
	if (!vItem.actionsValid)
	{
		Action branch;
		branch.rect = branchRect.toRect(); //opt.rect;

		QtnPropertyBase *property = vItem.item->property;
		branch.action = [property](QEvent *e, QRect) -> bool {
			if (e->type() == QEvent::MouseButtonPress)
			{
				property->toggleState(QtnPropertyStateCollapsed);
				return true;
			}

			return false;
		};

		vItem.actions.append(branch);
	}

	// skip branch space
	rect.setLeft(int(branchRect.right() + 1));
}

void QtnPropertyView::drawPropertySetItem(
	QStylePainter &painter, const QRect &rect, const VisibleItem &vItem)
{
	bool enabled = vItem.item->property->isEditableByUser();
	bool selected = m_activeProperty == vItem.item->property;

	QPalette::ColorGroup cg = enabled ? QPalette::Active : QPalette::Disabled;

	// fill background
	painter.fillRect(rect,
		selected ? palette().color(QPalette::Highlight)
				 : m_propertySetBackdroundColor);

	QRect nameRect = rect;

	// skip levels indent
	nameRect.setLeft(
		nameRect.left() + m_leadMargin + nameRect.height() * vItem.level);

	if (!nameRect.isValid())
		return;

	// draw branch node
	drawBranchNode(painter, nameRect, vItem);

	if (!nameRect.isValid())
		return;

	QtnPainterState s(painter);

	// draw name
	QFont font = painter.font();
	font.setBold(true);
	painter.setFont(font);

	painter.setPen(palette().color(
		cg, selected ? QPalette::HighlightedText : QPalette::Text));

	QString elidedName = painter.fontMetrics().elidedText(
		vItem.item->property->name(), Qt::ElideRight, nameRect.width());
	painter.drawText(nameRect,
		Qt::AlignLeading | Qt::AlignVCenter | Qt::TextSingleLine, elidedName);
}

void QtnPropertyView::drawPropertyItem(
	QStylePainter &painter, const QRect &rect, const VisibleItem &vItem)
{
	bool enabled = vItem.item->property->isEditableByUser();
	bool selected = m_activeProperty == vItem.item->property;

	QStyle::State state = QStyle::State_Active;

	if (enabled)
		state |= QStyle::State_Enabled;

	if (selected)
	{
		state |= QStyle::State_Selected;
		state |= QStyle::State_HasFocus;
	}

	QPalette::ColorGroup cg = enabled ? QPalette::Active : QPalette::Disabled;

	// fill background
	if (selected)
		painter.fillRect(rect, palette().color(QPalette::Highlight));

	int splitPos = splitPosition();

	QtnPainterState s(painter);

	QPen linesPen(m_linesColor);
	painter.setPen(linesPen);

	painter.drawLine(rect.bottomLeft(), rect.bottomRight());
	painter.drawLine(splitPos, rect.top(), splitPos, rect.bottom());

	QRect nameRect = rect;
	QRect valueRect = rect;
	QRect editRect = rect;

	nameRect.setRight(splitPosition());
	valueRect.setLeft(splitPosition() + m_leadMargin + 1);
	editRect.setLeft(splitPosition() + 1);

	// skip levels space
	nameRect.setLeft(
		nameRect.left() + m_leadMargin + nameRect.height() * vItem.level);

	if (!nameRect.isValid())
		return;

	// draw branch node
	drawBranchNode(painter, nameRect, vItem);

	if (!nameRect.isValid())
		return;

	// draw name
	painter.setPen(palette().color(
		cg, selected ? QPalette::HighlightedText : QPalette::Text));

	auto font = painter.font();
	auto property = vItem.item->property;
	font.setBold(!property->valueIsDefault());
	painter.setFont(font);

	painter.drawText(nameRect,
		Qt::AlignLeading | Qt::AlignVCenter | Qt::TextSingleLine,
		qtnElidedText(painter, property->name(), nameRect));

	font.setBold(false);
	painter.setFont(font);

	// draw property value
	if (valueRect.isValid())
	{
		vItem.needTooltip = false;

		// draw property value
		vItem.item->delegate->drawValue(
			painter, valueRect, state, &vItem.needTooltip);

		// add action to edit property value
		if (!vItem.actionsValid)
		{
			Action edit;
			edit.rect = editRect;

			auto propertyDelegate = vItem.item->delegate.get();
			edit.action = [propertyDelegate, this](
							  QEvent *e, QRect rect) -> bool {
				bool doEdit = false;

				if (this->propertyViewStyle() &
					QtnPropertyViewStyleDblClickActivation)
				{
					doEdit = (e->type() == QEvent::MouseButtonDblClick);
				} else
				{
					doEdit = (e->type() == QEvent::MouseButtonRelease);
				}

				if (doEdit)
				{
					return startPropertyEdit(propertyDelegate, e, rect);
				}

				return false;
			};

			vItem.actions.append(edit);
		}
	}
}

void QtnPropertyView::changeActivePropertyByIndex(int index)
{
	QtnPropertyBase *newActiveProperty =
		(index < 0) ? nullptr : m_visibleItems[index].item->property;
	setActiveProperty(newActiveProperty);
	ensureVisibleItemByIndex(index);
}

int QtnPropertyView::visibleItemIndexByPoint(QPoint pos) const
{
	int index = (verticalScrollBar()->value() + pos.y()) / m_itemHeight;

	if (index >= m_visibleItems.size())
		return -1;

	return index;
}

int QtnPropertyView::visibleItemIndexByProperty(
	const QtnPropertyBase *property) const
{
	validateVisibleItems();

	for (int i = 0, n = m_visibleItems.size(); i < n; ++i)
	{
		if (m_visibleItems[i].item->property == property)
			return i;
	}

	return -1;
}

QRect QtnPropertyView::visibleItemRect(int index) const
{
	Q_ASSERT(index >= 0 && index < m_visibleItems.size());

	QRect rect = viewport()->rect();
	rect.setTop(index * m_itemHeight - verticalScrollBar()->value());
	rect.setHeight(m_itemHeight);

	return rect;
}

bool QtnPropertyView::processItemActionByMouse(int index, QMouseEvent *e)
{
	if (index < 0)
		return false;

	const VisibleItem &vItem = m_visibleItems[index];

	if (!vItem.actionsValid)
		return false;

	const QList<Action> &actions = vItem.actions;

	for (const Action &action : actions)
	{
		if (action.rect.contains(e->pos()))
		{
			return action.action(e, action.rect);
		}
	}

	return false;
}

void QtnPropertyView::resizeEvent(QResizeEvent *e)
{
	Q_UNUSED(e);
	qtnStopInplaceEdit();
	invalidateVisibleItemsActions();
	updateVScrollbar();
}

static const int TOLERANCE = 3;

void QtnPropertyView::mousePressEvent(QMouseEvent *e)
{
	if (qAbs(e->x() - splitPosition()) < TOLERANCE)
	{
		Q_ASSERT(!m_rubberBand);
		m_rubberBand = new QRubberBand(QRubberBand::Line, this);

		QRect rect = viewport()->rect();
		rect.setLeft(e->x());
		rect.setRight(e->x());
		m_rubberBand->setGeometry(rect);
		m_rubberBand->show();
	} else
	{
		int index = visibleItemIndexByPoint(e->pos());

		if (index >= 0)
		{
			changeActivePropertyByIndex(index);
			processItemActionByMouse(index, e);
		}

		QAbstractScrollArea::mousePressEvent(e);
	}
}

void QtnPropertyView::mouseReleaseEvent(QMouseEvent *e)
{
	if (m_rubberBand)
	{
		delete m_rubberBand;
		m_rubberBand = nullptr;

		// update split ratio
		QRect rect = viewport()->rect();
		updateSplitRatio((float) (e->x() - rect.left()) / (float) rect.width());
	} else
	{
		processItemActionByMouse(visibleItemIndexByPoint(e->pos()), e);

		emit mouseReleased(e);
	}
}

void QtnPropertyView::mouseMoveEvent(QMouseEvent *e)
{
	if (m_rubberBand)
	{
		QRect rect = viewport()->rect();
		rect.setLeft(e->x());
		rect.setRight(e->x());
		m_rubberBand->setGeometry(rect);

		if (m_style & QtnPropertyViewStyleLiveSplit)
		{
			// update split ratio
			QRect rect = viewport()->rect();
			updateSplitRatio(
				(float) (e->x() - rect.left()) / (float) rect.width());
		}
	} else if (qAbs(e->x() - splitPosition()) < TOLERANCE)
	{
		setCursor(Qt::SplitHCursor);
	} else
	{
		setCursor(Qt::ArrowCursor);

		int index = visibleItemIndexByPoint(e->pos());

		if (index >= 0)
		{
			if (e->buttons() & Qt::LeftButton)
				changeActivePropertyByIndex(index);
			else
				processItemActionByMouse(index, e);
		}

		QAbstractScrollArea::mouseMoveEvent(e);
	}
}

void QtnPropertyView::mouseDoubleClickEvent(QMouseEvent *e)
{
	if (!m_rubberBand)
	{
		processItemActionByMouse(visibleItemIndexByPoint(e->pos()), e);
		QAbstractScrollArea::mouseDoubleClickEvent(e);
	}
}

bool QtnPropertyView::viewportEvent(QEvent *e)
{
	switch (e->type())
	{
		case QEvent::StyleChange:
			updateStyleStuff();
			break;

		case QEvent::ToolTip:
		{
			QHelpEvent *helpEvent = static_cast<QHelpEvent *>(e);
			tooltipEvent(helpEvent);
			break;
		}

		default:; // do nothing
	}

	return QAbstractScrollArea::viewportEvent(e);
}

void QtnPropertyView::scrollContentsBy(int dx, int dy)
{
	if (dx != 0 || dy != 0)
	{
		qtnStopInplaceEdit();
		invalidateVisibleItemsActions();
	}

	QAbstractScrollArea::scrollContentsBy(dx, dy);
}

void QtnPropertyView::keyPressEvent(QKeyEvent *e)
{
	validateVisibleItems();

	if (m_visibleItems.empty())
	{
		QAbstractScrollArea::keyPressEvent(e);
		return;
	}

	QWidget *inplaceEditor = qtnGetInplaceEdit();

	if (inplaceEditor)
	{
		int key = e->key();

		if (key == Qt::Key_Escape || key == Qt::Key_Return ||
			key == Qt::Key_Enter)
		{
			qtnStopInplaceEdit();
			// eat event
			e->accept();
		}

		return;
	}

	switch (e->key())
	{
		case Qt::Key_Home:
		{
			// go to first item
			changeActivePropertyByIndex(0);
			break;
		}

		case Qt::Key_End:
		{
			// go to last item
			changeActivePropertyByIndex(m_visibleItems.size() - 1);
			break;
		}

		case Qt::Key_Up:
		{
			// go to previous item
			int index = visibleItemIndexByProperty(activeProperty());

			if (index < 0)
				changeActivePropertyByIndex(0);
			else
				changeActivePropertyByIndex(qMax(0, index - 1));

			break;
		}

		case Qt::Key_Down:
		{
			// go to next item
			int index = visibleItemIndexByProperty(activeProperty());

			if (index < 0)
				changeActivePropertyByIndex(0);
			else
				changeActivePropertyByIndex(
					qMin(m_visibleItems.size() - 1, index + 1));

			break;
		}

		case Qt::Key_PageUp:
		{
			// go to previous page
			int index = visibleItemIndexByProperty(activeProperty());

			if (index < 0)
				changeActivePropertyByIndex(0);
			else
			{
				int itemsPerPage =
					qMax(viewport()->rect().height() / m_itemHeight, 1);
				changeActivePropertyByIndex(qMax(0, index - itemsPerPage));
			}

			break;
		}

		case Qt::Key_PageDown:
		{
			// go to next page
			int index = visibleItemIndexByProperty(activeProperty());

			if (index < 0)
				changeActivePropertyByIndex(0);
			else
			{
				int itemsPerPage =
					qMax(viewport()->rect().height() / m_itemHeight, 1);
				changeActivePropertyByIndex(
					qMin(m_visibleItems.size() - 1, index + itemsPerPage));
			}

			break;
		}

		case Qt::Key_Left:
		{
			// go to parent item or collapse
			int index = visibleItemIndexByProperty(activeProperty());

			if (index < 0)
				changeActivePropertyByIndex(0);
			else
			{
				const VisibleItem &vItem = m_visibleItems[index];

				if (vItem.hasChildren && !vItem.item->collapsed())
				{
					// collapse opened property
					vItem.item->property->addState(QtnPropertyStateCollapsed);
				} else if (vItem.item->parent)
				{
					// activate parent property
					setActiveProperty(vItem.item->parent->property, true);
				}
			}

			break;
		}

		case Qt::Key_Right:
		{
			// go to child item or expand
			int index = visibleItemIndexByProperty(activeProperty());

			if (index < 0)
				changeActivePropertyByIndex(0);
			else
			{
				const VisibleItem &vItem = m_visibleItems[index];

				if (vItem.hasChildren && vItem.item->collapsed())
				{
					// expand closed property
					vItem.item->property->removeState(
						QtnPropertyStateCollapsed);
				} else if (vItem.hasChildren)
				{
					// activate child property
					setActiveProperty(
						vItem.item->children.front()->property, true);
				}
			}

			break;
		}

		default:
		{
			int index = visibleItemIndexByProperty(activeProperty());

			if (index >= 0)
			{
				const auto &delegate = m_visibleItems[index].item->delegate;

				if (delegate && delegate->acceptKeyPressedForInplaceEdit(e))
				{
					QRect valueRect = visibleItemRect(index);
					valueRect.setLeft(splitPosition());

					if (!startPropertyEdit(delegate.get(), e, valueRect))
						return;

					// eat event
					e->accept();
					return;
				}
			}

			// process by default
			QAbstractScrollArea::keyPressEvent(e);
		}
	}
}

static QString qtnGetPropertyTooltip(const QtnPropertyBase *property)
{
	if (!property)
		return QString();

	QString tooltipText = property->description();

	if (tooltipText.isEmpty())
		tooltipText = property->name();

	return tooltipText;
}

void QtnPropertyView::tooltipEvent(QHelpEvent *e)
{
	int index = visibleItemIndexByPoint(e->pos());

	if (index >= 0)
	{
		QRect rect = visibleItemRect(index);
		int splitPos = splitPosition();

		QString tooltipText;

		const VisibleItem &vItem = m_visibleItems[index];

		// propertyset case
		if (!vItem.item->delegate)
		{
			tooltipText = qtnGetPropertyTooltip(vItem.item->property);
		}
		// name sub rect
		else if (e->x() < splitPos)
		{
			rect.setRight(splitPos);
			tooltipText = qtnGetPropertyTooltip(vItem.item->property);
		}
		// value sub rect
		else if (vItem.needTooltip)
		{
			rect.setLeft(splitPos);
			tooltipText = vItem.item->delegate->toolTip();
		}

		QToolTip::showText(e->globalPos(), tooltipText, this, rect);
	} else
	{
		QToolTip::hideText();
	}
}

QtnPropertyView::Item::Item()
	: property(nullptr)
	, level(0)
	, parent(nullptr)
{
}

QtnPropertyView::Item::~Item() {}

bool QtnPropertyView::Item::collapsed() const
{
	return property->isCollapsed();
}

void QtnPropertyView::updateItemsTree()
{
	m_itemsTree.reset(createItemsTree(m_propertySet));
	invalidateVisibleItems();
}

QtnPropertyView::Item *QtnPropertyView::createItemsTree(
	QtnPropertyBase *rootProperty)
{
	if (!rootProperty)
		return nullptr;

	auto item = new Item;
	item->property = rootProperty;
	auto &connections = item->connections;

	connections.push_back(
		QObject::connect(rootProperty, &QtnPropertyBase::propertyDidChange,
			this, &QtnPropertyView::onPropertyDidChange));

	QtnProperty *asProperty = rootProperty->asProperty();

	if (asProperty)
	{
		auto &delegate = item->delegate;
		delegate.reset(m_delegateFactory.createDelegate(*asProperty));

		if (delegate)
		{
			// apply attributes
			auto delegateInfo = asProperty->delegateInfo();

			if (delegateInfo)
			{
				delegate->applyAttributes(delegateInfo->attributes);
			}

			int n = delegate->subPropertyCount();
			item->children.reserve(n);

			// process delegate subproperties
			for (int i = 0; i < n; ++i)
			{
				QtnPropertyBase *child = delegate->subProperty(i);
				Q_ASSERT(child);

				auto childItem = createItemsTree(child);
				childItem->parent = item;
				item->children.emplace_back(childItem);
			}
		}
	} else
	{
		QtnPropertySet *asPropertySet = rootProperty->asPropertySet();

		if (asPropertySet)
		{
			auto &childProperties = asPropertySet->childProperties();
			item->children.reserve(childProperties.size());

			// process property set subproperties
			for (auto child : childProperties)
			{
				auto childItem = createItemsTree(child);
				childItem->parent = item;
				item->children.emplace_back(childItem);
			}
		} else
		{
			// unrecognized PropertyBase class
			Q_UNREACHABLE();
		}
	}

	return item;
}

void QtnPropertyView::setActivePropertyInternal(QtnPropertyBase *property)
{
	disconnectActiveProperty();

	m_activeProperty = property;
	emit activePropertyChanged(m_activeProperty);
	viewport()->update();

	connectActiveProperty();
}

bool QtnPropertyView::startPropertyEdit(
	QtnPropertyDelegate *delegate, QEvent *e, const QRect &rect)
{
	Q_ASSERT(nullptr != delegate);

	QtnInplaceInfo inplaceInfo;
	inplaceInfo.activationEvent = e;

	auto property = delegate->getOwnerProperty();
	Q_ASSERT(nullptr != property);

	auto connections = std::make_shared<QtnConnections>();
	bool editable = property->isEditableByUser();

	if (editable)
	{
		connections->push_back(QObject::connect(property, &QObject::destroyed,
			[connections]() { connections->clear(); }));

		connectPropertyToEdit(property, *connections);
	}

	auto editor = delegate->createValueEditor(viewport(), rect, &inplaceInfo);

	auto editFinished = [connections]() { connections->disconnect(); };

	if (nullptr == editor)
	{
		if (editable)
			editFinished();

		return false;
	}

	QObject::connect(editor, &QObject::destroyed, editFinished);

	if (!editor->isVisible())
		editor->show();

	qtnStartInplaceEdit(editor);
	return true;
}

void QtnPropertyView::invalidateVisibleItems()
{
	m_visibleItemsValid = false;
	update();
}

void QtnPropertyView::validateVisibleItems() const
{
	if (m_visibleItemsValid)
		return;

	m_visibleItems.clear();
	fillVisibleItems(
		m_itemsTree.get(), (m_style & QtnPropertyViewStyleShowRoot) ? 0 : -1);

	updateVScrollbar();

	m_visibleItemsValid = true;
}

void QtnPropertyView::fillVisibleItems(Item *item, int level) const
{
	if (!item)
		return;

	// process children only for negative levels
	if (level < 0)
	{
		// process children
		for (auto &child : item->children)
		{
			fillVisibleItems(child.get(), level + 1);
		}

		return;
	}

	// skip not accepted items
	if (!acceptItem(*item))
		return;

	VisibleItem vItem;
	vItem.item = item;
	vItem.level = level;

	if (item->collapsed())
	{
		// check if item has any child
		for (auto &child : item->children)
		{
			if (acceptItem(*child.get()))
			{
				vItem.hasChildren = true;
				break;
			}
		}

		// add item and quit
		m_visibleItems.append(vItem);
		return;
	}

	// add item
	m_visibleItems.append(vItem);

	// save just added item index
	int index = m_visibleItems.size() - 1;

	// process children
	for (auto &child : item->children)
	{
		fillVisibleItems(child.get(), level + 1);
	}

	// if we add something -> current item has children
	if (index < (m_visibleItems.size() - 1))
		m_visibleItems[index].hasChildren = true;
}

bool QtnPropertyView::acceptItem(const Item &item) const
{
	return item.property->isVisible();
}

void QtnPropertyView::updateVScrollbar() const
{
	int viewportHeight = viewport()->height();
	int virtualHeight = m_itemHeight * m_visibleItems.size();

	verticalScrollBar()->setSingleStep(m_itemHeight);
	verticalScrollBar()->setPageStep(viewportHeight);
	verticalScrollBar()->setRange(
		0, qMax(0, virtualHeight - viewportHeight + 2));
}

void QtnPropertyView::updateStyleStuff()
{
	QFontMetrics fm(font());
	m_itemHeight = fm.height() + m_itemHeightSpacing;

	m_propertySetBackdroundColor = m_linesColor =
		palette().color(QPalette::Button);

	m_leadMargin = style()->pixelMetric(QStyle::PM_CheckBoxLabelSpacing);
}

bool QtnPropertyView::ensureVisibleItemByIndex(int index)
{
	if (index < 0)
		return false;

	int vItemTop = index * m_itemHeight;
	int vItemBottom = vItemTop + m_itemHeight;

	QRect rect = viewport()->rect();
	int scrollPos = verticalScrollBar()->value();

	if (vItemTop < scrollPos)
		scrollPos = vItemTop;
	else if (vItemBottom > scrollPos + rect.height())
		scrollPos = vItemBottom - rect.height();
	else
		return false;

	verticalScrollBar()->setValue(scrollPos);
	return true;
}

void QtnPropertyView::invalidateVisibleItemsActions()
{
	for (auto it = m_visibleItems.begin(); it != m_visibleItems.end(); ++it)
	{
		(*it).actionsValid = false;
		(*it).actions.clear();
	}
}

int QtnPropertyView::splitPosition() const
{
	return (int) ((float) viewport()->rect().width() * m_splitRatio);
}

void QtnPropertyView::updateSplitRatio(float splitRatio)
{
	m_splitRatio = qBound(0.f, splitRatio, 1.f);
	// firce to regenerate actions
	invalidateVisibleItemsActions();
	// repaint
	viewport()->update();
}

void QtnPropertyView::connectActiveProperty()
{
	if (nullptr != m_activeProperty)
	{
		QObject::connect(m_activeProperty, &QObject::destroyed, this,
			&QtnPropertyView::onActivePropertyDestroyed);
	}
}

void QtnPropertyView::disconnectActiveProperty()
{
	if (nullptr != m_activeProperty)
	{
		QObject::disconnect(m_activeProperty, &QObject::destroyed, this,
			&QtnPropertyView::onActivePropertyDestroyed);
	}
}

void QtnPropertyView::onPropertyDidChange(QtnPropertyChangeReason reason)
{
	if (!reason)
		return;

	if (reason & QtnPropertyChangeReasonChildren)
	{
		updateItemsTree();
	} else if (reason & QtnPropertyChangeReasonState)
	{
		invalidateVisibleItems();
	}
	viewport()->update();

	emit propertiesChanged(reason);
}

QtnPropertyView::Item *QtnPropertyView::findItem(
	Item *currentItem, const QtnPropertyBase *property) const
{
	if (nullptr != currentItem && nullptr != property)
	{
		if (property == currentItem->property)
			return currentItem;

		for (auto &item : currentItem->children)
		{
			auto found = findItem(item.get(), property);

			if (nullptr != found)
				return found;
		}
	}

	return nullptr;
}

QtnPropertyView::VisibleItem::VisibleItem()
	: item(nullptr)
	, level(0)
	, hasChildren(false)
	, actionsValid(false)
	, needTooltip(false)
{
}

QtnPropertyViewFilter::~QtnPropertyViewFilter() {}

QtnPropertyViewFilter::QtnPropertyViewFilter() {}

QtnPainterState::QtnPainterState(QPainter &p)
	: m_p(p)
{
	m_p.save();
}

QtnPainterState::~QtnPainterState()
{
	m_p.restore();
}
