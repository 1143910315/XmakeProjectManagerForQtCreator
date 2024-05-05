// Copyright (C) 2016 Lorenz Haas
// Copyright (C) 2022 Xavier BESSON
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "xmakeformatter.h"

#include "xmakeprojectconstants.h"
#include "xmakeprojectmanagertr.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/idocument.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>

#include <texteditor/command.h>
#include <texteditor/formattexteditor.h>
#include <texteditor/texteditor.h>

#include <utils/algorithm.h>
#include <utils/genericconstants.h>
#include <utils/layoutbuilder.h>
#include <utils/mimeconstants.h>
#include <utils/mimeutils.h>

#include <QMenu>

using namespace Core;
using namespace TextEditor;
using namespace Utils;

namespace XMakeProjectManager::Internal {
    class XMakeFormatterSettings : public AspectContainer {
public:
        XMakeFormatterSettings() {
            setAutoApply(false);
            setSettingsGroups(Constants::XMAKEFORMATTER_SETTINGS_GROUP,
                              Constants::XMAKEFORMATTER_GENERAL_GROUP);

            command.setSettingsKey("autoFormatCommand");
            command.setDefaultValue("xmake-format");
            command.setExpectedKind(PathChooser::ExistingCommand);

            autoFormatOnSave.setSettingsKey("autoFormatOnSave");
            autoFormatOnSave.setLabelText(Tr::tr("Enable auto format on file save"));

            autoFormatOnlyCurrentProject.setSettingsKey("autoFormatOnlyCurrentProject");
            autoFormatOnlyCurrentProject.setDefaultValue(true);
            autoFormatOnlyCurrentProject.setLabelText(Tr::tr("Restrict to files contained in the current project"));
            autoFormatOnlyCurrentProject.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBox);

            autoFormatMime.setSettingsKey("autoFormatMime");
            autoFormatMime.setDefaultValue(Utils::Constants::CMAKE_MIMETYPE);
            autoFormatMime.setLabelText(Tr::tr("Restrict to MIME types:"));
            autoFormatMime.setDisplayStyle(StringAspect::LineEditDisplay);

            setLayouter([this] {
                            using namespace Layouting;

                            auto xmakeFormatter = new QLabel(
                                Tr::tr("<a href=\"%1\">XMakeFormat</a> command:")
                                .arg("qthelp://org.qt-project.qtcreator/doc/"
                                     "creator-project-xmake.html#formatting-xmake-files"));
                            xmakeFormatter->setOpenExternalLinks(true);

                            return Column {
                                Row { xmakeFormatter, command },
                                Space(10),
                                Group {
                                    title(Tr::tr("Automatic Formatting on File Save")),
                                    autoFormatOnSave.groupChecker(),
                                    // Conceptually, that's a Form, but this would look odd:
                                    // xxxxxx [____]
                                    // [x] xxxxxxxxxxxxxx
                                    Column {
                                        Row { autoFormatMime },
                                        autoFormatOnlyCurrentProject
                                    }
                                },
                                st
                            };
                        });

            MenuBuilder(Constants::XMAKEFORMATTER_MENU_ID)
            .setTitle(Tr::tr("XMakeFormatter"))
            .setIcon(ProjectExplorer::Icons::CMAKE_LOGO.icon())
            .setOnAllDisabledBehavior(ActionContainer::Show)
            .addToContainer(Core::Constants::M_TOOLS);

            Core::Command *cmd = ActionManager::registerAction(&formatFile, Constants::XMAKEFORMATTER_ACTION_ID);
            connect(&formatFile, &QAction::triggered, this, [this] {
                        auto command = formatCommand();
                        if (auto editor = EditorManager::currentEditor()) {
                            extendCommandWithConfigs(command, editor->document()->filePath());
                        }

                        TextEditor::formatCurrentFile(command);
                    });

            ActionManager::actionContainer(Constants::XMAKEFORMATTER_MENU_ID)->addAction(cmd);

            auto updateActions = [this] {
                auto editor = EditorManager::currentEditor();

                formatFile.setEnabled(haveValidFormatCommand && editor
                                      && isApplicable(editor->document()));
            };

            connect(&autoFormatMime, &Utils::StringAspect::changed,
                    this, updateActions);
            connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
                    this, updateActions);
            connect(EditorManager::instance(), &EditorManager::aboutToSave,
                    this, &XMakeFormatterSettings::applyIfNecessary);

            readSettings();

            const FilePath commandPath = command().searchInPath();
            haveValidFormatCommand = commandPath.exists() && commandPath.isExecutableFile();

