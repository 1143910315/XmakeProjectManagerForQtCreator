// Copyright (C) 2016 Kläralvdalens Datakonsult AB, a KDAB Group company.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "xmakelocatorfilter.h"

#include "xmakebuildstep.h"
#include "xmakebuildsystem.h"
#include "xmakeproject.h"
#include "xmakeprojectmanagertr.h"

#include <coreplugin/locator/ilocatorfilter.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <utils/algorithm.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace XMakeProjectManager::Internal {

using BuildAcceptor = std::function<void(const FilePath &, const QString &)>;

// XMakeBuildTargetFilter

static LocatorMatcherTasks xmakeMatchers(const BuildAcceptor &acceptor)
{
    using namespace Tasking;

    Storage<LocatorStorage> storage;

    const auto onSetup = [storage, acceptor] {
        const QString input = storage->input();
        const QList<Project *> projects = ProjectManager::projects();
        LocatorFilterEntries entries;
        for (Project *project : projects) {
            const auto xmakeProject = qobject_cast<const XMakeProject *>(project);
            if (!xmakeProject || !xmakeProject->activeTarget())
                continue;
            const auto bs = qobject_cast<XMakeBuildSystem *>(
                xmakeProject->activeTarget()->buildSystem());
            if (!bs)
                continue;

            const QList<XMakeBuildTarget> buildTargets = bs->buildTargets();
            for (const XMakeBuildTarget &target : buildTargets) {
                if (XMakeBuildSystem::filteredOutTarget(target))
                    continue;
                const int index = target.title.indexOf(input, 0, Qt::CaseInsensitive);
                if (index >= 0) {
                    const FilePath projectPath = xmakeProject->projectFilePath();
                    const QString displayName = target.title;
                    LocatorFilterEntry entry;
                    entry.displayName = displayName;
                    if (acceptor) {
                        entry.acceptor = [projectPath, displayName, acceptor] {
                            acceptor(projectPath, displayName);
                            return AcceptResult();
                        };
                    }
                    bool realTarget = false;
                    if (!target.backtrace.isEmpty() && target.targetType != UtilityType) {
                        const FilePath path = target.backtrace.last().path;
                        const int line = target.backtrace.last().line;
                        entry.linkForEditor = {path, line};
                        entry.extraInfo = path.shortNativePath();
                        realTarget = true;
                    } else {
                        entry.extraInfo = projectPath.shortNativePath();
                    }
                    entry.highlightInfo = {index, int(input.length())};
                    entry.filePath = xmakeProject->projectFilePath();
                    if (acceptor || realTarget)
                        entries.append(entry);
                }
            }
        }
        storage->reportOutput(entries);
    };
    return {{Sync(onSetup), storage}};
}

static void setupFilter(ILocatorFilter *filter)
{
    const auto projectListUpdated = [filter] {
        filter->setEnabled(Utils::contains(ProjectManager::projects(),
                           [](Project *p) { return qobject_cast<XMakeProject *>(p); }));
    };
    QObject::connect(ProjectManager::instance(), &ProjectManager::projectAdded,
                     filter, projectListUpdated);
    QObject::connect(ProjectManager::instance(), &ProjectManager::projectRemoved,
                     filter, projectListUpdated);
}

static void buildAcceptor(const FilePath &projectPath, const QString &displayName)
{
    // Get the project containing the target selected
    const auto xmakeProject = qobject_cast<XMakeProject *>(
        Utils::findOrDefault(ProjectManager::projects(), [projectPath](Project *p) {
            return p->projectFilePath() == projectPath;
        }));
    if (!xmakeProject || !xmakeProject->activeTarget()
        || !xmakeProject->activeTarget()->activeBuildConfiguration())
        return;

    if (BuildManager::isBuilding(xmakeProject))
        BuildManager::cancel();

    // Find the make step
    const BuildStepList *buildStepList =
        xmakeProject->activeTarget()->activeBuildConfiguration()->buildSteps();
    const auto buildStep = buildStepList->firstOfType<XMakeBuildStep>();
    if (!buildStep)
        return;

    // Change the make step to build only the given target
    const QStringList oldTargets = buildStep->buildTargets();
    buildStep->setBuildTargets({displayName});

    // Build
    BuildManager::buildProjectWithDependencies(xmakeProject);
    buildStep->setBuildTargets(oldTargets);
}

class XMakeBuildTargetFilter final : ILocatorFilter
{
public:
    XMakeBuildTargetFilter()
    {
        setId("Build XMake target");
        setDisplayName(Tr::tr("Build XMake Target"));
        setDescription(Tr::tr("Builds a target of any open XMake project."));
        setDefaultShortcutString("cm");
        setPriority(High);
        setupFilter(this);
    }

private:
    LocatorMatcherTasks matchers() final { return xmakeMatchers(&buildAcceptor); }
};

// OpenXMakeTargetLocatorFilter

class XMakeOpenTargetFilter final : ILocatorFilter
{
public:
    XMakeOpenTargetFilter()
    {
        setId("Open XMake target definition");
        setDisplayName(Tr::tr("Open XMake Target"));
        setDescription(Tr::tr("Locates the definition of a target of any open XMake project."));
        setDefaultShortcutString("cmo");
        setPriority(Medium);
        setupFilter(this);
    }

private:
    LocatorMatcherTasks matchers() final { return xmakeMatchers({}); }
};

// Setup

void setupXMakeLocatorFilters()
{
    static XMakeBuildTargetFilter theXMakeBuildTargetFilter;
    static XMakeOpenTargetFilter theXMakeOpenTargetFilter;
}

} // XMakeProjectManager::Internal
