// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace XMakeProjectManager::Internal {

class XMakeSpecificSettings final : public Utils::AspectContainer
{
public:
    XMakeSpecificSettings();

    Utils::BoolAspect autorunXMake{this};
    Utils::FilePathAspect ninjaPath{this};
    Utils::BoolAspect packageManagerAutoSetup{this};
    Utils::BoolAspect askBeforeReConfigureInitialParams{this};
    Utils::BoolAspect askBeforePresetsReload{this};
    Utils::BoolAspect showSourceSubFolders{this};
    Utils::BoolAspect showAdvancedOptionsByDefault{this};
    Utils::BoolAspect useJunctionsForSourceAndBuildDirectories{this};
};

XMakeSpecificSettings &settings();

} // XMakeProjectManager::Internal
