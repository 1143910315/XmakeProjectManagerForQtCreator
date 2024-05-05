// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "xmakeprojectmanager.h"

#include "xmakebuildsystem.h"
#include "xmakekitaspect.h"
#include "xmakeprocess.h"
#include "xmakeproject.h"
#include "xmakeprojectconstants.h"
#include "xmakeprojectimporter.h"
#include "xmakeprojectmanagertr.h"
#include "xmakeprojectnodes.h"
#include "xmakespecificsettings.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/modemanager.h>

#include <cppeditor/cpptoolsreuse.h>

#include <debugger/analyzer/analyzerconstants.h>
#include <debugger/analyzer/analyzermanager.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

#include <utils/action.h>
#include <utils/checkablemessagebox.h>
#include <utils/utilsicons.h>

#include <QMessageBox>

using namespace Core;
using namespace CppEditor;
using namespace ProjectExplorer;
using namespace Utils;

namespace XMakeProjectManager::Internal {
    class XMakeManager final : public QObject {
public:
        XMakeManager();

private:
        void updateCmakeActions(Node *node);
        void clearXMakeCache(BuildSystem *buildSystem);
        void runXMake(BuildSystem *buildSystem);
        void runXMakeWithProfiling(BuildSystem *buildSystem);
        void rescanProject(BuildSystem *buildSystem);
        void buildFileContextMenu();
        void buildFile(Node *node = nullptr);
        void updateBuildFileAction();
        void enableBuildFileMenus(Node *node);
        void reloadXMakePresets();

        QAction *m_runXMakeAction;
        QAction *m_clearXMakeCacheAction;
        QAction *m_runXMakeActionContextMenu;
        QAction *m_rescanProjectAction;
        QAction *m_buildFileContextMenu;
        QAction *m_reloadXMakePresetsAction;
        Utils::Action *m_buildFileAction;
        QAction *m_xmakeProfilerAction;
        QAction *m_xmakeDebuggerAction;
        QAction *m_xmakeDebuggerSeparator;
        bool m_canDebugXMake = false;
    };

