#pragma once

#include <qglobal.h>

#if defined(PLUGIN_LIBRARY)
#define PLUGIN_EXPORT Q_DECL_EXPORT
#else
#define PLUGIN_EXPORT Q_DECL_IMPORT
#endif
