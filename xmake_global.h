// Copyright (C) 2016 Canonical Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#if defined(USE_XMAKE_EXPORT)

#if defined(XMAKEPROJECTMANAGER_LIBRARY)
#  define XMAKE_EXPORT Q_DECL_EXPORT
#elif defined(XMAKEPROJECTMANAGER_STATIC_LIBRARY)
#  define XMAKE_EXPORT
#else
#  define XMAKE_EXPORT Q_DECL_IMPORT
#endif

#else
#define XMAKE_EXPORT
#endif
