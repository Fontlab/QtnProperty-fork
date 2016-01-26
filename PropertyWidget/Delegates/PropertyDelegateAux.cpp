/*
   Copyright (c) 2012-1015 Alex Zhondin <qtinuum.team@gmail.com>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "PropertyDelegateAux.h"
#include "PropertyView.h"
#include <QMouseEvent>

QtnSubItem::QtnSubItem(bool trackState)
    : m_trackState(trackState),
      m_activeCount(0),
      m_state(QtnSubItemStateNone)
{
}

bool QtnSubItem::activate(QtnPropertyView *widget)
{
    if (!m_trackState)
        return false;

    Q_ASSERT(m_activeCount <= 1);
    if (m_activeCount > 1)
        return false;

    ++m_activeCount;

    if (m_state != QtnSubItemStateUnderCursor)
    {
        m_state = QtnSubItemStateUnderCursor;
        widget->viewport()->update();
        selfEvent(EventActivated, widget);
     }

    return true;
}

bool QtnSubItem::deactivate(QtnPropertyView *widget)
{
    if (!m_trackState)
        return false;

    Q_ASSERT(m_activeCount > 0);
    if (m_activeCount == 0)
        return false;

    --m_activeCount;

    if ((m_activeCount == 0) && (m_state != QtnSubItemStateNone))
    {
        m_state = QtnSubItemStateNone;
        widget->viewport()->update();
        selfEvent(EventDeactivated, widget);
    }

    return true;
}

bool QtnSubItem::grabMouse(QtnPropertyView* widget)
{
    Q_ASSERT(m_activeCount > 0);
    Q_ASSERT(m_state == QtnSubItemStateUnderCursor);

    m_state = QtnSubItemStatePushed;
    widget->viewport()->update();
    selfEvent(EventGrabMouse, widget);

    return true;
}

bool QtnSubItem::releaseMouse(QtnPropertyView* widget)
{
    Q_ASSERT(m_activeCount > 0);
    Q_ASSERT(m_state == QtnSubItemStatePushed);

    m_state = QtnSubItemStateUnderCursor;
    widget->viewport()->update();
    selfEvent(EventReleaseMouse, widget);

    return true;
}

void QtnSubItem::draw(QtnDrawContext& context) const
{
    if (drawHandler)
        drawHandler(context, *this);
}

bool QtnSubItem::event(QtnEventContext& context)
{
    if (m_trackState)
    {
        switch (context.eventType())
        {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonDblClick:
        {
            if (context.eventAs<QMouseEvent>()->button() == Qt::LeftButton)
                context.grabMouse(this);
        } break;

        case QEvent::MouseButtonRelease:
        {
            if ((context.eventAs<QMouseEvent>()->button() == Qt::LeftButton)
                    && (m_state == QtnSubItemStatePushed))
                context.releaseMouse(this);
        } break;

        default:
            ;
        }
    }

    if (eventHandler)
        return eventHandler(context, *this);

    return false;
}

bool QtnSubItem::selfEvent(SubItemEventType type, QtnPropertyView* widget)
{
    QEvent event_((QEvent::Type)type);
    QtnEventContext context{&event_, widget};
    return event(context);
}

QStyle* QtnDrawContext::style() const
{
    return widget->style();
}

void QtnDrawContext::initStyleOption(QStyleOption& option) const
{
    option.initFrom(widget);
    // State_MouseOver should be set explicitly
    option.state &= ~QStyle::State_MouseOver;
}

const QPalette& QtnDrawContext::palette() const
{
    return widget->palette();
}

QPalette::ColorGroup QtnDrawContext::colorGroup() const
{
    if (!widget->isEnabled())
        return QPalette::Disabled;
    else if (!widget->hasFocus())
        return QPalette::Inactive;
    else
        return QPalette::Active;
}

bool QtnEventContext::grabMouse(QtnSubItem* subItem)
{
    return widget->grabMouseForSubItem(subItem);
}

bool QtnEventContext::releaseMouse(QtnSubItem* subItem)
{
    return widget->releaseMouseForSubItem(subItem);
}
