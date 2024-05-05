// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "xmakeabstractprocessstep.h"

#include "xmakekitaspect.h"
#include "xmakeprojectconstants.h"
#include "xmakeprojectmanagertr.h"
#include "xmaketool.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace XMakeProjectManager::Internal {

// XMakeAbstractProcessStep

XMakeAbstractProcessStep::XMakeAbstractProcessStep(BuildStepList *bsl, Utils::Id id)
    : AbstractProcessStep(bsl, id)
{}

bool XMakeAbstractProcessStep::init()
{
    if (!AbstractProcessStep::init())
        return false;

    BuildConfiguration *bc = buildConfiguration();
    QTC_ASSERT(bc, return false);

    if (!bc->isEnabled()) {
        emit addTask(
            BuildSystemTask(Task::Error, Tr::tr("The build configuration is currently disabled.")));
        emitFaultyConfigurationMessage();
        return false;
    }

    XMakeTool *tool = XMakeKitAspect::xmakeTool(kit());
    if (!tool || !tool->isValid()) {
        emit addTask(BuildSystemTask(Task::Error,
                                     Tr::tr("A XMake tool must be set up for building. "
                                            "Configure a XMake tool in the kit options.")));
        emitFaultyConfigurationMessage();
        return false;
    }

    // Warn if doing out-of-source builds with a XMakeCache.txt is the source directory
    const Utils::FilePath projectDirectory = bc->target()->project()->projectDirectory();
    if (bc->buildDirectory() != projectDirectory) {
        if (projectDirectory.pathAppended(Constants::XMAKE_CACHE_TXT).exists()) {
            emit addTask(BuildSystemTask(
                Task::Warning,
                Tr::tr("There is a XMakeCache.txt file in \"%1\", which suggest an "
                       "in-source build was done before. You are now building in \"%2\", "
                       "and the XMakeCache.txt file might confuse XMake.")
                    .arg(projectDirectory.toUserOutput(), bc->buildDirectory().toUserOutput())));
        }
    }

    return true;
}

} // namespace XMakeProjectManager::Internal
