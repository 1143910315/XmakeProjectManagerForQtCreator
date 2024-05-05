// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "xmakeproject.h"

#include "xmakekitaspect.h"
#include "xmakeprojectconstants.h"
#include "xmakeprojectimporter.h"
#include "xmakeprojectmanagertr.h"
#include "presetsmacros.h"

#include <coreplugin/icontext.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>

#include <qtsupport/qtkitaspect.h>

#include <utils/mimeconstants.h>

using namespace ProjectExplorer;
using namespace Utils;
using namespace XMakeProjectManager::Internal;

namespace XMakeProjectManager {

/*!
  \class XMakeProject
*/
XMakeProject::XMakeProject(const FilePath &fileName)
    : Project(Utils::Constants::CMAKE_MIMETYPE, fileName)
{
    setId(XMakeProjectManager::Constants::XMAKE_PROJECT_ID);
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
    setDisplayName(projectDirectory().fileName());
    setCanBuildProducts();

    // This only influences whether 'Install into temporary host directory'
    // will show up by default enabled in some remote deploy configurations.
    // We rely on staging via the actual xmake build step.
    setHasMakeInstallEquivalent(false);

    readPresets();
}

XMakeProject::~XMakeProject()
{
    delete m_projectImporter;
}

Tasks XMakeProject::projectIssues(const Kit *k) const
{
    Tasks result = Project::projectIssues(k);

    if (!XMakeKitAspect::xmakeTool(k))
        result.append(createProjectTask(Task::TaskType::Error, Tr::tr("No xmake tool set.")));
    if (ToolchainKitAspect::toolChains(k).isEmpty())
        result.append(createProjectTask(Task::TaskType::Warning, Tr::tr("No compilers set in kit.")));

    result.append(m_issues);

    return result;
}


ProjectImporter *XMakeProject::projectImporter() const
{
    if (!m_projectImporter)
        m_projectImporter = new XMakeProjectImporter(projectFilePath(), this);
    return m_projectImporter;
}

void XMakeProject::addIssue(IssueType type, const QString &text)
{
    m_issues.append(createProjectTask(type, text));
}

void XMakeProject::clearIssues()
{
    m_issues.clear();
}

PresetsData XMakeProject::presetsData() const
{
    return m_presetsData;
}

template<typename T>
static QStringList recursiveInheritsList(const T &presetsHash, const QStringList &inheritsList)
{
    QStringList result;
    for (const QString &inheritFrom : inheritsList) {
        result << inheritFrom;
        if (presetsHash.contains(inheritFrom)) {
            auto item = presetsHash[inheritFrom];
            if (item.inherits)
                result << recursiveInheritsList(presetsHash, item.inherits.value());
        }
    }
    return result;
}

Internal::PresetsData XMakeProject::combinePresets(Internal::PresetsData &xmakePresetsData,
                                                   Internal::PresetsData &xmakeUserPresetsData)
{
    Internal::PresetsData result;
    result.version = xmakePresetsData.version;
    result.xmakeMinimimRequired = xmakePresetsData.xmakeMinimimRequired;

    result.include = xmakePresetsData.include;
    if (result.include) {
        if (xmakeUserPresetsData.include)
            result.include->append(xmakeUserPresetsData.include.value());
    } else {
        result.include = xmakeUserPresetsData.include;
    }

    auto combinePresetsInternal = [](auto &presetsHash,
                                     auto &presets,
                                     auto &userPresets,
                                     const QString &presetType) {
        // Populate the hash map with the XMakePresets
        for (const auto &p : presets)
            presetsHash.insert(p.name, p);

        auto resolveInherits = [](auto &presetsHash, auto &presetsList) {
            Utils::sort(presetsList, [](const auto &left, const auto &right) {
                const bool sameInheritance = left.inherits && right.inherits
                                             && left.inherits.value() == right.inherits.value();
                const bool leftInheritsRight = left.inherits
                                               && left.inherits.value().contains(right.name);

                const bool inheritsGreater = left.inherits && right.inherits
                                             && left.inherits.value().first()
                                                    > right.inherits.value().first();

                const bool noInheritsGreater = !left.inherits && !right.inherits
                                               && left.name > right.name;

                if ((left.inherits && !right.inherits) || leftInheritsRight || sameInheritance
                    || inheritsGreater || noInheritsGreater)
                    return false;
                return true;
            });
            for (auto &p : presetsList) {
                if (!p.inherits)
                    continue;

                const QStringList inheritsList = recursiveInheritsList(presetsHash,
                                                                       p.inherits.value());
                Utils::reverseForeach(inheritsList, [&presetsHash, &p](const QString &inheritFrom) {
                    if (presetsHash.contains(inheritFrom)) {
                        p.inheritFrom(presetsHash[inheritFrom]);
                        presetsHash[p.name] = p;
                    }
                });
            }
        };

        // First resolve the XMakePresets
        resolveInherits(presetsHash, presets);

        // Add the XMakeUserPresets to the resolve hash map
        for (const auto &p : userPresets) {
            if (presetsHash.contains(p.name)) {
                TaskHub::addTask(
                    BuildSystemTask(Task::TaskType::Error,
                                    Tr::tr("XMakeUserPresets.json cannot re-define the %1 preset: %2")
                                        .arg(presetType)
                                        .arg(p.name),
                                    "XMakeUserPresets.json"));
                TaskHub::requestPopup();
            } else {
                presetsHash.insert(p.name, p);
            }
        }

        // Then resolve the XMakeUserPresets
        resolveInherits(presetsHash, userPresets);

        // Get both XMakePresets and XMakeUserPresets into the result
        auto result = presets;

        // std::vector doesn't have append
        std::copy(userPresets.begin(), userPresets.end(), std::back_inserter(result));
        return result;
    };

    QHash<QString, PresetsDetails::ConfigurePreset> configurePresetsHash;
    QHash<QString, PresetsDetails::BuildPreset> buildPresetsHash;

    result.configurePresets = combinePresetsInternal(configurePresetsHash,
                                                     xmakePresetsData.configurePresets,
                                                     xmakeUserPresetsData.configurePresets,
                                                     "configure");
    result.buildPresets = combinePresetsInternal(buildPresetsHash,
                                                 xmakePresetsData.buildPresets,
                                                 xmakeUserPresetsData.buildPresets,
                                                 "build");

    return result;
}

void XMakeProject::setupBuildPresets(Internal::PresetsData &presetsData)
{
    for (auto &buildPreset : presetsData.buildPresets) {
        if (buildPreset.inheritConfigureEnvironment) {
            if (!buildPreset.configurePreset && !buildPreset.hidden) {
                TaskHub::addTask(BuildSystemTask(
                    Task::TaskType::Error,
                    Tr::tr("Build preset %1 is missing a corresponding configure preset.")
                        .arg(buildPreset.name)));
                TaskHub::requestPopup();
            }

            const QString &configurePresetName = buildPreset.configurePreset.value_or(QString());
            buildPreset.environment
                = Utils::findOrDefault(presetsData.configurePresets,
                                       [configurePresetName](
                                           const PresetsDetails::ConfigurePreset &configurePreset) {
                                           return configurePresetName == configurePreset.name;
                                       })
                      .environment;
        }
    }
}

void XMakeProject::readPresets()
{
    auto parsePreset = [](const Utils::FilePath &presetFile) -> Internal::PresetsData {
        Internal::PresetsData data;
        Internal::PresetsParser parser;

        QString errorMessage;
        int errorLine = -1;

        if (presetFile.exists()) {
            if (parser.parse(presetFile, errorMessage, errorLine)) {
                data = parser.presetsData();
            } else {
                TaskHub::addTask(BuildSystemTask(Task::TaskType::Error,
                                                 Tr::tr("Failed to load %1: %2")
                                                     .arg(presetFile.fileName())
                                                     .arg(errorMessage),
                                                 presetFile,
                                                 errorLine));
                TaskHub::requestPopup();
            }
        }
        return data;
    };

    std::function<void(Internal::PresetsData & presetData, Utils::FilePaths & inclueStack)>
        resolveIncludes = [&](Internal::PresetsData &presetData, Utils::FilePaths &includeStack) {
            if (presetData.include) {
                for (const QString &path : presetData.include.value()) {
                    Utils::FilePath includePath = Utils::FilePath::fromUserInput(path);
                    if (!includePath.isAbsolutePath())
                        includePath = presetData.fileDir.resolvePath(path);

                    Internal::PresetsData includeData = parsePreset(includePath);
                    if (includeData.include) {
                        if (includeStack.contains(includePath)) {
                            TaskHub::addTask(BuildSystemTask(
                                Task::TaskType::Warning,
                                Tr::tr("Attempt to include \"%1\" which was already parsed.")
                                    .arg(includePath.path()),
                                Utils::FilePath(),
                                -1));
                            TaskHub::requestPopup();
                        } else {
                            resolveIncludes(includeData, includeStack);
                        }
                    }

                    presetData.configurePresets = includeData.configurePresets
                                                  + presetData.configurePresets;
                    presetData.buildPresets = includeData.buildPresets + presetData.buildPresets;

                    includeStack << includePath;
                }
            }
        };

    const Utils::FilePath xmakePresetsJson = projectDirectory().pathAppended("XMakePresets.json");
    const Utils::FilePath xmakeUserPresetsJson = projectDirectory().pathAppended("XMakeUserPresets.json");

    Internal::PresetsData xmakePresetsData = parsePreset(xmakePresetsJson);
    Internal::PresetsData xmakeUserPresetsData = parsePreset(xmakeUserPresetsJson);

    // resolve the include
    Utils::FilePaths includeStack = {xmakePresetsJson};
    resolveIncludes(xmakePresetsData, includeStack);

    includeStack = {xmakeUserPresetsJson};
    resolveIncludes(xmakeUserPresetsData, includeStack);

    m_presetsData = combinePresets(xmakePresetsData, xmakeUserPresetsData);
    setupBuildPresets(m_presetsData);

    for (const auto &configPreset : m_presetsData.configurePresets) {
        if (configPreset.hidden.value())
            continue;

        if (configPreset.condition) {
            if (!XMakePresets::Macros::evaluatePresetCondition(configPreset, projectFilePath()))
                continue;
        }
        m_presetsData.havePresets = true;
        break;
    }
}

bool XMakeProject::setupTarget(Target *t)
{
    t->updateDefaultBuildConfigurations();
    if (t->buildConfigurations().isEmpty())
        return false;
    t->updateDefaultDeployConfigurations();
    return true;
}

ProjectExplorer::DeploymentKnowledge XMakeProject::deploymentKnowledge() const
{
    return !files([](const ProjectExplorer::Node *n) {
                return n->filePath().fileName() == "QtCreatorDeployment.txt";
            })
                   .isEmpty()
               ? DeploymentKnowledge::Approximative
               : DeploymentKnowledge::Bad;
}

void XMakeProject::configureAsExampleProject(ProjectExplorer::Kit *kit)
{
    QList<BuildInfo> infoList;
    const QList<Kit *> kits(kit != nullptr ? QList<Kit *>({kit}) : KitManager::kits());
    for (Kit *k : kits) {
        if (QtSupport::QtKitAspect::qtVersion(k) != nullptr) {
            if (auto factory = BuildConfigurationFactory::find(k, projectFilePath()))
                infoList << factory->allAvailableSetups(k, projectFilePath());
        }
    }
    setup(infoList);
}

void XMakeProjectManager::XMakeProject::setOldPresetKits(
    const QList<ProjectExplorer::Kit *> &presetKits) const
{
    m_oldPresetKits = presetKits;
}

QList<Kit *> XMakeProject::oldPresetKits() const
{
    return m_oldPresetKits;
}

} // namespace XMakeProjectManager
