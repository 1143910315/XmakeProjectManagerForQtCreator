// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "xmakebuildconfiguration.h"
#include "xmakebuildstep.h"
#include "xmakebuildsystem.h"
#include "xmakeeditor.h"
#include "xmakeformatter.h"
#include "xmakeinstallstep.h"
#include "xmakelocatorfilter.h"
#include "xmakekitaspect.h"
#include "xmakeparser.h"
#include "xmakeproject.h"
#include "xmakeprojectconstants.h"
#include "xmakeprojectimporter.h"
#include "xmakeprojectmanager.h"
#include "xmakeprojectmanagertr.h"
#include "xmakeprojectnodes.h"
#include "xmakesettingspage.h"
#include "xmaketoolmanager.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>

#include <extensionsystem/iplugin.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>

#include <texteditor/formattexteditor.h>
#include <texteditor/snippets/snippetprovider.h>

#include <utils/action.h>
#include <utils/mimeconstants.h>
#include <utils/fsengine/fileiconprovider.h>

#include <QTimer>
#include <QMenu>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace XMakeProjectManager::Internal {
    class XMakeProjectPlugin final : public ExtensionSystem::IPlugin {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "XmakeProjectManagerForQtCreator.json")

        void initialize() final {
            setupXMakeToolManager(this);

            setupXMakeSettingsPage();
            setupXMakeKitAspects();

            setupXMakeBuildConfiguration();
            setupXMakeBuildStep();
            setupXMakeInstallStep();

            setupXMakeEditor();

            setupXMakeLocatorFilters();
            setupXMakeFormatter();

            setupXMakeManager();

#ifdef WITH_TESTS
            addTestCreator(createXMakeConfigTest);
            addTestCreator(createXMakeParserTest);
            addTestCreator(createXMakeProjectImporterTest);
#endif

            FileIconProvider::registerIconOverlayForSuffix(Constants::Icons::FILE_OVERLAY, "xmake");
            FileIconProvider::registerIconOverlayForFilename(Constants::Icons::FILE_OVERLAY,
                                                             Constants::XMAKE_LISTS_TXT);

            TextEditor::SnippetProvider::registerGroup(Constants::XMAKE_SNIPPETS_GROUP_ID,
                                                       Tr::tr("XMake", "SnippetProvider"));
            ProjectManager::registerProjectType<XMakeProject>(Utils::Constants::XMAKE_PROJECT_MIMETYPE);

            ActionBuilder(this, Constants::BUILD_TARGET_CONTEXT_MENU)
            .setParameterText(Tr::tr("Build \"%1\""), Tr::tr("Build"), ActionBuilder::AlwaysEnabled)
            .setContext(XMakeProjectManager::Constants::XMAKE_PROJECT_ID)
            .bindContextAction(&m_buildTargetContextAction)
            .setCommandAttribute(Command::CA_Hide)
            .setCommandAttribute(Command::CA_UpdateText)
            .setCommandDescription(m_buildTargetContextAction->text())
            .addToContainer(ProjectExplorer::Constants::M_SUBPROJECTCONTEXT,
                            ProjectExplorer::Constants::G_PROJECT_BUILD)
            .addOnTriggered(this, [] {
                                if (auto bs = qobject_cast<XMakeBuildSystem *>(ProjectTree::currentBuildSystem())) {
                                    auto targetNode = dynamic_cast<const XMakeTargetNode *>(ProjectTree::currentNode());
                                    bs->buildXMakeTarget(targetNode ? targetNode->displayName() : QString());
                                }
                            });

            connect(ProjectTree::instance(), &ProjectTree::currentNodeChanged,
                    this, &XMakeProjectPlugin::updateContextActions);
        }

        void extensionsInitialized() final {
            // Delay the restoration to allow the devices to load first.
            QTimer::singleShot(0, this, [] {
                                   XMakeToolManager::restoreXMakeTools();
                               });
        }

        void updateContextActions(ProjectExplorer::Node *node) {
            auto targetNode = dynamic_cast<const XMakeTargetNode *>(node);
            const QString targetDisplayName = targetNode ? targetNode->displayName() : QString();

            // Build Target:
            m_buildTargetContextAction->setParameter(targetDisplayName);
            m_buildTargetContextAction->setEnabled(targetNode);
            m_buildTargetContextAction->setVisible(targetNode);
        }

        Action *m_buildTargetContextAction = nullptr;
    };
} // XMakeProjectManager::Internal

#include "xmakeprojectplugin.moc"
