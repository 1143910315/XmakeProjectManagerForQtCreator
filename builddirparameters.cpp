// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "builddirparameters.h"

#include "xmakebuildconfiguration.h"
#include "xmakebuildsystem.h"
#include "xmakekitaspect.h"
#include "xmaketoolmanager.h"

#include <projectexplorer/customparser.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <utils/algorithm.h>
#include <utils/aspects.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace XMakeProjectManager::Internal {

BuildDirParameters::BuildDirParameters() = default;

BuildDirParameters::BuildDirParameters(XMakeBuildSystem *buildSystem)
{
    QTC_ASSERT(buildSystem, return);
    auto bc = buildSystem->xmakeBuildConfiguration();
    QTC_ASSERT(bc, return);

    expander = bc->macroExpander();

    const QStringList expandedArguments = Utils::transform(bc->initialXMakeArguments.allValues(),
                                                           [this](const QString &s) {
                                                               return expander->expand(s);
                                                           });
    initialXMakeArguments = Utils::filtered(expandedArguments,
                                            [](const QString &s) { return !s.isEmpty(); });
    configurationChangesArguments = Utils::transform(buildSystem->configurationChangesArguments(),
                                                     [this](const QString &s) {
                                                         return expander->expand(s);
                                                     });
    additionalXMakeArguments = Utils::transform(bc->additionalXMakeArguments(),
                                                [this](const QString &s) {
                                                    return expander->expand(s);
                                                });
    const Target *t = bc->target();
    const Kit *k = t->kit();
    const Project *p = t->project();

    projectName = p->displayName();

    sourceDirectory = bc->sourceDirectory();
    if (sourceDirectory.isEmpty())
        sourceDirectory = p->projectDirectory();
    buildDirectory = bc->buildDirectory();

    xmakeBuildType = buildSystem->xmakeBuildType();

    environment = bc->configureEnvironment();
    // Disable distributed building for configuration runs. XMake does not do those in parallel,
    // so there is no win in sending data over the network.
    // Unfortunately distcc does not have a simple environment flag to turn it off:-/
    if (Utils::HostOsInfo::isAnyUnixHost())
        environment.set("ICECC", "no");

    environment.set("QTC_RUN", "1");
    environment.setFallback("CMAKE_COLOR_DIAGNOSTICS", "1");
    environment.setFallback("CLICOLOR_FORCE", "1");

    xmakeToolId = XMakeKitAspect::xmakeToolId(k);

    outputParserGenerator = [k, bc]() {
        QList<OutputLineParser *> outputParsers = k->createOutputParsers();
        for (const Id id : bc->customParsers()) {
            if (auto parser = createCustomParserFromId(id))
                outputParsers << parser;
        }
        return outputParsers;
    };
}

bool BuildDirParameters::isValid() const
{
    return xmakeTool();
}

XMakeTool *BuildDirParameters::xmakeTool() const
{
    return XMakeToolManager::findById(xmakeToolId);
}

QList<OutputLineParser *> BuildDirParameters::outputParsers() const
{
    QTC_ASSERT(outputParserGenerator, return {});
    return outputParserGenerator();
}

} // XMakeProjectManager::Internal