            formatFile.setEnabled(haveValidFormatCommand);
            connect(&command, &FilePathAspect::validChanged, this, [this](bool validState) {
                        haveValidFormatCommand = validState;
                        formatFile.setEnabled(haveValidFormatCommand);
                    });
        }

        bool isApplicable(const IDocument *document) const;

        void applyIfNecessary(IDocument *document) const;

        TextEditor::Command formatCommand() const {
            TextEditor::Command cmd;
            cmd.setExecutable(command());
            cmd.setProcessing(TextEditor::Command::FileProcessing);
            cmd.addOption("--in-place");
            cmd.addOption("%file");
            return cmd;
        }

        static FilePaths formatConfigFiles(const FilePath &dir) {
            if (dir.isEmpty()) {
                return FilePaths();
            }

            return filtered(transform({ ".xmake-format",
                                        ".xmake-format.py",
                                        ".xmake-format.json",
                                        ".xmake-format.yaml",
                                        "xmake-format.py",
                                        "xmake-format.json",
                                        "xmake-format.yaml" },
                                      [dir](const QString &fileName) {
                                          return dir.pathAppended(fileName);
                                      }),
                            &FilePath::exists);
        }

        static FilePaths findConfigs(const FilePath &fileName) {
            FilePath parentDirectory = fileName.parentDir();
            while (parentDirectory.exists()) {
                FilePaths configFiles = formatConfigFiles(parentDirectory);
                if (!configFiles.isEmpty()) {
                    return configFiles;
                }

                parentDirectory = parentDirectory.parentDir();
            }
            return FilePaths();
        }

        static void extendCommandWithConfigs(TextEditor::Command &command, const FilePath &source) {
            const FilePaths configFiles = findConfigs(source);
            if (!configFiles.isEmpty()) {
                command.addOption("--config-files");
                command.addOptions(Utils::transform(configFiles, &FilePath::nativePath));
            }
        }

        FilePathAspect command { this };
        bool haveValidFormatCommand { false };
        BoolAspect autoFormatOnSave { this };
        BoolAspect autoFormatOnlyCurrentProject { this };
        StringAspect autoFormatMime { this };

        QAction formatFile { Tr::tr("Format &Current File") };
    };

    bool XMakeFormatterSettings::isApplicable(const IDocument *document) const {
        if (!document) {
            return false;
        }

        if (autoFormatMime().isEmpty()) {
            return true;
        }

        const QStringList allowedMimeTypes = autoFormatMime().split(';');
        const MimeType documentMimeType = Utils::mimeTypeForName(document->mimeType());

        return anyOf(allowedMimeTypes, [&documentMimeType](const QString &mime) {
                         return documentMimeType.inherits(mime);
                     });
    }

    void XMakeFormatterSettings::applyIfNecessary(IDocument *document) const {
        if (!autoFormatOnSave()) {
            return;
        }

        if (!document) {
            return;
        }

        if (!isApplicable(document)) {
            return;
        }

        // Check if file is contained in the current project (if wished)
        if (autoFormatOnlyCurrentProject()) {
            const ProjectExplorer::Project *pro = ProjectExplorer::ProjectTree::currentProject();
            if (!pro || pro->files([document](const ProjectExplorer::Node *n) {
                                       return ProjectExplorer::Project::SourceFiles(n)
                                              && n->filePath() == document->filePath();
                                   }).isEmpty()) {
                return;
            }
        }

        TextEditor::Command command = formatCommand();
        if (!command.isValid()) {
            return;
        }

        const QList<IEditor *> editors = DocumentModel::editorsForDocument(document);
        if (editors.isEmpty()) {
            return;
        }

        IEditor *currentEditor = EditorManager::currentEditor();
        IEditor *editor = editors.contains(currentEditor) ? currentEditor : editors.first();
        if (auto widget = TextEditorWidget::fromEditor(editor)) {
            extendCommandWithConfigs(command, editor->document()->filePath());
            TextEditor::formatEditor(widget, command);
        }
    }

    XMakeFormatterSettings &formatterSettings() {
        static XMakeFormatterSettings theSettings;
        return theSettings;
    }

    class XMakeFormatterSettingsPage final : public Core::IOptionsPage {
public:
        XMakeFormatterSettingsPage() {
            setId(Constants::Settings::FORMATTER_ID);
            setDisplayName(Tr::tr("Formatter"));
            setDisplayCategory("XMake");
            setCategory(Constants::Settings::CATEGORY);
            setSettingsProvider([] {
                                    return &formatterSettings();
                                });
        }
    };

    void setupXMakeFormatter() {
        static const XMakeFormatterSettingsPage theXMakeFormatterSettingsPage;

        formatterSettings();
    };
} // XMakeProjectManager::Internal
