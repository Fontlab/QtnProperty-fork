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

#ifndef QTN_PROPERTYVIEW_H
#define QTN_PROPERTYVIEW_H

#include "CoreAPI.h"
#include "Delegates/PropertyDelegateFactory.h"
#include "Utils/AccessibilityProxy.h"
#include "Utils/QtnConnections.h"

#include <QAbstractScrollArea>

#include <memory>

class QRubberBand;
class QHelpEvent;

class QTN_IMPORT_EXPORT QtnPropertyViewFilter
{
	Q_DISABLE_COPY(QtnPropertyViewFilter)

public:
	virtual bool accept(const QtnPropertyBase *property) const = 0;

	virtual ~QtnPropertyViewFilter();

protected:
	QtnPropertyViewFilter();
};

enum QtnPropertyViewStyleFlag
{
	QtnPropertyViewStyleNone = 0x0000,
	QtnPropertyViewStyleShowRoot = 0x0001,
	QtnPropertyViewStyleLiveSplit = 0x0002,
	QtnPropertyViewStyleDblClickActivation = 0x0004
};

Q_DECLARE_FLAGS(QtnPropertyViewStyle, QtnPropertyViewStyleFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(QtnPropertyViewStyle)

class QTN_IMPORT_EXPORT QtnPropertyView : public QAbstractScrollArea
{
	Q_OBJECT
	Q_DISABLE_COPY(QtnPropertyView)

public:
	explicit QtnPropertyView(
		QWidget *parent = nullptr, QtnPropertySet *propertySet = nullptr);
	virtual ~QtnPropertyView() override;

	inline QtnPropertyDelegateFactory *delegateFactory();

	inline const QtnPropertySet *propertySet() const;
	inline QtnPropertySet *propertySet();
	void setPropertySet(QtnPropertySet *newPropertySet);

	QtnPropertyBase *getPropertyParent(const QtnPropertyBase *property) const;
	inline QtnPropertyBase *activeProperty();
	inline const QtnPropertyBase *activeProperty() const;
	bool setActiveProperty(
		QtnPropertyBase *newActiveProperty, bool ensureVisible = false);
	bool setActiveProperty(int index, bool ensureVisible = false);

	bool ensureVisible(const QtnPropertyBase *property);

	inline int itemHeight() const;

	inline quint32 itemHeightSpacing() const;
	bool setItemHeightSpacing(quint32 itemHeightSpacing);

	inline QtnPropertyViewStyle propertyViewStyle() const;
	void setPropertyViewStyle(QtnPropertyViewStyle style);
	void addPropertyViewStyle(QtnPropertyViewStyle style);
	void removePropertyViewStyle(QtnPropertyViewStyle style);

	QtnPropertyBase *getPropertyAt(
		const QPoint &position, QRect *out_rect = nullptr);

	void connectPropertyToEdit(
		QtnPropertyBase *property, QtnConnections &outConnections);

public slots:
	QtnAccessibilityProxy *accessibilityProxy();

Q_SIGNALS:
	void propertiesChanged(QtnPropertyChangeReason reason);
	// emits when active property has changed
	void activePropertyChanged(QtnPropertyBase *activeProperty);
	void mouseReleased(QMouseEvent *e);
	void beforePropertyEdited(
		QtnProperty *property, QtnPropertyValuePtr newValue, int typeId);
	void propertyEdited(QtnProperty *property);

private slots:
	void onActivePropertyDestroyed();
	void onEditedPropertyWillChange(QtnPropertyChangeReason reason,
		QtnPropertyValuePtr newValue, int typeId);
	void onEditedPropertyDidChange(QtnPropertyChangeReason reason);

protected:
	virtual void paintEvent(QPaintEvent *e) override;
	virtual void resizeEvent(QResizeEvent *e) override;
	virtual void mousePressEvent(QMouseEvent *e) override;
	virtual void mouseReleaseEvent(QMouseEvent *e) override;
	virtual void mouseMoveEvent(QMouseEvent *e) override;
	virtual void mouseDoubleClickEvent(QMouseEvent *e) override;
	virtual bool viewportEvent(QEvent *e) override;
	virtual void scrollContentsBy(int dx, int dy) override;
	virtual void keyPressEvent(QKeyEvent *e) override;
	virtual void tooltipEvent(QHelpEvent *e);

private:
	struct Item
	{
		QtnPropertyBase *property;
		std::unique_ptr<QtnPropertyDelegate> delegate;
		int level;

		Item *parent;
		std::vector<std::unique_ptr<Item>> children;
		QtnConnections connections;

		Item();
		~Item();

		bool collapsed() const;
	};

	struct Action
	{
		QRect rect;
		std::function<bool(QEvent *, QRect)> action;
	};

	struct VisibleItem
	{
		Item *item;
		int level;
		bool hasChildren;

		mutable QList<Action> actions;
		mutable bool actionsValid;
		mutable bool needTooltip;

		VisibleItem();
	};

private:
	void updateItemsTree();
	Item *createItemsTree(QtnPropertyBase *rootProperty);

	void setActivePropertyInternal(QtnPropertyBase *property);
	bool startPropertyEdit(
		QtnPropertyDelegate *delegate, QEvent *e, const QRect &rect);

	void invalidateVisibleItems();
	void validateVisibleItems() const;
	void fillVisibleItems(Item *item, int level) const;
	bool acceptItem(const Item &item) const;

	void drawBranchNode(
		QStylePainter &painter, QRect &rect, const VisibleItem &vItem);
	void drawPropertySetItem(
		QStylePainter &painter, const QRect &rect, const VisibleItem &vItem);
	void drawPropertyItem(
		QStylePainter &painter, const QRect &rect, const VisibleItem &vItem);

	void changeActivePropertyByIndex(int index);
	int visibleItemIndexByPoint(QPoint pos) const;
	int visibleItemIndexByProperty(const QtnPropertyBase *property) const;
	QRect visibleItemRect(int index) const;

	bool processItemActionByMouse(int index, QMouseEvent *e);

	void updateVScrollbar() const;
	void updateStyleStuff();

	bool ensureVisibleItemByIndex(int index);
	void invalidateVisibleItemsActions();

	int splitPosition() const;
	void updateSplitRatio(float splitRatio);

	void connectActiveProperty();
	void disconnectActiveProperty();

	void onPropertyDidChange(QtnPropertyChangeReason reason);

private:
	Item *findItem(Item *currentItem, const QtnPropertyBase *property) const;

	QtnPropertySet *m_propertySet;
	QtnPropertyBase *m_activeProperty;

	QtnPropertyDelegateFactory m_delegateFactory;

	std::unique_ptr<Item> m_itemsTree;

	mutable QList<VisibleItem> m_visibleItems;
	mutable bool m_visibleItemsValid;

	QtnPropertyViewStyle m_style;
	int m_itemHeight;
	quint32 m_itemHeightSpacing;
	int m_leadMargin;
	QColor m_linesColor;
	QColor m_propertySetBackdroundColor;

	float m_splitRatio;
	QRubberBand *m_rubberBand;

	friend class QtnAccessibilityProxy;
	QtnAccessibilityProxy *m_accessibilityProxy;
};

QtnPropertyDelegateFactory *QtnPropertyView::delegateFactory()
{
	return &m_delegateFactory;
}

const QtnPropertySet *QtnPropertyView::propertySet() const
{
	return m_propertySet;
}

QtnPropertySet *QtnPropertyView::propertySet()
{
	return m_propertySet;
}

QtnPropertyBase *QtnPropertyView::activeProperty()
{
	return m_activeProperty;
}

const QtnPropertyBase *QtnPropertyView::activeProperty() const
{
	return m_activeProperty;
}

int QtnPropertyView::itemHeight() const
{
	return m_itemHeight;
}

quint32 QtnPropertyView::itemHeightSpacing() const
{
	return m_itemHeightSpacing;
}

QtnPropertyViewStyle QtnPropertyView::propertyViewStyle() const
{
	return m_style;
}

#endif // QTN_PROPERTYVIEW_H
