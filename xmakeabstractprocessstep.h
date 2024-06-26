// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/abstractprocessstep.h>

namespace XMakeProjectManager::Internal {
    class XMakeAbstractProcessStep : public ProjectExplorer::AbstractProcessStep {
public:
        XMakeAbstractProcessStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id);

protected:
        bool init() override;
    };
} // namespace XMakeProjectManager::Internal