    XMakeManager::XMakeManager() {
        namespace PEC = ProjectExplorer::Constants;

        const Context projectContext(XMakeProjectManager::Constants::XMAKE_PROJECT_ID);

        ActionBuilder(this, Constants::RUN_XMAKE)
        .setText(Tr::tr("Run XMake"))
        .setIcon(ProjectExplorer::Icons::CMAKE_LOGO.icon())
        .bindContextAction(&m_runXMakeAction)
        .setCommandAttribute(Command::CA_Hide)
        .addToContainer(PEC::M_BUILDPROJECT, PEC::G_BUILD_BUILD)
        .addOnTriggered(this, [this] {
                            runXMake(ProjectManager::startupBuildSystem());
                        });

        ActionBuilder(this, Constants::CLEAR_XMAKE_CACHE)
        .setText(Tr::tr("Clear XMake Configuration"))
        .bindContextAction(&m_clearXMakeCacheAction)
        .setCommandAttribute(Command::CA_Hide)
        .addToContainer(PEC::M_BUILDPROJECT, PEC::G_BUILD_BUILD)
        .addOnTriggered(this, [this] {
                            clearXMakeCache(ProjectManager::startupBuildSystem());
                        });

        ActionBuilder(this, Constants::RUN_XMAKE_CONTEXT_MENU)
        .setText(Tr::tr("Run XMake"))
        .setIcon(ProjectExplorer::Icons::CMAKE_LOGO.icon())
        .setContext(projectContext)
        .bindContextAction(&m_runXMakeActionContextMenu)
        .setCommandAttribute(Command::CA_Hide)
        .addToContainer(PEC::M_PROJECTCONTEXT, PEC::G_PROJECT_BUILD)
        .addOnTriggered(this, [this] {
                            runXMake(ProjectTree::currentBuildSystem());
                        });

        ActionBuilder(this, Constants::BUILD_FILE_CONTEXT_MENU)
        .setText(Tr::tr("Build"))
        .bindContextAction(&m_buildFileContextMenu)
        .setContext(projectContext)
        .setCommandAttribute(Command::CA_Hide)
        .addToContainer(PEC::M_FILECONTEXT, PEC::G_FILE_OTHER)
        .addOnTriggered(this, [this] {
                            buildFileContextMenu();
                        });

        ActionBuilder(this, Constants::RESCAN_PROJECT)
        .setText(Tr::tr("Rescan Project"))
        .bindContextAction(&m_rescanProjectAction)
        .setCommandAttribute(Command::CA_Hide)
        .addToContainer(PEC::M_BUILDPROJECT, PEC::G_BUILD_BUILD)
        .addOnTriggered(this, [this] {
                            rescanProject(ProjectTree::currentBuildSystem());
                        });

        ActionBuilder(this, Constants::RELOAD_XMAKE_PRESETS)
        .setText(Tr::tr("Reload XMake Presets"))
        .setIcon(Utils::Icons::RELOAD.icon())
        .bindContextAction(&m_reloadXMakePresetsAction)
        .setCommandAttribute(Command::CA_Hide)
        .addToContainer(PEC::M_BUILDPROJECT, PEC::G_BUILD_BUILD)
        .addOnTriggered(this, [this] {
                            reloadXMakePresets();
                        });

        ActionBuilder(this, Constants::BUILD_FILE)
        .setParameterText(Tr::tr("Build File \"%1\""), Tr::tr("Build File"),
                          ActionBuilder::AlwaysEnabled)
        .bindContextAction(&m_buildFileAction)
        .setCommandAttribute(Command::CA_Hide)
        .setCommandAttribute(Command::CA_UpdateText)
        .setCommandDescription(m_buildFileAction->text())
        .setDefaultKeySequence(Tr::tr("Ctrl+Alt+B"))
        .addToContainer(PEC::M_BUILDPROJECT, PEC::G_BUILD_BUILD)
        .addOnTriggered(this, [this] {
                            buildFile();
                        });

        // XMake Profiler
        ActionBuilder(this, Constants::RUN_XMAKE_PROFILER)
        .setIcon(ProjectExplorer::Icons::CMAKE_LOGO.icon())
        .setText(Tr::tr("XMake Profiler"))
        .bindContextAction(&m_xmakeProfilerAction)
        .addToContainer(Debugger::Constants::M_DEBUG_ANALYZER,
                        Debugger::Constants::G_ANALYZER_TOOLS,
                        false)
        .addOnTriggered(this, [this] {
                            runXMakeWithProfiling(ProjectManager::startupBuildSystem());
                        });

        // XMake Debugger
        ActionContainer *mdebugger = ActionManager::actionContainer(PEC::M_DEBUG_STARTDEBUGGING);
        mdebugger->appendGroup(Constants::XMAKE_DEBUGGING_GROUP);
        mdebugger->addSeparator(Context(Core::Constants::C_GLOBAL),
                                Constants::XMAKE_DEBUGGING_GROUP,
                                &m_xmakeDebuggerSeparator);

        ActionBuilder(this, Constants::RUN_XMAKE_DEBUGGER)
        .setText(Tr::tr("Start XMake Debugging"))
        .setIcon(ProjectExplorer::Icons::CMAKE_LOGO.icon())
        .bindContextAction(&m_xmakeDebuggerAction)
        .addToContainer(PEC::M_DEBUG_STARTDEBUGGING, Constants::XMAKE_DEBUGGING_GROUP)
        .addOnTriggered(this, [] {
                            ProjectExplorerPlugin::runStartupProject(PEC::DAP_XMAKE_DEBUG_RUN_MODE,
                                                                     /*forceSkipDeploy=*/ true);
                        });

        connect(ProjectManager::instance(), &ProjectManager::startupProjectChanged, this, [this] {
                    auto xmakeBuildSystem = qobject_cast<XMakeBuildSystem *>(
                        ProjectManager::startupBuildSystem());
                    if (xmakeBuildSystem) {
                        const BuildDirParameters parameters(xmakeBuildSystem);
                        const auto tool = parameters.xmakeTool();
                        XMakeTool::Version version = tool ? tool->version() : XMakeTool::Version();
                        m_canDebugXMake = (version.major == 3 && version.minor >= 27) || version.major > 3;
                    }
                    updateCmakeActions(ProjectTree::currentNode());
                });
        connect(BuildManager::instance(), &BuildManager::buildStateChanged, this, [this] {
                    updateCmakeActions(ProjectTree::currentNode());
                });
        connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
                this, &XMakeManager::updateBuildFileAction);
        connect(ProjectTree::instance(), &ProjectTree::currentNodeChanged,
                this, &XMakeManager::updateCmakeActions);

