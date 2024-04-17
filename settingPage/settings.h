// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace XmakeProjectManagerForQtCreator::Internal {
    class XmakeSettings final : public Utils::AspectContainer {
public:
        XmakeSettings();

        Utils::StringAspect xmakePath { this };
    };

    XmakeSettings &settings();
} // XmakeProjectManagerForQtCreator::Internal
