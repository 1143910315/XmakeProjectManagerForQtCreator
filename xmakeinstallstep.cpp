// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "xmakeinstallstep.h"

#include "xmakeabstractprocessstep.h"
#include "xmakebuildsystem.h"
#include "xmakekitaspect.h"
#include "xmakeparser.h"
#include "xmakeprojectconstants.h"
#include "xmakeprojectmanagertr.h"
#include "xmaketool.h"

#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/layoutbuilder.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace XMakeProjectManager::Internal {

// XMakeInstallStep

class XMakeInstallStep final : public XMakeAbstractProcessStep
{
public:
    XMakeInstallStep(BuildStepList *bsl, Id id)
        : XMakeAbstractProcessStep(bsl, id)
    {
        xmakeArguments.setSettingsKey("XMakeProjectManager.InstallStep.XMakeArguments");
        xmakeArguments.setLabelText(Tr::tr("XMake arguments:"));
        xmakeArguments.setDisplayStyle(StringAspect::LineEditDisplay);

        setCommandLineProvider([this] { return xmakeCommand(); });
    }

private:
    CommandLine xmakeCommand() const;

    void setupOutputFormatter(OutputFormatter *formatter) override;
    QWidget *createConfigWidget() override;

    StringAspect xmakeArguments{this};
};

void XMakeInstallStep::setupOutputFormatter(OutputFormatter *formatter)
{
    XMakeParser *xmakeParser = new XMakeParser;
    xmakeParser->setSourceDirectory(project()->projectDirectory());
    formatter->addLineParsers({xmakeParser});
    formatter->addSearchDir(processParameters()->effectiveWorkingDirectory());
    XMakeAbstractProcessStep::setupOutputFormatter(formatter);
}

CommandLine XMakeInstallStep::xmakeCommand() const
{
    CommandLine cmd;
    if (XMakeTool *tool = XMakeKitAspect::xmakeTool(kit()))
        cmd.setExecutable(tool->xmakeExecutable());

    FilePath buildDirectory = ".";
    if (buildConfiguration())
        buildDirectory = buildConfiguration()->buildDirectory();

    cmd.addArgs({"--install", buildDirectory.path()});

    auto bs = qobject_cast<XMakeBuildSystem *>(buildSystem());
    if (bs && bs->isMultiConfigReader()) {
        cmd.addArg("--config");
        cmd.addArg(bs->xmakeBuildType());
    }

    cmd.addArgs(xmakeArguments(), CommandLine::Raw);

    return cmd;
}

QWidget *XMakeInstallStep::createConfigWidget()
{
    auto updateDetails = [this] {
        ProcessParameters param;
        setupProcessParameters(&param);
        param.setCommandLine(xmakeCommand());

        setSummaryText(param.summary(displayName()));
    };

    setDisplayName(Tr::tr("Install", "ConfigWidget display name."));

    using namespace Layouting;
    auto widget = Form { xmakeArguments, noMargin }.emerge();

    updateDetails();

    connect(&xmakeArguments, &StringAspect::changed, this, updateDetails);

    connect(ProjectExplorerPlugin::instance(),
            &ProjectExplorerPlugin::settingsChanged,
            this,
            updateDetails);
    connect(buildConfiguration(), &BuildConfiguration::buildDirectoryChanged, this, updateDetails);
    connect(buildConfiguration(), &BuildConfiguration::buildTypeChanged, this, updateDetails);

    return widget;
}

// XMakeInstallStepFactory

class XMakeInstallStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    XMakeInstallStepFactory()
    {
        registerStep<XMakeInstallStep>(Constants::CMAKE_INSTALL_STEP_ID);
        setDisplayName(
            Tr::tr("XMake Install", "Display name for XMakeProjectManager::XMakeInstallStep id."));
        setSupportedProjectType(Constants::CMAKE_PROJECT_ID);
        setSupportedStepLists({ProjectExplorer::Constants::BUILDSTEPS_DEPLOY});
    }
};

void setupXMakeInstallStep()
{
    static XMakeInstallStepFactory theXMakeInstallStepFactory;
}

} // XMakeProjectManager::Internal