        updateCmakeActions(ProjectTree::currentNode());
    }

    void XMakeManager::updateCmakeActions(Node *node) {
        auto project = qobject_cast<XMakeProject *>(ProjectManager::startupProject());
        const bool visible = project && !BuildManager::isBuilding(project);
        m_runXMakeAction->setVisible(visible);
        m_runXMakeActionContextMenu->setEnabled(visible);
        m_clearXMakeCacheAction->setVisible(visible);
        m_rescanProjectAction->setVisible(visible);
        m_xmakeProfilerAction->setEnabled(visible);

        m_xmakeDebuggerAction->setEnabled(m_canDebugXMake && visible);
        m_xmakeDebuggerSeparator->setVisible(m_canDebugXMake && visible);

        const bool reloadPresetsVisible = [project] {
            if (!project) {
                return false;
            }
            const FilePath presetsPath = project->projectFilePath().parentDir().pathAppended(
                "XMakePresets.json");
            return presetsPath.exists();
        }();
        m_reloadXMakePresetsAction->setVisible(reloadPresetsVisible);

        enableBuildFileMenus(node);
    }

    void XMakeManager::clearXMakeCache(BuildSystem *buildSystem) {
        auto xmakeBuildSystem = dynamic_cast<XMakeBuildSystem *>(buildSystem);
        QTC_ASSERT(xmakeBuildSystem, return );

        xmakeBuildSystem->clearXMakeCache();
    }

    void XMakeManager::runXMake(BuildSystem *buildSystem) {
        auto xmakeBuildSystem = dynamic_cast<XMakeBuildSystem *>(buildSystem);
        QTC_ASSERT(xmakeBuildSystem, return );

        if (ProjectExplorerPlugin::saveModifiedFiles()) {
            xmakeBuildSystem->runXMake();
        }
    }

    void XMakeManager::runXMakeWithProfiling(BuildSystem *buildSystem) {
        auto xmakeBuildSystem = dynamic_cast<XMakeBuildSystem *>(buildSystem);
        QTC_ASSERT(xmakeBuildSystem, return );

        if (ProjectExplorerPlugin::saveModifiedFiles()) {
            // xmakeBuildSystem->runXMakeWithProfiling() below will trigger Target::buildSystemUpdated
            // which will ensure that the "xmake-profile.json" has been created and we can load the viewer
            QObject::connect(xmakeBuildSystem->target(), &Target::buildSystemUpdated, this, [] {
                                 Core::Command *ctfVisualiserLoadTrace = Core::ActionManager::command(
                                     "Analyzer.Menu.StartAnalyzer.CtfVisualizer.LoadTrace");

                                 if (ctfVisualiserLoadTrace) {
                                     auto *action = ctfVisualiserLoadTrace->actionForContext(Core::Constants::C_GLOBAL);
                                     const FilePath file = TemporaryDirectory::masterDirectoryFilePath()
                                         / "xmake-profile.json";
                                     action->setData(file.nativePath());
                                     emit ctfVisualiserLoadTrace->action()->triggered();
                                 }
                             });

            xmakeBuildSystem->runXMakeWithProfiling();
        }
    }

    void XMakeManager::rescanProject(BuildSystem *buildSystem) {
        auto xmakeBuildSystem = dynamic_cast<XMakeBuildSystem *>(buildSystem);
        QTC_ASSERT(xmakeBuildSystem, return );

        xmakeBuildSystem->runXMakeAndScanProjectTree();// by my experience: every rescan run requires xmake run too
    }

    void XMakeManager::updateBuildFileAction() {
        Node *node = nullptr;
        if (Core::IDocument *currentDocument = Core::EditorManager::currentDocument()) {
            node = ProjectTree::nodeForFile(currentDocument->filePath());
        }
        enableBuildFileMenus(node);
    }

    void XMakeManager::enableBuildFileMenus(Node *node) {
        m_buildFileAction->setVisible(false);
        m_buildFileAction->setEnabled(false);
        m_buildFileAction->setParameter(QString());
        m_buildFileContextMenu->setEnabled(false);

        if (!node) {
            return;
        }
        Project *project = ProjectTree::projectForNode(node);
        if (!project) {
            return;
        }
        Target *target = project->activeTarget();
        if (!target) {
            return;
        }
        const QString generator = XMakeGeneratorKitAspect::generator(target->kit());
        if (generator != "Ninja" && !generator.contains("Makefiles")) {
            return;
        }

        if (const FileNode *fileNode = node->asFileNode()) {
            const FileType type = fileNode->fileType();
            const bool visible = qobject_cast<XMakeProject *>(project)
                && dynamic_cast<XMakeTargetNode *>(node->parentProjectNode())
                && (type == FileType::Source || type == FileType::Header);

            const bool enabled = visible && !BuildManager::isBuilding(project);
            m_buildFileAction->setVisible(visible);
            m_buildFileAction->setEnabled(enabled);
            m_buildFileAction->setParameter(node->filePath().fileName());
            m_buildFileContextMenu->setEnabled(enabled);
        }
    }

    void XMakeManager::reloadXMakePresets() {
        QMessageBox::StandardButton clickedButton = CheckableMessageBox::question(
            Core::ICore::dialogParent(),
            Tr::tr("Reload XMake Presets"),
            Tr::tr("Re-generates the kits that were created for XMake presets. All manual "
                   "modifications to the XMake project settings will be lost."),
            settings().askBeforePresetsReload.askAgainCheckableDecider(),
            QMessageBox::Yes | QMessageBox::Cancel,
            QMessageBox::Yes,
            QMessageBox::Yes,
        {
            { QMessageBox::Yes, Tr::tr("Reload") },
        });

        settings().writeSettings();

        if (clickedButton == QMessageBox::Cancel) {
            return;
        }

        XMakeProject *project = static_cast<XMakeProject *>(ProjectTree::currentProject());
        if (!project) {
            return;
        }

        const QSet<QString> oldPresets = Utils::transform<QSet>(project->presetsData().configurePresets,
                                                                [](const auto &preset) {
                                                                    return preset.name;
                                                                });
        project->readPresets();

        QList<Kit *> oldKits;
        for (const auto &target : project->targets()) {
            const XMakeConfigItem presetItem = XMakeConfigurationKitAspect::xmakePresetConfigItem(
                target->kit());

            if (BuildManager::isBuilding(target)) {
                BuildManager::cancel();
            }

            // Only clear the XMake configuration for preset kits. Any manual kit configuration
            // will get the chance to get imported afterwards in the Kit selection wizard
            XMakeBuildSystem *bs = static_cast<XMakeBuildSystem *>(target->buildSystem());
            if (!presetItem.isNull() && bs) {
                bs->clearXMakeCache();
            }

            if (!presetItem.isNull() && oldPresets.contains(QString::fromUtf8(presetItem.value))) {
                oldKits << target->kit();
            }

            project->removeTarget(target);
        }

        project->setOldPresetKits(oldKits);

        emit project->projectImporter()->cmakePresetsUpdated();

        Core::ModeManager::activateMode(ProjectExplorer::Constants::MODE_SESSION);
        Core::ModeManager::setFocusToCurrentMode();
    }

    void XMakeManager::buildFile(Node *node) {
        if (!node) {
            Core::IDocument *currentDocument = Core::EditorManager::currentDocument();
            if (!currentDocument) {
                return;
            }
            const Utils::FilePath file = currentDocument->filePath();
            node = ProjectTree::nodeForFile(file);
        }
        FileNode *fileNode = node ? node->asFileNode() : nullptr;
        if (!fileNode) {
            return;
        }
        Project *project = ProjectTree::projectForNode(fileNode);
        if (!project) {
            return;
        }
        XMakeTargetNode *targetNode = dynamic_cast<XMakeTargetNode *>(fileNode->parentProjectNode());
        if (!targetNode) {
            return;
        }
        FilePath filePath = fileNode->filePath();
        if (filePath.fileName().contains(".h")) {
            bool wasHeader = false;
            const FilePath sourceFile = CppEditor::correspondingHeaderOrSource(filePath, &wasHeader);
            if (wasHeader && !sourceFile.isEmpty()) {
                filePath = sourceFile;
            }
        }
        Target *target = project->activeTarget();
        QTC_ASSERT(target, return );
        const QString generator = XMakeGeneratorKitAspect::generator(target->kit());
        const QString relativeSource = filePath.relativeChildPath(targetNode->filePath()).toString();
        Utils::FilePath targetBase;
        BuildConfiguration *bc = target->activeBuildConfiguration();
        QTC_ASSERT(bc, return );
        if (generator == "Ninja") {
            const Utils::FilePath relativeBuildDir = targetNode->buildDirectory().relativeChildPath(
                bc->buildDirectory());
            targetBase = relativeBuildDir / "XMakeFiles" / (targetNode->displayName() + ".dir");
        } else if (!generator.contains("Makefiles")) {
            Core::MessageManager::writeFlashing(addXMakePrefix(
                Tr::tr("Build File is not supported for generator \"%1\"").arg(generator)));
            return;
        }

        auto cbc = static_cast<XMakeBuildSystem *>(bc->buildSystem());
        const QString sourceFile = targetBase.pathAppended(relativeSource).toString();
        const QString objExtension = [&]() -> QString {
            const auto sourceKind = ProjectFile::classify(relativeSource);
            const QByteArray xmakeLangExtension = ProjectFile::isCxx(sourceKind)
                                                      ? "XMAKE_CXX_OUTPUT_EXTENSION"
                                                      : "XMAKE_C_OUTPUT_EXTENSION";
            const QString extension = cbc->configurationFromXMake().stringValueOf(xmakeLangExtension);
            if (!extension.isEmpty()) {
                return extension;
            }

            const auto toolchain = ProjectFile::isCxx(sourceKind)
                                       ? ToolchainKitAspect::cxxToolchain(target->kit())
                                       : ToolchainKitAspect::cToolchain(target->kit());
            using namespace ProjectExplorer::Constants;
            static QSet<Id> objIds {
                CLANG_CL_TOOLCHAIN_TYPEID,
                MSVC_TOOLCHAIN_TYPEID,
                MINGW_TOOLCHAIN_TYPEID,
            };
            if (objIds.contains(toolchain->typeId())) {
                return ".obj";
            }
            return ".o";
        }();

        cbc->buildXMakeTarget(sourceFile + objExtension);
    }

    void XMakeManager::buildFileContextMenu() {
        if (Node *node = ProjectTree::currentNode()) {
            buildFile(node);
        }
    }

    void setupXMakeManager() {
        static XMakeManager theXMakeManager;
    }
} // XMakeProjectManager::Internal
