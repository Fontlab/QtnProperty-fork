/*******************************************************************************
Copyright (c) 2025

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

#include "QtnSpinBox.h"
#include <QKeyEvent>

QtnSpinBox::QtnSpinBox(QWidget *parent)
	: QSpinBox(parent)
{
}

void QtnSpinBox::keyPressEvent(QKeyEvent *event)
{
	if (event)
	{
		int key = event->key();
		if (key == Qt::Key_Up || key == Qt::Key_Down)
		{
			int direction = (key == Qt::Key_Up) ? 1 : -1;
			Qt::KeyboardModifiers mods = event->modifiers();
			int step = 1;
			if (mods & (Qt::ControlModifier | Qt::MetaModifier))
				step = 100;
			else if (mods & Qt::ShiftModifier)
				step = 10;
			else if (mods & Qt::AltModifier)
				step = 1; // Alt has no fractional for ints; keep 1

			int newVal = qBound(minimum(), value() + direction * step, maximum());
			setValue(newVal);

			event->accept();
			return;
		}
	}
	QSpinBox::keyPressEvent(event);
}


