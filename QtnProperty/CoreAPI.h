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

#pragma once
#include <qglobal.h>

#if defined(QTN_DYNAMIC_LIBRARY)
#define QTN_IMPORT_EXPORT Q_DECL_EXPORT
#else
#define QTN_IMPORT_EXPORT Q_DECL_IMPORT
#endif

#define returnQByteArrayLiteral(str) \
	enum \
	{ \
		Size = sizeof(str) - 1 \
	}; \
	static const QStaticByteArrayData<Size> qbytearray_literal = { \
		Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER(Size), str \
	}; \
	QByteArrayDataPtr holder = { qbytearray_literal.data_ptr() }; \
	return QByteArray(holder)
