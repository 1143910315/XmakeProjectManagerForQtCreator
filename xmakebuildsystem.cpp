// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "xmakebuildsystem.h"

#include "builddirparameters.h"
#include "xmakebuildconfiguration.h"
#include "xmakebuildstep.h"
#include "xmakebuildtarget.h"
#include "xmakekitaspect.h"
#include "xmakeprocess.h"
#include "xmakeproject.h"
#include "xmakeprojectconstants.h"
#include "xmakeprojectmanagertr.h"
#include "xmakespecificsettings.h"
#include "xmaketoolmanager.h"
#include "projecttreehelper.h"

#include <android/androidconstants.h>

#include <coreplugin/icore.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/extracompiler.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectupdater.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>

#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <qtapplicationmanager/appmanagerconstants.h>

#include <qtsupport/qtcppkitinfo.h>
#include <qtsupport/qtsupportconstants.h>

#include <utils/algorithm.h>
#include <utils/checkablemessagebox.h>
#include <utils/macroexpander.h>
#include <utils/mimeconstants.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

#include <QClipboard>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>

using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

namespace XMakeProjectManager::Internal {
    static Q_LOGGING_CATEGORY(xmakeBuildSystemLog, "qtc.xmake.buildsystem", QtWarningMsg);

// --------------------------------------------------------------------
// XMakeBuildSystem:
// --------------------------------------------------------------------

    XMakeBuildSystem::XMakeBuildSystem(XMakeBuildConfiguration *bc)
        : BuildSystem(bc)
        , m_cppCodeModelUpdater(ProjectUpdaterFactory::createCppProjectUpdater()) {
        // TreeScanner:
        connect(&m_treeScanner, &TreeScanner::finished,
                this, &XMakeBuildSystem::handleTreeScanningFinished);

        m_treeScanner.setFilter([this](const MimeType &mimeType, const FilePath &fn) {
                                    // Mime checks requires more resources, so keep it last in check list
                                    auto isIgnored = TreeScanner::isWellKnownBinary(mimeType, fn);

                                    // Cache mime check result for speed up
                                    if (!isIgnored) {
                                        auto it = m_mimeBinaryCache.find(mimeType.name());
                                        if (it != m_mimeBinaryCache.end()) {
                                            isIgnored = *it;
                                        } else {
                                            isIgnored = TreeScanner::isMimeBinary(mimeType, fn);
                                            m_mimeBinaryCache[mimeType.name()] = isIgnored;
                                        }
                                    }

                                    return isIgnored;
                                });

        m_treeScanner.setTypeFactory([](const MimeType &mimeType, const FilePath &fn) {
                                         auto type = TreeScanner::genericFileType(mimeType, fn);
                                         if (type == FileType::Unknown) {
                                             if (mimeType.isValid()) {
                                                 const QString mt = mimeType.name();
                                                 if (mt == Utils::Constants::CMAKE_PROJECT_MIMETYPE
                                                     || mt == Utils::Constants::CMAKE_MIMETYPE) {
                                                     type = FileType::Project;
                                                 }
                                             }
                                         }
                                         return type;
                                     });

        connect(&m_reader, &FileApiReader::configurationStarted, this, [this] {
                    clearError(ForceEnabledChanged::True);
                });

        connect(&m_reader,
                &FileApiReader::dataAvailable,
                this,
                &XMakeBuildSystem::handleParsingSucceeded);
        connect(&m_reader, &FileApiReader::errorOccurred, this, &XMakeBuildSystem::handleParsingFailed);
        connect(&m_reader, &FileApiReader::dirty, this, &XMakeBuildSystem::becameDirty);
        connect(&m_reader, &FileApiReader::debuggingStarted, this, &BuildSystem::debuggingStarted);

        wireUpConnections();

        m_isMultiConfig = XMakeGeneratorKitAspect::isMultiConfigGenerator(bc->kit());
    }

    XMakeBuildSystem::~XMakeBuildSystem() {
        if (!m_treeScanner.isFinished()) {
            auto future = m_treeScanner.future();
            future.cancel();
            future.waitForFinished();
        }

        delete m_cppCodeModelUpdater;
        qDeleteAll(m_extraCompilers);
    }

    void XMakeBuildSystem::triggerParsing() {
        qCDebug(xmakeBuildSystemLog) << buildConfiguration()->displayName() << "Parsing has been triggered";

        if (!buildConfiguration()->isActive()) {
            qCDebug(xmakeBuildSystemLog)
                << "Parsing has been triggered: SKIPPING since BC is not active -- clearing state.";
            stopParsingAndClearState();
            return; // ignore request, this build configuration is not active!
        }

        auto guard = guardParsingRun();

        if (!guard.guardsProject()) {
            // This can legitimately trigger if e.g. Build->Run XMake
            // is selected while this here is already running.

            // Stop old parse run and keep that ParseGuard!
            qCDebug(xmakeBuildSystemLog) << "Stopping current parsing run!";
            stopParsingAndClearState();
        } else {
            // Use new ParseGuard
            m_currentGuard = std::move(guard);
        }
        QTC_ASSERT(!m_reader.isParsing(), return );

        qCDebug(xmakeBuildSystemLog) << "ParseGuard acquired.";

        int reparseParameters = takeReparseParameters();

        m_waitingForParse = true;
        m_combinedScanAndParseResult = true;

        QTC_ASSERT(m_parameters.isValid(), return );

        TaskHub::clearTasks(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);

        qCDebug(xmakeBuildSystemLog) << "Parse called with flags:"
                                     << reparseParametersString(reparseParameters);

        const FilePath cache = m_parameters.buildDirectory.pathAppended(Constants::CMAKE_CACHE_TXT);
        if (!cache.exists()) {
            reparseParameters |= REPARSE_FORCE_INITIAL_CONFIGURATION | REPARSE_FORCE_CMAKE_RUN;
            qCDebug(xmakeBuildSystemLog)
                << "No" << cache
                << "file found, new flags:" << reparseParametersString(reparseParameters);
        }

        if ((0 == (reparseParameters & REPARSE_FORCE_EXTRA_CONFIGURATION))
            && mustApplyConfigurationChangesArguments(m_parameters)) {
            reparseParameters |= REPARSE_FORCE_CMAKE_RUN | REPARSE_FORCE_EXTRA_CONFIGURATION;
        }

        // The code model will be updated after the XMake run. There is no need to have an
        // active code model updater when the next one will be triggered.
        m_cppCodeModelUpdater->cancel();

        const XMakeTool *tool = m_parameters.xmakeTool();
        XMakeTool::Version version = tool ? tool->version() : XMakeTool::Version();
        const bool isDebuggable = (version.major == 3 && version.minor >= 27) || version.major > 3;

        qCDebug(xmakeBuildSystemLog) << "Asking reader to parse";
        m_reader.parse(reparseParameters & REPARSE_FORCE_CMAKE_RUN,
                       reparseParameters & REPARSE_FORCE_INITIAL_CONFIGURATION,
                       reparseParameters & REPARSE_FORCE_EXTRA_CONFIGURATION,
                       (reparseParameters & REPARSE_DEBUG) && isDebuggable,
                       reparseParameters & REPARSE_PROFILING);
    }

    void XMakeBuildSystem::requestDebugging() {
        qCDebug(xmakeBuildSystemLog) << "Requesting parse due to \"Rescan Project\" command";
        reparse(REPARSE_FORCE_CMAKE_RUN | REPARSE_FORCE_EXTRA_CONFIGURATION | REPARSE_URGENT
                | REPARSE_DEBUG);
    }

    bool XMakeBuildSystem::supportsAction(Node *context, ProjectAction action, const Node *node) const {
        const auto xmakeTarget = dynamic_cast<XMakeTargetNode *>(context);
        if (xmakeTarget) {
            const auto buildTarget = Utils::findOrDefault(m_buildTargets,
                                                          [xmakeTarget](const XMakeBuildTarget &bt) {
                                                              return bt.title
                                                                     == xmakeTarget->buildKey();
                                                          });
            if (buildTarget.targetType != UtilityType) {
                return action == ProjectAction::AddNewFile || action == ProjectAction::AddExistingFile
                       || action == ProjectAction::AddExistingDirectory
                       || action == ProjectAction::Rename || action == ProjectAction::RemoveFile;
            }
        }

        return BuildSystem::supportsAction(context, action, node);
    }

    static QString relativeFilePaths(const FilePaths &filePaths, const FilePath &projectDir) {
        return Utils::transform(filePaths, [projectDir](const FilePath &path) {
                                    return path.canonicalPath().relativePathFrom(projectDir).cleanPath().toString();
                                }).join(' ');
    };

    static QString newFilesForFunction(const std::string &xmakeFunction,
                                       const FilePaths &filePaths,
                                       const FilePath &projDir) {
        if (xmakeFunction == "qt_add_qml_module" || xmakeFunction == "qt6_add_qml_module") {
            FilePaths sourceFiles;
            FilePaths resourceFiles;
            FilePaths qmlFiles;

            for (const auto &file : filePaths) {
                using namespace Utils::Constants;
                const auto mimeType = Utils::mimeTypeForFile(file);
                if (mimeType.matchesName(CPP_SOURCE_MIMETYPE)
                    || mimeType.matchesName(CPP_HEADER_MIMETYPE)
                    || mimeType.matchesName(OBJECTIVE_C_SOURCE_MIMETYPE)
                    || mimeType.matchesName(OBJECTIVE_CPP_SOURCE_MIMETYPE)) {
                    sourceFiles << file;
                } else if (mimeType.matchesName(QML_MIMETYPE)
                           || mimeType.matchesName(QMLUI_MIMETYPE)
                           || mimeType.matchesName(QMLPROJECT_MIMETYPE)
                           || mimeType.matchesName(JS_MIMETYPE)
                           || mimeType.matchesName(JSON_MIMETYPE)) {
                    qmlFiles << file;
                } else {
                    resourceFiles << file;
                }
            }

            QStringList result;
            if (!sourceFiles.isEmpty()) {
                result << QString("SOURCES %1").arg(relativeFilePaths(sourceFiles, projDir));
            }
            if (!resourceFiles.isEmpty()) {
                result << QString("RESOURCES %1").arg(relativeFilePaths(resourceFiles, projDir));
            }
            if (!qmlFiles.isEmpty()) {
                result << QString("QML_FILES %1").arg(relativeFilePaths(qmlFiles, projDir));
            }

            return result.join("\n");
        }

        return relativeFilePaths(filePaths, projDir);
    }

    static std::optional<Link> xmakeFileForBuildKey(const QString &buildKey,
                                                    const QList<XMakeBuildTarget> &targets) {
        auto target = Utils::findOrDefault(targets, [buildKey](const XMakeBuildTarget &target) {
                                               return target.title == buildKey;
                                           });
        if (target.backtrace.isEmpty()) {
            qCCritical(xmakeBuildSystemLog) << "target.backtrace for" << buildKey << "is empty."
                                            << "The location where to add the files is unknown.";
            return std::nullopt;
        }
        return std::make_optional(Link(target.backtrace.last().path, target.backtrace.last().line));
    }

    static std::optional<cmListFile> getUncachedXMakeListFile(const FilePath &targetXMakeFile) {
        // Have a fresh look at the XMake file, not relying on a cached value
        Core::DocumentManager::saveModifiedDocumentSilently(
            Core::DocumentModel::documentForFilePath(targetXMakeFile));
        expected_str<QByteArray> fileContent = targetXMakeFile.fileContents();
        cmListFile xmakeListFile;
        std::string errorString;
        if (fileContent) {
            fileContent = fileContent->replace("\r\n", "\n");
            if (!xmakeListFile.ParseString(fileContent->toStdString(),
                                           targetXMakeFile.fileName().toStdString(),
                                           errorString)) {
                qCCritical(xmakeBuildSystemLog).noquote() << targetXMakeFile.toUserOutput()
                                                          << "failed to parse! Error:"
                                                          << QString::fromStdString(errorString);
                return std::nullopt;
            }
        }
        return std::make_optional(xmakeListFile);
    }

    static std::optional<cmListFileFunction> findFunction(
        const cmListFile &xmakeListFile, std::function<bool(const cmListFileFunction &)> pred,
        bool reverse = false) {
        if (reverse) {
            auto function = std::find_if(xmakeListFile.Functions.rbegin(),
                                         xmakeListFile.Functions.rend(), pred);
            if (function == xmakeListFile.Functions.rend()) {
                return std::nullopt;
            }
            return std::make_optional(*function);
        }
        auto function
            = std::find_if(xmakeListFile.Functions.begin(), xmakeListFile.Functions.end(), pred);
        if (function == xmakeListFile.Functions.end()) {
            return std::nullopt;
        }
        return std::make_optional(*function);
    }

    struct SnippetAndLocation {
        QString snippet;
        long line = -1;
        long column = -1;
    };

    static SnippetAndLocation generateSnippetAndLocationForSources(
        const QString &newSourceFiles,
        const cmListFile &xmakeListFile,
        const cmListFileFunction &function,
        const QString &targetName) {
        static QSet<std::string> knownFunctions { "add_executable",
                                                  "add_library",
                                                  "qt_add_executable",
                                                  "qt_add_library",
                                                  "qt6_add_executable",
                                                  "qt6_add_library",
                                                  "qt_add_qml_module",
                                                  "qt6_add_qml_module" };
        SnippetAndLocation result;
        int extraChars = 0;
        auto afterFunctionLastArgument =
            [&result, &extraChars, newSourceFiles](const auto &f) {
                auto lastArgument = f.Arguments().back();
                result.line = lastArgument.Line;
                result.column = lastArgument.Column + static_cast<int>(lastArgument.Value.size()) - 1;
                result.snippet = QString("\n%1").arg(newSourceFiles);
                // Take into consideration the quotes
                if (lastArgument.Delim == cmListFileArgument::Quoted) {
                    extraChars = 2;
                }
            };
        if (knownFunctions.contains(function.LowerCaseName())) {
            afterFunctionLastArgument(function);
        } else {
            const std::string target_name = targetName.toStdString();
            auto targetSources = [target_name](const auto &func) {
                return func.LowerCaseName() == "target_sources"
                       && func.Arguments().size() && func.Arguments().front().Value == target_name;
            };
            std::optional<cmListFileFunction> targetSourcesFunc = findFunction(xmakeListFile,
                                                                               targetSources);
            if (!targetSourcesFunc.has_value()) {
                result.line = function.LineEnd() + 1;
                result.column = 0;
                result.snippet = QString("\ntarget_sources(%1\n  PRIVATE\n    %2\n)\n")
                    .arg(targetName)
                    .arg(newSourceFiles);
            } else {
                afterFunctionLastArgument(*targetSourcesFunc);
            }
        }
        if (extraChars) {
            result.line += extraChars;
        }
        return result;
    }
    static expected_str<bool> insertSnippetSilently(const FilePath &xmakeFile,
                                                    const SnippetAndLocation &snippetLocation) {
        BaseTextEditor *editor = qobject_cast<BaseTextEditor *>(
            Core::EditorManager::openEditorAt({ xmakeFile,
                                                int(snippetLocation.line),
                                                int(snippetLocation.column) },
                                              Constants::CMAKE_EDITOR_ID,
                                              Core::EditorManager::DoNotMakeVisible));
        if (!editor) {
            return make_unexpected("BaseTextEditor cannot be obtained for " + xmakeFile.toUserOutput()
                                   + ":" + QString::number(snippetLocation.line) + ":"
                                   + QString::number(snippetLocation.column));
        }
        editor->insert(snippetLocation.snippet);
        editor->editorWidget()->autoIndent();
        if (!Core::DocumentManager::saveDocument(editor->document())) {
            return make_unexpected("Changes to " + xmakeFile.toUserOutput() + " could not be saved.");
        }
        return true;
    }

    static void findLastRelevantArgument(const cmListFileFunction &function,
                                         int minimumArgPos,
                                         const QSet<QString> &lowerCaseStopParams,
                                         QString *lastRelevantArg,
                                         int *lastRelevantPos) {
        const std::vector<cmListFileArgument> args = function.Arguments();
        *lastRelevantPos = int(args.size()) - 1;
        for (int i = minimumArgPos, end = int(args.size()); i < end; ++i) {
            const QString lowerArg = QString::fromStdString(args.at(i).Value).toLower();
            if (lowerCaseStopParams.contains(lowerArg)) {
                *lastRelevantPos = i - 1;
                break;
            }
            *lastRelevantArg = lowerArg;
        }
    }

    static std::optional<cmListFileFunction> findSetFunctionFor(const cmListFile &xmakeListFile,
                                                                const QString &lowerVariableName) {
        auto findSetFunc = [lowerVariableName](const auto &func) {
            if (func.LowerCaseName() != "set") {
                return false;
            }
            std::vector<cmListFileArgument> args = func.Arguments();
            return args.size()
                   && QString::fromStdString(args.front().Value).toLower() == lowerVariableName;
        };
        return findFunction(xmakeListFile, findSetFunc);
    }

    static std::optional<cmListFileFunction> handleTSAddVariant(const cmListFile &xmakeListFile,
                                                                const QSet<QString> &lowerFunctionNames,
                                                                std::optional<QString> targetName,
                                                                const QSet<QString> &stopParams,
                                                                int *lastArgumentPos) {
        std::optional<cmListFileFunction> function;
        auto currentFunc = findFunction(xmakeListFile, [lowerFunctionNames, targetName](const auto &func) {
                                            auto lower = QString::fromStdString(func.LowerCaseName());
                                            if (lowerFunctionNames.contains(lower)) {
                                                if (!targetName) {
                                                    return true;
                                                }
                                                const std::vector<cmListFileArgument> args = func.Arguments();
                                                if (args.size()) {
                                                    return *targetName == QString::fromStdString(args.front().Value);
                                                }
                                            }
                                            return false;
                                        });
        if (currentFunc) {
            QString lastRelevant;
            const int argsMinimumPos = targetName.has_value() ? 2 : 1;

            findLastRelevantArgument(*currentFunc, argsMinimumPos, stopParams,
                                     &lastRelevant, lastArgumentPos);
            // handle argument
            if (!lastRelevant.isEmpty() && lastRelevant.startsWith('$')) {
                QString var = lastRelevant.mid(1);
                if (var.startsWith('{') && var.endsWith('}')) {
                    var = var.mid(1, var.size() - 2);
                }
                if (!var.isEmpty()) {
                    std::optional<cmListFileFunction> setFunc = findSetFunctionFor(xmakeListFile, var);
                    if (setFunc) {
                        function = *setFunc;
                        *lastArgumentPos = int(function->Arguments().size()) - 1;
                    }
                }
            }
            if (!function.has_value()) { // no variable used or we failed to find respective SET()
                function = currentFunc;
            }
        }
        return function;
    }

    static std::optional<cmListFileFunction> handleQtAddTranslations(const cmListFile &xmakeListFile,
                                                                     std::optional<QString> targetName,
                                                                     int *lastArgumentPos) {
        const QSet<QString> stopParams { "resource_prefix", "output_targets",
                                         "qm_files_output_variable", "sources", "include_directories",
                                         "lupdate_options", "lrelease_options" };
        return handleTSAddVariant(xmakeListFile, { "qt6_add_translations", "qt_add_translations" },
                                  targetName, stopParams, lastArgumentPos);
    }

    static std::optional<cmListFileFunction> handleQtAddLupdate(const cmListFile &xmakeListFile,
                                                                std::optional<QString> targetName,
                                                                int *lastArgumentPos) {
        const QSet<QString> stopParams { "sources", "include_directories", "no_global_target", "options" };
        return handleTSAddVariant(xmakeListFile, { "qt6_add_lupdate", "qt_add_lupdate" },
                                  targetName, stopParams, lastArgumentPos);
    }

    static std::optional<cmListFileFunction> handleQtCreateTranslation(const cmListFile &xmakeListFile,
                                                                       int *lastArgumentPos) {
        return handleTSAddVariant(xmakeListFile, { "qt_create_translation", "qt5_create_translation" },
                                  std::nullopt, { "options" }, lastArgumentPos);
    }

    static expected_str<bool> insertQtAddTranslations(const cmListFile &xmakeListFile,
                                                      const FilePath &targetCmakeFile,
                                                      const QString &targetName,
                                                      int targetDefinitionLine,
                                                      const QString &filesToAdd,
                                                      int qtMajorVersion,
                                                      bool addLinguist) {
        std::optional<cmListFileFunction> function
            = findFunction(xmakeListFile, [targetDefinitionLine](const auto &func) {
                               return func.Line() == targetDefinitionLine;
                           });
        if (!function.has_value()) {
            return false;
        }

        // FIXME: room for improvement
        // * this just updates "the current xmake path" for e.g. conditional setups like
        // differentiating between desktop and device build config we do not update all
        QString snippet;
        if (qtMajorVersion == 5) {
            snippet = QString("\nqt_create_translation(QM_FILES %1)\n").arg(filesToAdd);
        } else {
            snippet = QString("\nqt_add_translations(%1 TS_FILES %2)\n").arg(targetName, filesToAdd);
        }

        const int insertionLine = function->LineEnd() + 1;
        expected_str<bool> inserted = insertSnippetSilently(targetCmakeFile,
                                                            { snippet, insertionLine, 0 });
        if (!inserted || !addLinguist) {
            return inserted;
        }

        function = findFunction(xmakeListFile, [](const auto &func) {
                                    return func.LowerCaseName() == "find_package";
                                }, /* reverse = */ true);
        if (!function.has_value()) {
            qCCritical(xmakeBuildSystemLog) << "Failed to find a find_package().";
            return inserted; // we just fail to insert LinguistTool, but otherwise succeeded
        }
        if (insertionLine < function->LineEnd() + 1) {
            qCCritical(xmakeBuildSystemLog) << "find_package() calls after old insertion. "
                "Refusing to process.";
            return inserted; // we just fail to insert LinguistTool, but otherwise succeeded
        }

        snippet = QString("find_package(Qt%1 REQUIRED COMPONENTS LinguistTools)\n").arg(qtMajorVersion);
        return insertSnippetSilently(targetCmakeFile, { snippet, function->LineEnd() + 1, 0 });
    }

    bool XMakeBuildSystem::addTsFiles(Node *context, const FilePaths &filePaths, FilePaths *notAdded) {
        if (notAdded) {
            notAdded->append(filePaths);
        }

        if (auto n = dynamic_cast<XMakeTargetNode *>(context)) {
            const QString targetName = n->buildKey();
            const std::optional<Link> xmakeFile = xmakeFileForBuildKey(targetName, buildTargets());
            if (!xmakeFile.has_value()) {
                return false;
            }

            const FilePath targetXMakeFile = xmakeFile->targetFilePath;
            std::optional<cmListFile> xmakeListFile = getUncachedXMakeListFile(targetXMakeFile);
            if (!xmakeListFile.has_value()) {
                return false;
            }

            int lastArgumentPos = -1;
            std::optional<cmListFileFunction> function
                = handleQtAddTranslations(*xmakeListFile, targetName, &lastArgumentPos);
            if (!function.has_value()) {
                function = handleQtAddLupdate(*xmakeListFile, targetName, &lastArgumentPos);
            }
            if (!function.has_value()) {
                function = handleQtCreateTranslation(*xmakeListFile, &lastArgumentPos);
            }

            const QString filesToAdd = relativeFilePaths(filePaths, n->filePath().canonicalPath());
            bool linguistToolsMissing = false;
            int qtMajorVersion = -1;
            if (!function.has_value()) {
                if (auto qt = m_findPackagesFilesHash.value("Qt6Core"); qt.hasValidTarget()) {
                    qtMajorVersion = 6;
                } else if (auto qt = m_findPackagesFilesHash.value("Qt5Core"); qt.hasValidTarget()) {
                    qtMajorVersion = 5;
                }

                if (qtMajorVersion != -1) {
                    const QString linguistTools = QString("Qt%1LinguistTools").arg(qtMajorVersion);
                    auto linguist = m_findPackagesFilesHash.value(linguistTools);
                    linguistToolsMissing = !linguist.hasValidTarget();
                }

                // we failed to find any pre-existing, add one ourself
                expected_str<bool> inserted = insertQtAddTranslations(*xmakeListFile,
                                                                      targetXMakeFile,
                                                                      targetName,
                                                                      xmakeFile->targetLine,
                                                                      filesToAdd,
                                                                      qtMajorVersion,
                                                                      linguistToolsMissing);
                if (!inserted) {
                    qCCritical(xmakeBuildSystemLog) << inserted.error();
                } else if (notAdded) {
                    notAdded->removeIf([filePaths](const FilePath &p) {
                                           return filePaths.contains(p);
                                       });
                }

                return inserted.value_or(false);
            }

            auto lastArgument = function->Arguments().at(lastArgumentPos);
            const int lastArgLength = static_cast<int>(lastArgument.Value.size()) - 1;
            SnippetAndLocation snippetLocation { QString("\n%1").arg(filesToAdd),
                                                 lastArgument.Line, lastArgument.Column + lastArgLength };
            // Take into consideration the quotes
            if (lastArgument.Delim == cmListFileArgument::Quoted) {
                snippetLocation.column += 2;
            }

            expected_str<bool> inserted = insertSnippetSilently(targetXMakeFile, snippetLocation);
            if (!inserted) {
                qCCritical(xmakeBuildSystemLog) << inserted.error();
                return false;
            }

            if (notAdded) {
                notAdded->removeIf([filePaths](const FilePath &p) {
                                       return filePaths.contains(p);
                                   });
            }
            return true;
        }
        return false;
    }

    bool XMakeBuildSystem::addSrcFiles(Node *context, const FilePaths &filePaths, FilePaths *notAdded) {
        if (notAdded) {
            notAdded->append(filePaths);
        }

        if (auto n = dynamic_cast<XMakeTargetNode *>(context)) {
            const QString targetName = n->buildKey();
            const std::optional<Link> xmakeFile = xmakeFileForBuildKey(targetName, buildTargets());
            if (!xmakeFile) {
                return false;
            }

            const FilePath targetXMakeFile = xmakeFile->targetFilePath;
            const int targetDefinitionLine = xmakeFile->targetLine;

            std::optional<cmListFile> xmakeListFile = getUncachedXMakeListFile(targetXMakeFile);
            if (!xmakeListFile) {
                return false;
            }

            std::optional<cmListFileFunction> function
                = findFunction(*xmakeListFile, [targetDefinitionLine](const auto &func) {
                                   return func.Line() == targetDefinitionLine;
                               });
            if (!function.has_value()) {
                qCCritical(xmakeBuildSystemLog) << "Function that defined the target" << targetName
                                                << "could not be found at" << targetDefinitionLine;
                return false;
            }

            const std::string target_name = function->Arguments().front().Value;
            auto qtAddModule = [target_name](const auto &func) {
                return (func.LowerCaseName() == "qt_add_qml_module"
                    || func.LowerCaseName() == "qt6_add_qml_module")
                       && func.Arguments().front().Value == target_name;
            };
            // Special case: when qt_add_executable and qt_add_qml_module use the same target name
            // then qt_add_qml_module function should be used
            function = findFunction(*xmakeListFile, qtAddModule).value_or(*function);

            const QString newSourceFiles = newFilesForFunction(function->LowerCaseName(),
                                                               filePaths,
                                                               n->filePath().canonicalPath());

            const SnippetAndLocation snippetLocation = generateSnippetAndLocationForSources(
                newSourceFiles, *xmakeListFile, *function, targetName);
            expected_str<bool> inserted = insertSnippetSilently(targetXMakeFile, snippetLocation);
            if (!inserted) {
                qCCritical(xmakeBuildSystemLog) << inserted.error();
                return false;
            }

            if (notAdded) {
                notAdded->removeIf([filePaths](const FilePath &p) {
                                       return filePaths.contains(p);
                                   });
            }
            return true;
        }

        return false;
    }

    bool XMakeBuildSystem::addFiles(Node *context, const FilePaths &filePaths, FilePaths *notAdded) {
        FilePaths tsFiles, srcFiles;
        std::tie(tsFiles, srcFiles) = Utils::partition(filePaths, [](const FilePath &fp) {
                                                           return Utils::mimeTypeForFile(fp.toString()).name() == Utils::Constants::LINGUIST_MIMETYPE;
                                                       });
        bool success = true;
        if (!srcFiles.isEmpty()) {
            success = addSrcFiles(context, srcFiles, notAdded);
        }

        if (!tsFiles.isEmpty()) {
            success = addTsFiles(context, tsFiles, notAdded) || success;
        }

        if (success) {
            return true;
        }
        return BuildSystem::addFiles(context, filePaths, notAdded);
    }

    std::optional<XMakeBuildSystem::ProjectFileArgumentPosition>
    XMakeBuildSystem::projectFileArgumentPosition(const QString &targetName, const QString &fileName) {
        const std::optional<Link> xmakeFile = xmakeFileForBuildKey(targetName, buildTargets());
        if (!xmakeFile) {
            return std::nullopt;
        }

        const FilePath targetXMakeFile = xmakeFile->targetFilePath;
        const int targetDefinitionLine = xmakeFile->targetLine;

        std::optional<cmListFile> xmakeListFile = getUncachedXMakeListFile(targetXMakeFile);
        if (!xmakeListFile) {
            return std::nullopt;
        }

        std::optional<cmListFileFunction> function
            = findFunction(*xmakeListFile, [targetDefinitionLine](const auto &func) {
                               return func.Line() == targetDefinitionLine;
                           });
        if (!function.has_value()) {
            qCCritical(xmakeBuildSystemLog) << "Function that defined the target" << targetName
                                            << "could not be found at" << targetDefinitionLine;
            return std::nullopt;
        }

        const std::string target_name = targetName.toStdString();
        auto targetSourcesFunc = findFunction(*xmakeListFile, [target_name](const auto &func) {
                                                  return func.LowerCaseName() == "target_sources" && func.Arguments().size() > 1
                                                         && func.Arguments().front().Value == target_name;
                                              });

        auto addQmlModuleFunc = findFunction(*xmakeListFile, [target_name](const auto &func) {
                                                 return (func.LowerCaseName() == "qt_add_qml_module"
                                                     || func.LowerCaseName() == "qt6_add_qml_module")
                                                        && func.Arguments().size() > 1 && func.Arguments().front().Value == target_name;
                                             });

        auto setSourceFilePropFunc = findFunction(*xmakeListFile, [](const auto &func) {
                                                      return func.LowerCaseName() == "set_source_files_properties";
                                                  });

        for (const auto &func : { function, targetSourcesFunc, addQmlModuleFunc, setSourceFilePropFunc }) {
            if (!func.has_value()) {
                continue;
            }
            auto filePathArgument = Utils::findOrDefault(
                func->Arguments(), [file_name = fileName.toStdString()](const auto &arg) {
                    return arg.Value == file_name;
                });

            if (!filePathArgument.Value.empty()) {
                return ProjectFileArgumentPosition { filePathArgument, targetXMakeFile, fileName };
            } else {
                // Check if the filename is part of globbing variable result
                const auto globFunctions = std::get<0>(
                    Utils::partition(xmakeListFile->Functions, [](const auto &f) {
                                         return f.LowerCaseName() == "file" && f.Arguments().size() > 2
                                                && (f.Arguments().front().Value == "GLOB"
                                                    || f.Arguments().front().Value == "GLOB_RECURSE");
                                     }));

                const auto globVariables = Utils::transform<QSet>(globFunctions, [](const auto &func) {
                                                                      return std::string("${") + func.Arguments()[1].Value + "}";
                                                                  });

                const auto haveGlobbing = Utils::anyOf(func->Arguments(),
                                                       [globVariables](const auto &arg) {
                                                           return globVariables.contains(arg.Value);
                                                       });

                if (haveGlobbing) {
                    return ProjectFileArgumentPosition { filePathArgument,
                                                         targetXMakeFile,
                                                         fileName,
                                                         true };
                }

                // Check if the filename is part of a variable set by the user
                const auto setFunctions = std::get<0>(
                    Utils::partition(xmakeListFile->Functions, [](const auto &f) {
                                         return f.LowerCaseName() == "set" && f.Arguments().size() > 1;
                                     }));

                for (const auto &arg : func->Arguments()) {
                    auto matchedFunctions = Utils::filtered(setFunctions, [arg](const auto &f) {
                                                                return arg.Value == std::string("${") + f.Arguments()[0].Value + "}";
                                                            });

                    for (const auto &f : matchedFunctions) {
                        filePathArgument = Utils::findOrDefault(f.Arguments(),
                                                                [file_name = fileName.toStdString()](
                                                                    const auto &arg) {
                                                                    return arg.Value == file_name;
                                                                });

                        if (!filePathArgument.Value.empty()) {
                            return ProjectFileArgumentPosition { filePathArgument,
                                                                 targetXMakeFile,
                                                                 fileName };
                        }
                    }
                }
            }
        }

        return std::nullopt;
    }

    RemovedFilesFromProject XMakeBuildSystem::removeFiles(Node *context,
                                                          const FilePaths &filePaths,
                                                          FilePaths *notRemoved) {
        FilePaths badFiles;
        if (auto n = dynamic_cast<XMakeTargetNode *>(context)) {
            const FilePath projDir = n->filePath().canonicalPath();
            const QString targetName = n->buildKey();

            for (const auto &file : filePaths) {
                const QString fileName
                    = file.canonicalPath().relativePathFrom(projDir).cleanPath().toString();

                auto filePos = projectFileArgumentPosition(targetName, fileName);
                if (filePos) {
                    if (!filePos.value().xmakeFile.exists()) {
                        badFiles << file;

                        qCCritical(xmakeBuildSystemLog).noquote()
                            << "File" << filePos.value().xmakeFile.path() << "does not exist.";
                        continue;
                    }

                    BaseTextEditor *editor = qobject_cast<BaseTextEditor *>(
                        Core::EditorManager::openEditorAt({ filePos.value().xmakeFile,
                                                            static_cast<int>(filePos.value().argumentPosition.Line),
                                                            static_cast<int>(filePos.value().argumentPosition.Column
                                                                - 1) },
                                                          Constants::CMAKE_EDITOR_ID,
                                                          Core::EditorManager::DoNotMakeVisible));
                    if (!editor) {
                        badFiles << file;

                        qCCritical(xmakeBuildSystemLog).noquote()
                            << "BaseTextEditor cannot be obtained for"
                            << filePos.value().xmakeFile.path() << filePos.value().argumentPosition.Line
                            << int(filePos.value().argumentPosition.Column - 1);
                        continue;
                    }

                    // If quotes were used for the source file, remove the quotes too
                    int extraChars = 0;
                    if (filePos->argumentPosition.Delim == cmListFileArgument::Quoted) {
                        extraChars = 2;
                    }

                    if (!filePos.value().fromGlobbing) {
                        editor->replace(filePos.value().relativeFileName.length() + extraChars, "");
                    }

                    editor->editorWidget()->autoIndent();
                    if (!Core::DocumentManager::saveDocument(editor->document())) {
                        badFiles << file;

                        qCCritical(xmakeBuildSystemLog).noquote()
                            << "Changes to" << filePos.value().xmakeFile.path()
                            << "could not be saved.";
                        continue;
                    }
                } else {
                    badFiles << file;
                }
            }

            if (notRemoved && !badFiles.isEmpty()) {
                *notRemoved = badFiles;
            }

            return badFiles.isEmpty() ? RemovedFilesFromProject::Ok : RemovedFilesFromProject::Error;
        }

        return RemovedFilesFromProject::Error;
    }

    bool XMakeBuildSystem::canRenameFile(Node *context,
                                         const FilePath &oldFilePath,
                                         const FilePath &newFilePath) {
        // "canRenameFile" will cause an actual rename after the function call.
        // This will make the a sequence like
        // canonicalPath().relativePathFrom(projDir).cleanPath().toString()
        // to fail if the file doesn't exist on disk
        // therefore cache the results for the subsequent "renameFile" call
        // where oldFilePath has already been renamed as newFilePath.

        if (auto n = dynamic_cast<XMakeTargetNode *>(context)) {
            const FilePath projDir = n->filePath().canonicalPath();
            const QString oldRelPathName
                = oldFilePath.canonicalPath().relativePathFrom(projDir).cleanPath().toString();

            const QString targetName = n->buildKey();

            const QString key
                = QStringList { projDir.path(), targetName, oldFilePath.path(), newFilePath.path() }
            .join(";");

            auto filePos = projectFileArgumentPosition(targetName, oldRelPathName);
            if (!filePos) {
                return false;
            }

            m_filesToBeRenamed.insert(key, filePos.value());
            return true;
        }
        return false;
    }

    bool XMakeBuildSystem::renameFile(Node *context,
                                      const FilePath &oldFilePath,
                                      const FilePath &newFilePath) {
        if (auto n = dynamic_cast<XMakeTargetNode *>(context)) {
            const FilePath projDir = n->filePath().canonicalPath();
            const FilePath newRelPath = newFilePath.canonicalPath().relativePathFrom(projDir).cleanPath();
            const QString newRelPathName = newRelPath.toString();

            // FilePath needs the file to exist on disk, the old file has already been renamed
            const QString oldRelPathName
                = newRelPath.parentDir().pathAppended(oldFilePath.fileName()).cleanPath().toString();

            const QString targetName = n->buildKey();
            const QString key
                = QStringList { projDir.path(), targetName, oldFilePath.path(), newFilePath.path() }.join(
                ";");

            std::optional<XMakeBuildSystem::ProjectFileArgumentPosition> fileToRename
                = m_filesToBeRenamed.take(key);
            if (!fileToRename->xmakeFile.exists()) {
                qCCritical(xmakeBuildSystemLog).noquote()
                    << "File" << fileToRename->xmakeFile.path() << "does not exist.";
                return false;
            }

            do {
                BaseTextEditor *editor = qobject_cast<BaseTextEditor *>(
                    Core::EditorManager::openEditorAt(
                        { fileToRename->xmakeFile,
                          static_cast<int>(fileToRename->argumentPosition.Line),
                          static_cast<int>(fileToRename->argumentPosition.Column - 1) },
                        Constants::CMAKE_EDITOR_ID,
                        Core::EditorManager::DoNotMakeVisible));
                if (!editor) {
                    qCCritical(xmakeBuildSystemLog).noquote()
                        << "BaseTextEditor cannot be obtained for" << fileToRename->xmakeFile.path()
                        << fileToRename->argumentPosition.Line
                        << int(fileToRename->argumentPosition.Column);
                    return false;
                }

                // If quotes were used for the source file, skip the starting quote
                if (fileToRename->argumentPosition.Delim == cmListFileArgument::Quoted) {
                    editor->setCursorPosition(editor->position() + 1);
                }

                if (!fileToRename->fromGlobbing) {
                    editor->replace(fileToRename->relativeFileName.length(), newRelPathName);
                }

                editor->editorWidget()->autoIndent();
                if (!Core::DocumentManager::saveDocument(editor->document())) {
                    qCCritical(xmakeBuildSystemLog).noquote()
                        << "Changes to" << fileToRename->xmakeFile.path() << "could not be saved.";
                    return false;
                }

                // Try the next occurrence. This can happen if set_source_file_properties is used
                fileToRename = projectFileArgumentPosition(targetName, oldRelPathName);
            } while (fileToRename);

            return true;
        }

        return false;
    }

    FilePaths XMakeBuildSystem::filesGeneratedFrom(const FilePath &sourceFile) const {
        FilePath project = projectDirectory();
        FilePath baseDirectory = sourceFile.parentDir();

        while (baseDirectory.isChildOf(project)) {
            const FilePath xmakeListsTxt = baseDirectory.pathAppended(Constants::CMAKE_LISTS_TXT);
            if (xmakeListsTxt.exists()) {
                break;
            }
            baseDirectory = baseDirectory.parentDir();
        }

        const FilePath relativePath = baseDirectory.relativePathFrom(project);
        FilePath generatedFilePath = buildConfiguration()->buildDirectory().resolvePath(relativePath);

        if (sourceFile.suffix() == "ui") {
            const QString generatedFileName = "ui_" + sourceFile.completeBaseName() + ".h";

            auto targetNode = this->project()->nodeForFilePath(sourceFile);
            while (targetNode && !dynamic_cast<const XMakeTargetNode *>(targetNode)) {
                targetNode = targetNode->parentFolderNode();
            }

            FilePaths generatedFilePaths;
            if (targetNode) {
                const QString autogenSignature = targetNode->buildKey() + "_autogen/include";

                // If AUTOUIC reports the generated header file name, use that path
                generatedFilePaths = this->project()->files(
                    [autogenSignature, generatedFileName](const Node *n) {
                        const FilePath filePath = n->filePath();
                        if (!filePath.contains(autogenSignature)) {
                            return false;
                        }

                        return Project::GeneratedFiles(n) && filePath.endsWith(generatedFileName);
                    });
            }

            if (generatedFilePaths.empty()) {
                generatedFilePaths = { generatedFilePath.pathAppended(generatedFileName) };
            }

            return generatedFilePaths;
        }
        if (sourceFile.suffix() == "scxml") {
            generatedFilePath = generatedFilePath.pathAppended(sourceFile.completeBaseName());
            return { generatedFilePath.stringAppended(".h"), generatedFilePath.stringAppended(".cpp") };
        }

        // TODO: Other types will be added when adapters for their compilers become available.
        return {};
    }

    QString XMakeBuildSystem::reparseParametersString(int reparseFlags) {
        QString result;
        if (reparseFlags == REPARSE_DEFAULT) {
            result = "<NONE>";
        } else {
            if (reparseFlags & REPARSE_URGENT) {
                result += " URGENT";
            }
            if (reparseFlags & REPARSE_FORCE_CMAKE_RUN) {
                result += " FORCE_CMAKE_RUN";
            }
            if (reparseFlags & REPARSE_FORCE_INITIAL_CONFIGURATION) {
                result += " FORCE_CONFIG";
            }
        }
        return result.trimmed();
    }

    void XMakeBuildSystem::reparse(int reparseParameters) {
        setParametersAndRequestParse(BuildDirParameters(this), reparseParameters);
    }

    void XMakeBuildSystem::setParametersAndRequestParse(const BuildDirParameters &parameters,
                                                        const int reparseParameters) {
        project()->clearIssues();

        qCDebug(xmakeBuildSystemLog) << buildConfiguration()->displayName()
                                     << "setting parameters and requesting reparse"
                                     << reparseParametersString(reparseParameters);

        if (!buildConfiguration()->isActive()) {
            qCDebug(xmakeBuildSystemLog) << "setting parameters and requesting reparse: SKIPPING since BC is not active -- clearing state.";
            stopParsingAndClearState();
            return; // ignore request, this build configuration is not active!
        }

        const XMakeTool *tool = parameters.xmakeTool();
        if (!tool || !tool->isValid()) {
            TaskHub::addTask(
                BuildSystemTask(Task::Error,
                                Tr::tr("The kit needs to define a XMake tool to parse this project.")));
            return;
        }
        if (!tool->hasFileApi()) {
            TaskHub::addTask(
                BuildSystemTask(Task::Error,
                                XMakeKitAspect::msgUnsupportedVersion(tool->version().fullVersion)));
            return;
        }
        QTC_ASSERT(parameters.isValid(), return );

        m_parameters = parameters;
        ensureBuildDirectory(parameters);
        updateReparseParameters(reparseParameters);

        m_reader.setParameters(m_parameters);

        if (reparseParameters & REPARSE_URGENT) {
            qCDebug(xmakeBuildSystemLog) << "calling requestReparse";
            requestParse();
        } else {
            qCDebug(xmakeBuildSystemLog) << "calling requestDelayedReparse";
            requestDelayedParse();
        }
    }

    bool XMakeBuildSystem::mustApplyConfigurationChangesArguments(const BuildDirParameters &parameters) const {
        if (parameters.configurationChangesArguments.isEmpty()) {
            return false;
        }

        int answer = QMessageBox::question(Core::ICore::dialogParent(),
                                           Tr::tr("Apply configuration changes?"),
                                           "<p>" + Tr::tr("Run XMake with configuration changes?")
                                           + "</p><pre>"
                                           + parameters.configurationChangesArguments.join("\n")
                                           + "</pre>",
                                           QMessageBox::Apply | QMessageBox::Discard,
                                           QMessageBox::Apply);
        return answer == QMessageBox::Apply;
    }

    void XMakeBuildSystem::runXMake() {
        qCDebug(xmakeBuildSystemLog) << "Requesting parse due \"Run XMake\" command";
        reparse(REPARSE_FORCE_CMAKE_RUN | REPARSE_URGENT);
    }

    void XMakeBuildSystem::runXMakeAndScanProjectTree() {
        qCDebug(xmakeBuildSystemLog) << "Requesting parse due to \"Rescan Project\" command";
        reparse(REPARSE_FORCE_CMAKE_RUN | REPARSE_URGENT);
    }

    void XMakeBuildSystem::runXMakeWithExtraArguments() {
        qCDebug(xmakeBuildSystemLog) << "Requesting parse due to \"Rescan Project\" command";
        reparse(REPARSE_FORCE_CMAKE_RUN | REPARSE_FORCE_EXTRA_CONFIGURATION | REPARSE_URGENT);
    }

    void XMakeBuildSystem::runXMakeWithProfiling() {
        qCDebug(xmakeBuildSystemLog) << "Requesting parse due \"XMake Profiler\" command";
        reparse(REPARSE_FORCE_CMAKE_RUN | REPARSE_URGENT | REPARSE_FORCE_EXTRA_CONFIGURATION
                | REPARSE_PROFILING);
    }

    void XMakeBuildSystem::stopXMakeRun() {
        qCDebug(xmakeBuildSystemLog) << buildConfiguration()->displayName()
                                     << "stopping XMake's run";
        m_reader.stopXMakeRun();
    }

    void XMakeBuildSystem::buildXMakeTarget(const QString &buildTarget) {
        QTC_ASSERT(!buildTarget.isEmpty(), return );
        if (ProjectExplorerPlugin::saveModifiedFiles()) {
            xmakeBuildConfiguration()->buildTarget(buildTarget);
        }
    }

    bool XMakeBuildSystem::persistXMakeState() {
        BuildDirParameters parameters(this);
        QTC_ASSERT(parameters.isValid(), return false);

        const bool hadBuildDirectory = parameters.buildDirectory.exists();
        ensureBuildDirectory(parameters);

        int reparseFlags = REPARSE_DEFAULT;
        qCDebug(xmakeBuildSystemLog) << "Checking whether build system needs to be persisted:"
                                     << "buildDir:" << parameters.buildDirectory
                                     << "Has extraargs:" << !parameters.configurationChangesArguments.isEmpty();

        if (mustApplyConfigurationChangesArguments(parameters)) {
            reparseFlags = REPARSE_FORCE_EXTRA_CONFIGURATION;
            qCDebug(xmakeBuildSystemLog) << "   -> must run XMake with extra arguments.";
        }
        if (!hadBuildDirectory) {
            reparseFlags = REPARSE_FORCE_INITIAL_CONFIGURATION;
            qCDebug(xmakeBuildSystemLog) << "   -> must run XMake with initial arguments.";
        }

        if (reparseFlags == REPARSE_DEFAULT) {
            return false;
        }

        qCDebug(xmakeBuildSystemLog) << "Requesting parse to persist XMake State";
        setParametersAndRequestParse(parameters,
                                     REPARSE_URGENT | REPARSE_FORCE_CMAKE_RUN | reparseFlags);
        return true;
    }

    void XMakeBuildSystem::clearXMakeCache() {
        QTC_ASSERT(m_parameters.isValid(), return );
        QTC_ASSERT(!m_isHandlingError, return );

        stopParsingAndClearState();

        const FilePath pathsToDelete[] = {
            m_parameters.buildDirectory / Constants::CMAKE_CACHE_TXT,
            m_parameters.buildDirectory / Constants::CMAKE_CACHE_TXT_PREV,
            m_parameters.buildDirectory / "XMakeFiles",
            m_parameters.buildDirectory / ".xmake/api/v1/reply",
            m_parameters.buildDirectory / ".xmake/api/v1/reply.prev",
            m_parameters.buildDirectory / Constants::PACKAGE_MANAGER_DIR
        };

        for (const FilePath &path : pathsToDelete) {
            path.removeRecursively();
        }

        emit configurationCleared();
    }

    void XMakeBuildSystem::combineScanAndParse(bool restoredFromBackup) {
        if (buildConfiguration()->isActive()) {
            if (m_waitingForParse) {
                return;
            }

            if (m_combinedScanAndParseResult) {
                updateProjectData();
                m_currentGuard.markAsSuccess();

                if (restoredFromBackup) {
                    project()->addIssue(
                        XMakeProject::IssueType::Warning,
                        Tr::tr("<b>XMake configuration failed<b>"
                               "<p>The backup of the previous configuration has been restored.</p>"
                               "<p>Issues and \"Projects > Build\" settings "
                               "show more information about the failure.</p>"));
                }

                m_reader.resetData();

                m_currentGuard = {};
                m_testNames.clear();

                emitBuildSystemUpdated();

                runCTest();
            } else {
                updateFallbackProjectData();

                project()->addIssue(XMakeProject::IssueType::Warning,
                                    Tr::tr("<b>Failed to load project<b>"
                                           "<p>Issues and \"Projects > Build\" settings "
                                           "show more information about the failure.</p>"));
            }
        }
    }

    void XMakeBuildSystem::checkAndReportError(QString &errorMessage) {
        if (!errorMessage.isEmpty()) {
            setError(errorMessage);
            errorMessage.clear();
        }
    }

    static QSet<FilePath> projectFilesToWatch(const QSet<XMakeFileInfo> &xmakeFiles) {
        return Utils::transform(Utils::filtered(xmakeFiles,
                                                [](const XMakeFileInfo &info) {
                                                    return !info.isGenerated;
                                                }),
                                [](const XMakeFileInfo &info) {
                                    return info.path;
                                });
    }

    void XMakeBuildSystem::updateProjectData() {
        qCDebug(xmakeBuildSystemLog) << "Updating XMake project data";

        QTC_ASSERT(m_treeScanner.isFinished() && !m_reader.isParsing(), return );

        buildConfiguration()->project()->setExtraProjectFiles(projectFilesToWatch(m_xmakeFiles));

        XMakeConfig patchedConfig = configurationFromXMake();
        {
            QSet<QString> res;
            QStringList apps;
            for (const auto &target : std::as_const(m_buildTargets)) {
                if (target.targetType == DynamicLibraryType) {
                    res.insert(target.executable.parentDir().toString());
                    apps.push_back(target.executable.toUserOutput());
                }
                // ### shall we add also the ExecutableType ?
            }
            {
                XMakeConfigItem paths;
                paths.key = Android::Constants::ANDROID_SO_LIBS_PATHS;
                paths.values = Utils::toList(res);
                patchedConfig.append(paths);
            }

            apps.sort();
            {
                XMakeConfigItem appsPaths;
                appsPaths.key = "TARGETS_BUILD_PATH";
                appsPaths.values = apps;
                patchedConfig.append(appsPaths);
            }
        }

        Project *p = project();
        {
            auto newRoot = m_reader.rootProjectNode();
            if (newRoot) {
                setRootProjectNode(std::move(newRoot));

                if (QTC_GUARD(p->rootProjectNode())) {
                    const QString nodeName = p->rootProjectNode()->displayName();
                    p->setDisplayName(nodeName);

                    // set config on target nodes
                    const QSet<QString> buildKeys = Utils::transform<QSet>(m_buildTargets,
                                                                           &XMakeBuildTarget::title);
                    p->rootProjectNode()->forEachProjectNode(
                        [patchedConfig, buildKeys](const ProjectNode *node) {
                            if (buildKeys.contains(node->buildKey())) {
                                auto targetNode = const_cast<XMakeTargetNode *>(
                                    dynamic_cast<const XMakeTargetNode *>(node));
                                if (QTC_GUARD(targetNode)) {
                                    targetNode->setConfig(patchedConfig);
                                }
                            }
                        });
                }
            }
        }

        {
            qDeleteAll(m_extraCompilers);
            m_extraCompilers = findExtraCompilers();
            qCDebug(xmakeBuildSystemLog) << "Extra compilers created.";
        }

        QtSupport::CppKitInfo kitInfo(kit());
        QTC_ASSERT(kitInfo.isValid(), return );

        struct QtMajorToPkgNames {
            QtMajorVersion major = QtMajorVersion::None;
            QStringList pkgNames;
        };

        auto qtVersionFromXMake = [this](const QList<QtMajorToPkgNames> &mapping) {
            for (const QtMajorToPkgNames &m : mapping) {
                for (const QString &pkgName : m.pkgNames) {
                    auto qt = m_findPackagesFilesHash.value(pkgName);
                    if (qt.hasValidTarget()) {
                        return m.major;
                    }
                }
            }
            return QtMajorVersion::None;
        };

        QtMajorVersion qtVersion = kitInfo.projectPartQtVersion;
        if (qtVersion == QtMajorVersion::None) {
            qtVersion = qtVersionFromXMake({ { QtMajorVersion::Qt6, { "Qt6", "Qt6Core" } },
                                           { QtMajorVersion::Qt5, { "Qt5", "Qt5Core" } },
                                           { QtMajorVersion::Qt4, { "Qt4", "Qt4Core" } }
                                       });
        }

        QString errorMessage;
        RawProjectParts rpps = m_reader.createRawProjectParts(errorMessage);
        if (!errorMessage.isEmpty()) {
            setError(errorMessage);
        }
        qCDebug(xmakeBuildSystemLog) << "Raw project parts created." << errorMessage;

        for (RawProjectPart &rpp : rpps) {
            rpp.setQtVersion(qtVersion); // TODO: Check if project actually uses Qt.
            const FilePath includeFileBaseDir = buildConfiguration()->buildDirectory();
            QStringList cxxFlags = rpp.flagsForCxx.commandLineFlags;
            QStringList cFlags = rpp.flagsForC.commandLineFlags;
            addTargetFlagForIos(cFlags, cxxFlags, this, [this] {
                                    return m_configurationFromXMake.stringValueOf("CMAKE_OSX_DEPLOYMENT_TARGET");
                                });
            if (kitInfo.cxxToolchain) {
                rpp.setFlagsForCxx({ kitInfo.cxxToolchain, cxxFlags, includeFileBaseDir });
            }
            if (kitInfo.cToolchain) {
                rpp.setFlagsForC({ kitInfo.cToolchain, cFlags, includeFileBaseDir });
            }
        }

        m_cppCodeModelUpdater->update({ p, kitInfo, buildConfiguration()->environment(), rpps },
                                      m_extraCompilers);

        {
            const bool mergedHeaderPathsAndQmlImportPaths = kit()->value(
                QtSupport::Constants::KIT_HAS_MERGED_HEADER_PATHS_WITH_QML_IMPORT_PATHS, false).toBool();
            QStringList extraHeaderPaths;
            QList<QByteArray> moduleMappings;
            for (const RawProjectPart &rpp : std::as_const(rpps)) {
                FilePath moduleMapFile = buildConfiguration()->buildDirectory()
                    .pathAppended("qml_module_mappings/" + rpp.buildSystemTarget);
                if (expected_str<QByteArray> content = moduleMapFile.fileContents()) {
                    auto lines = content->split('\n');
                    for (const QByteArray &line : lines) {
                        if (!line.isEmpty()) {
                            moduleMappings.append(line.simplified());
                        }
                    }
                }

                if (mergedHeaderPathsAndQmlImportPaths) {
                    for (const auto &headerPath : rpp.headerPaths) {
                        if (headerPath.type == HeaderPathType::User || headerPath.type == HeaderPathType::System) {
                            extraHeaderPaths.append(headerPath.path);
                        }
                    }
                }
            }
            updateQmlJSCodeModel(extraHeaderPaths, moduleMappings);
        }
        updateInitialXMakeExpandableVars();

        emit buildConfiguration()->buildTypeChanged();

        qCDebug(xmakeBuildSystemLog) << "All XMake project data up to date.";
    }

    void XMakeBuildSystem::handleTreeScanningFinished() {
        TreeScanner::Result result = m_treeScanner.release();
        m_allFiles = result.folderNode;
        qDeleteAll(result.allFiles);

        updateFileSystemNodes();
    }

    void XMakeBuildSystem::updateFileSystemNodes() {
        auto newRoot = std::make_unique<XMakeProjectNode>(m_parameters.sourceDirectory);
        newRoot->setDisplayName(m_parameters.sourceDirectory.fileName());

        if (!m_reader.topCmakeFile().isEmpty()) {
            auto node = std::make_unique<FileNode>(m_reader.topCmakeFile(), FileType::Project);
            node->setIsGenerated(false);

            std::vector<std::unique_ptr<FileNode>> fileNodes;
            fileNodes.emplace_back(std::move(node));

            addXMakeLists(newRoot.get(), std::move(fileNodes));
        }

        if (m_allFiles) {
            addFileSystemNodes(newRoot.get(), m_allFiles);
        }
        setRootProjectNode(std::move(newRoot));

        m_reader.resetData();

        m_currentGuard = {};
        emitBuildSystemUpdated();

        qCDebug(xmakeBuildSystemLog) << "All fallback XMake project data up to date.";
    }

    void XMakeBuildSystem::updateFallbackProjectData() {
        qCDebug(xmakeBuildSystemLog) << "Updating fallback XMake project data";
        qCDebug(xmakeBuildSystemLog) << "Starting TreeScanner";
        QTC_CHECK(m_treeScanner.isFinished());
        if (m_treeScanner.asyncScanForFiles(projectDirectory())) {
            Core::ProgressManager::addTask(m_treeScanner.future(),
                                           Tr::tr("Scan \"%1\" project tree")
                                           .arg(project()->displayName()),
                                           "XMake.Scan.Tree");
        }
    }

    void XMakeBuildSystem::updateXMakeConfiguration(QString &errorMessage) {
        XMakeConfig xmakeConfig = m_reader.takeParsedConfiguration(errorMessage);
        for (auto &ci : xmakeConfig) {
            ci.inXMakeCache = true;
        }
        if (!errorMessage.isEmpty()) {
            const XMakeConfig changes = configurationChanges();
            for (const auto &ci : changes) {
                if (ci.isInitial) {
                    continue;
                }
                const bool haveConfigItem = Utils::contains(xmakeConfig, [ci](const XMakeConfigItem& i) {
                                                                return i.key == ci.key;
                                                            });
                if (!haveConfigItem) {
                    xmakeConfig.append(ci);
                }
            }
        }

        const bool hasAndroidTargetBuildDirSupport
            = XMakeConfigItem::toBool(
            xmakeConfig.stringValueOf("QT_INTERNAL_ANDROID_TARGET_BUILD_DIR_SUPPORT"))
                .value_or(false);

        const bool useAndroidTargetBuildDir
            = XMakeConfigItem::toBool(xmakeConfig.stringValueOf("QT_USE_TARGET_ANDROID_BUILD_DIR"))
                .value_or(false);

        project()->setExtraData(Android::Constants::AndroidBuildTargetDirSupport,
                                QVariant::fromValue(hasAndroidTargetBuildDirSupport));
        project()->setExtraData(Android::Constants::UseAndroidBuildTargetDir,
                                QVariant::fromValue(useAndroidTargetBuildDir));

        QVariantList packageTargets;
        for (const XMakeBuildTarget &buildTarget : buildTargets()) {
            bool isBuiltinPackage = false;
            bool isInstallablePackage = false;
            for (const ProjectExplorer::FolderNode::LocationInfo &bs : buildTarget.backtrace) {
                if (bs.displayName == "qt6_am_create_builtin_package") {
                    isBuiltinPackage = true;
                } else if (bs.displayName == "qt6_am_create_installable_package") {
                    isInstallablePackage = true;
                }
            }

            if (!isBuiltinPackage && !isInstallablePackage) {
                continue;
            }

            QVariantMap packageTarget;
            for (const FilePath &sourceFile : buildTarget.sourceFiles) {
                if (sourceFile.fileName() == "info.yaml") {
                    packageTarget.insert("manifestFilePath", QVariant::fromValue(sourceFile.absoluteFilePath()));
                    packageTarget.insert("xmakeTarget", buildTarget.title);
                    packageTarget.insert("isBuiltinPackage", isBuiltinPackage);
                    for (const FilePath &osf : buildTarget.sourceFiles) {
                        if (osf.fileName().endsWith(".ampkg.rule")) {
                            packageTarget.insert("packageFilePath", QVariant::fromValue(osf.absoluteFilePath().chopped(5)));
                        }
                    }
                }
            }
            packageTargets.append(packageTarget);
        }
        project()->setExtraData(AppManager::Constants::APPMAN_PACKAGE_TARGETS, packageTargets);

        setConfigurationFromXMake(xmakeConfig);
    }

    void XMakeBuildSystem::handleParsingSucceeded(bool restoredFromBackup) {
        if (!buildConfiguration()->isActive()) {
            stopParsingAndClearState();
            return;
        }

        clearError();

        QString errorMessage;
        {
            m_buildTargets = Utils::transform(XMakeBuildStep::specialTargets(m_reader.usesAllCapsTargets()), [this](const QString &t) {
                                                  XMakeBuildTarget result;
                                                  result.title = t;
                                                  result.workingDirectory = m_parameters.buildDirectory;
                                                  result.sourceDirectory = m_parameters.sourceDirectory;
                                                  return result;
                                              });
            m_buildTargets += m_reader.takeBuildTargets(errorMessage);
            m_xmakeFiles = m_reader.takeXMakeFileInfos(errorMessage);
            setupXMakeSymbolsHash();

            checkAndReportError(errorMessage);
        }

        {
            updateXMakeConfiguration(errorMessage);
            checkAndReportError(errorMessage);
        }

        if (const XMakeTool *tool = m_parameters.xmakeTool()) {
            m_ctestPath = tool->xmakeExecutable().withNewPath(m_reader.ctestPath());
        }

        setApplicationTargets(appTargets());

        // Note: This is practically always wrong and resulting in an empty view.
        // Setting the real data is triggered from a successful run of a
        // MakeInstallStep.
        setDeploymentData(deploymentDataFromFile());

        QTC_ASSERT(m_waitingForParse, return );
        m_waitingForParse = false;

        combineScanAndParse(restoredFromBackup);
    }

    void XMakeBuildSystem::handleParsingFailed(const QString &msg) {
        setError(msg);

        QString errorMessage;
        updateXMakeConfiguration(errorMessage);
        // ignore errorMessage here, we already got one.

        m_ctestPath.clear();

        QTC_CHECK(m_waitingForParse);
        m_waitingForParse = false;
        m_combinedScanAndParseResult = false;

        combineScanAndParse(false);
    }

    void XMakeBuildSystem::wireUpConnections() {
        // At this point the entire project will be fully configured, so let's connect everything and
        // trigger an initial parser run

        // Became active/inactive:
        connect(target(), &Target::activeBuildConfigurationChanged, this, [this] {
                    // Build configuration has changed:
                    qCDebug(xmakeBuildSystemLog) << "Requesting parse due to active BC changed";
                    reparse(XMakeBuildSystem::REPARSE_DEFAULT);
                });
        connect(project(), &Project::activeTargetChanged, this, [this] {
                    // Build configuration has changed:
                    qCDebug(xmakeBuildSystemLog) << "Requesting parse due to active target changed";
                    reparse(XMakeBuildSystem::REPARSE_DEFAULT);
                });

        // BuildConfiguration changed:
        connect(buildConfiguration(), &BuildConfiguration::environmentChanged, this, [this] {
                    // The environment on our BC has changed, force XMake run to catch up with possible changes
                    qCDebug(xmakeBuildSystemLog) << "Requesting parse due to environment change";
                    reparse(XMakeBuildSystem::REPARSE_FORCE_CMAKE_RUN);
                });
        connect(buildConfiguration(), &BuildConfiguration::buildDirectoryChanged, this, [this] {
                    // The build directory of our BC has changed:
                    // Does the directory contain a XMakeCache ? Existing build, just parse
                    // No XMakeCache? Run with initial arguments!
                    qCDebug(xmakeBuildSystemLog) << "Requesting parse due to build directory change";
                    const BuildDirParameters parameters(this);
                    const FilePath xmakeCacheTxt = parameters.buildDirectory.pathAppended(
                        Constants::CMAKE_CACHE_TXT);
                    const bool hasXMakeCache = xmakeCacheTxt.exists();
                    const auto options = ReparseParameters(
                        hasXMakeCache
                    ? REPARSE_DEFAULT
                    : (REPARSE_FORCE_INITIAL_CONFIGURATION | REPARSE_FORCE_CMAKE_RUN));
                    if (hasXMakeCache) {
                        QString errorMessage;
                        const XMakeConfig config = XMakeConfig::fromFile(xmakeCacheTxt, &errorMessage);
                        if (!config.isEmpty() && errorMessage.isEmpty()) {
                            QString xmakeBuildTypeName = config.stringValueOf("CMAKE_BUILD_TYPE");
                            xmakeBuildConfiguration()->setXMakeBuildType(xmakeBuildTypeName, true);
                        }
                    }
                    reparse(options);
                });

        connect(project(), &Project::projectFileIsDirty, this, [this] {
                    const bool isBuilding = BuildManager::isBuilding(project());
                    if (buildConfiguration()->isActive() && !isParsing() && !isBuilding) {
                        if (settings().autorunXMake()) {
                            qCDebug(xmakeBuildSystemLog) << "Requesting parse due to dirty project file";
                            reparse(XMakeBuildSystem::REPARSE_FORCE_CMAKE_RUN);
                        }
                    }
                });

        // Force initial parsing run:
        if (buildConfiguration()->isActive()) {
            qCDebug(xmakeBuildSystemLog) << "Initial run:";
            reparse(XMakeBuildSystem::REPARSE_DEFAULT);
        }
    }

    void XMakeBuildSystem::setupXMakeSymbolsHash() {
        m_xmakeSymbolsHash.clear();

        m_projectKeywords.functions.clear();
        m_projectKeywords.variables.clear();

        auto handleFunctionMacroOption = [&](const XMakeFileInfo &xmakeFile,
                                             const cmListFileFunction &func) {
            if (func.LowerCaseName() != "function" && func.LowerCaseName() != "macro"
                && func.LowerCaseName() != "option") {
                return;
            }

            if (func.Arguments().size() == 0) {
                return;
            }
            auto arg = func.Arguments()[0];

            Utils::Link link;
            link.targetFilePath = xmakeFile.path;
            link.targetLine = arg.Line;
            link.targetColumn = arg.Column - 1;
            m_xmakeSymbolsHash.insert(QString::fromUtf8(arg.Value), link);

            if (func.LowerCaseName() == "option") {
                m_projectKeywords.variables[QString::fromUtf8(arg.Value)] = FilePath();
            } else {
                m_projectKeywords.functions[QString::fromUtf8(arg.Value)] = FilePath();
            }
        };

        m_projectImportedTargets.clear();
        auto handleImportedTargets = [&](const XMakeFileInfo &xmakeFile,
                                         const cmListFileFunction &func) {
            if (func.LowerCaseName() != "add_library") {
                return;
            }

            if (func.Arguments().size() == 0) {
                return;
            }
            auto arg = func.Arguments()[0];
            const QString targetName = QString::fromUtf8(arg.Value);

            const bool haveImported = Utils::contains(func.Arguments(), [](const auto &arg) {
                                                          return arg.Value == "IMPORTED";
                                                      });
            if (haveImported && !targetName.contains("${")) {
                m_projectImportedTargets << targetName;

                // Allow navigation to the imported target
                Utils::Link link;
                link.targetFilePath = xmakeFile.path;
                link.targetLine = arg.Line;
                link.targetColumn = arg.Column - 1;
                m_xmakeSymbolsHash.insert(targetName, link);
            }
        };

        // Handle project targets, unfortunately the XMake file-api doesn't deliver the
        // column of the target, just the line. Make sure to find it out
        QHash<FilePath, QPair<int, QString>> projectTargetsSourceAndLine;
        for (const auto &target : std::as_const(buildTargets())) {
            if (target.targetType == TargetType::UtilityType) {
                continue;
            }
            if (target.backtrace.isEmpty()) {
                continue;
            }

            projectTargetsSourceAndLine.insert(target.backtrace.last().path,
                                               { target.backtrace.last().line, target.title });
        }
        auto handleProjectTargets = [&](const XMakeFileInfo &xmakeFile, const cmListFileFunction &func) {
            const auto it = projectTargetsSourceAndLine.find(xmakeFile.path);
            if (it == projectTargetsSourceAndLine.end() || it->first != func.Line()) {
                return;
            }

            if (func.Arguments().size() == 0) {
                return;
            }
            auto arg = func.Arguments()[0];

            Utils::Link link;
            link.targetFilePath = xmakeFile.path;
            link.targetLine = arg.Line;
            link.targetColumn = arg.Column - 1;
            m_xmakeSymbolsHash.insert(it->second, link);
        };

        // Gather the exported variables for the Find<Package> XMake packages
        m_projectFindPackageVariables.clear();

        const std::string fphsFunctionName = "find_package_handle_standard_args";
        XMakeKeywords keywords;
        if (auto tool = XMakeKitAspect::xmakeTool(target()->kit())) {
            keywords = tool->keywords();
        }
        QSet<std::string> fphsFunctionArgs;
        if (keywords.functionArgs.contains(QString::fromStdString(fphsFunctionName))) {
            const QList<std::string> args
                = Utils::transform(keywords.functionArgs.value(QString::fromStdString(fphsFunctionName)),
                                   &QString::toStdString);
            fphsFunctionArgs = Utils::toSet(args);
        }

        auto handleFindPackageVariables = [&](const XMakeFileInfo &xmakeFile, const cmListFileFunction &func) {
            if (func.LowerCaseName() != fphsFunctionName) {
                return;
            }

            if (func.Arguments().size() == 0) {
                return;
            }
            auto firstArgument = func.Arguments()[0];
            const auto filteredArguments = Utils::filtered(func.Arguments(), [&](const auto &arg) {
                                                               return !fphsFunctionArgs.contains(arg.Value) && arg != firstArgument;
                                                           });

            for (const auto &arg : filteredArguments) {
                const QString value = QString::fromUtf8(arg.Value);
                if (value.contains("${") || (value.startsWith('"') && value.endsWith('"'))
                    || (value.startsWith("'") && value.endsWith("'"))) {
                    continue;
                }

                m_projectFindPackageVariables << value;

                Utils::Link link;
                link.targetFilePath = xmakeFile.path;
                link.targetLine = arg.Line;
                link.targetColumn = arg.Column - 1;
                m_xmakeSymbolsHash.insert(value, link);
            }
        };

        // Prepare a hash with all .xmake files
        m_dotXMakeFilesHash.clear();
        auto handleDotXMakeFiles = [&](const XMakeFileInfo &xmakeFile) {
            if (xmakeFile.path.suffix() == "xmake") {
                Utils::Link link;
                link.targetFilePath = xmakeFile.path;
                link.targetLine = 1;
                link.targetColumn = 0;
                m_dotXMakeFilesHash.insert(xmakeFile.path.completeBaseName(), link);
            }
        };

        // Gather all Find<Package>.xmake and <Package>Config.xmake / <Package>-config.xmake files
        m_findPackagesFilesHash.clear();
        auto handleFindPackageXMakeFiles = [&](const XMakeFileInfo &xmakeFile) {
            const QString fileName = xmakeFile.path.fileName();

            const QString findPackageName = [fileName]() -> QString {
                auto findIdx = fileName.indexOf("Find");
                auto endsWithXMakeIdx = fileName.lastIndexOf(".xmake");
                if (findIdx == 0 && endsWithXMakeIdx > 0) {
                    return fileName.mid(4, endsWithXMakeIdx - 4);
                }
                return QString();
            }();

            const QString configPackageName = [fileName]() -> QString {
                auto configXMakeIdx = fileName.lastIndexOf("Config.xmake");
                if (configXMakeIdx > 0) {
                    return fileName.left(configXMakeIdx);
                }
                auto dashConfigXMakeIdx = fileName.lastIndexOf("-config.xmake");
                if (dashConfigXMakeIdx > 0) {
                    return fileName.left(dashConfigXMakeIdx);
                }
                return QString();
            }();

            if (!findPackageName.isEmpty() || !configPackageName.isEmpty()) {
                Utils::Link link;
                link.targetFilePath = xmakeFile.path;
                link.targetLine = 1;
                link.targetColumn = 0;
                m_findPackagesFilesHash.insert(!findPackageName.isEmpty() ? findPackageName
                                                                      : configPackageName,
                                               link);
            }
        };

        for (const auto &xmakeFile : std::as_const(m_xmakeFiles)) {
            for (const auto &func : xmakeFile.xmakeListFile.Functions) {
                handleFunctionMacroOption(xmakeFile, func);
                handleImportedTargets(xmakeFile, func);
                handleProjectTargets(xmakeFile, func);
                handleFindPackageVariables(xmakeFile, func);
            }
            handleDotXMakeFiles(xmakeFile);
            handleFindPackageXMakeFiles(xmakeFile);
        }

        m_projectFindPackageVariables.removeDuplicates();
    }

    void XMakeBuildSystem::ensureBuildDirectory(const BuildDirParameters &parameters) {
        const FilePath bdir = parameters.buildDirectory;

        if (!buildConfiguration()->createBuildDirectory()) {
            handleParsingFailed(Tr::tr("Failed to create build directory \"%1\".").arg(bdir.toUserOutput()));
            return;
        }

        const XMakeTool *tool = parameters.xmakeTool();
        if (!tool) {
            handleParsingFailed(Tr::tr("No XMake tool set up in kit."));
            return;
        }

        if (tool->xmakeExecutable().needsDevice()) {
            if (!tool->xmakeExecutable().ensureReachable(bdir)) {
                // Make sure that the build directory is available on the device.
                handleParsingFailed(
                    Tr::tr("The remote XMake executable cannot write to the local build directory."));
            }
        }
    }

    void XMakeBuildSystem::stopParsingAndClearState() {
        qCDebug(xmakeBuildSystemLog) << buildConfiguration()->displayName()
                                     << "stopping parsing run!";
        m_reader.stop();
        m_reader.resetData();
    }

    void XMakeBuildSystem::becameDirty() {
        qCDebug(xmakeBuildSystemLog) << "XMakeBuildSystem: becameDirty was triggered.";
        if (isParsing()) {
            return;
        }

        reparse(REPARSE_DEFAULT);
    }

    void XMakeBuildSystem::updateReparseParameters(const int parameters) {
        m_reparseParameters |= parameters;
    }

    int XMakeBuildSystem::takeReparseParameters() {
        int result = m_reparseParameters;
        m_reparseParameters = REPARSE_DEFAULT;
        return result;
    }

    void XMakeBuildSystem::runCTest() {
        if (!m_error.isEmpty() || m_ctestPath.isEmpty()) {
            qCDebug(xmakeBuildSystemLog) << "Cancel ctest run after failed xmake run";
            emit testInformationUpdated();
            return;
        }
        qCDebug(xmakeBuildSystemLog) << "Requesting ctest run after xmake run";

        const BuildDirParameters parameters(this);
        QTC_ASSERT(parameters.isValid(), return );

        ensureBuildDirectory(parameters);
        m_ctestProcess.reset(new Process);
        m_ctestProcess->setEnvironment(buildConfiguration()->environment());
        m_ctestProcess->setWorkingDirectory(parameters.buildDirectory);
        m_ctestProcess->setCommand({ m_ctestPath, { "-N", "--show-only=json-v1" } });
        connect(m_ctestProcess.get(), &Process::done, this, [this] {
                    if (m_ctestProcess->result() == ProcessResult::FinishedWithSuccess) {
                        const QJsonDocument json = QJsonDocument::fromJson(m_ctestProcess->rawStdOut());
                        if (!json.isEmpty() && json.isObject()) {
                            const QJsonObject jsonObj = json.object();
                            const QJsonObject btGraph = jsonObj.value("backtraceGraph").toObject();
                            const QJsonArray xmakelists = btGraph.value("files").toArray();
                            const QJsonArray nodes = btGraph.value("nodes").toArray();
                            const QJsonArray tests = jsonObj.value("tests").toArray();
                            int counter = 0;
                            for (const auto &testVal : tests) {
                                ++counter;
                                const QJsonObject test = testVal.toObject();
                                QTC_ASSERT(!test.isEmpty(), continue);
                                int file = -1;
                                int line = -1;
                                const int bt = test.value("backtrace").toInt(-1);
                                // we may have no real backtrace due to different registering
                                if (bt != -1) {
                                    QSet<int> seen;
                                    std::function<QJsonObject(int)> findAncestor = [&](int index) {
                                        QJsonObject node = nodes.at(index).toObject();
                                        const int parent = node.value("parent").toInt(-1);
                                        if (parent < 0 || !Utils::insert(seen, parent)) {
                                            return node;
                                        }
                                        return findAncestor(parent);
                                    };
                                    const QJsonObject btRef = findAncestor(bt);
                                    file = btRef.value("file").toInt(-1);
                                    line = btRef.value("line").toInt(-1);
                                }
                                // we may have no XMakeLists.txt file reference due to different registering
                                const FilePath xmakeFile = file != -1
                            ? FilePath::fromString(xmakelists.at(file).toString()) : FilePath();
                                m_testNames.append({ test.value("name").toString(), counter, xmakeFile, line });
                            }
                        }
                    }
                    emit testInformationUpdated();
                });
        m_ctestProcess->start();
    }

    XMakeBuildConfiguration *XMakeBuildSystem::xmakeBuildConfiguration() const {
        return static_cast<XMakeBuildConfiguration *>(BuildSystem::buildConfiguration());
    }

    static FilePaths librarySearchPaths(const XMakeBuildSystem *bs, const QString &buildKey) {
        const XMakeBuildTarget xmakeBuildTarget
            = Utils::findOrDefault(bs->buildTargets(), [buildKey](const auto &target) {
                                       return target.title == buildKey && target.targetType != UtilityType;
                                   });

        return xmakeBuildTarget.libraryDirectories;
    }

    const QList<BuildTargetInfo> XMakeBuildSystem::appTargets() const {
        QList<BuildTargetInfo> appTargetList;
        const bool forAndroid = DeviceTypeKitAspect::deviceTypeId(kit())
            == Android::Constants::ANDROID_DEVICE_TYPE;
        for (const XMakeBuildTarget &ct : m_buildTargets) {
            if (XMakeBuildSystem::filteredOutTarget(ct)) {
                continue;
            }

            if (ct.targetType == ExecutableType || (forAndroid && ct.targetType == DynamicLibraryType)) {
                const QString buildKey = ct.title;

                BuildTargetInfo bti;
                bti.displayName = ct.title;
                bti.targetFilePath = ct.executable;
                bti.projectFilePath = ct.sourceDirectory.cleanPath();
                bti.workingDirectory = ct.workingDirectory;
                bti.buildKey = buildKey;
                bti.usesTerminal = !ct.linksToQtGui;
                bti.isQtcRunnable = ct.qtcRunnable;

                // Workaround for QTCREATORBUG-19354:
                bti.runEnvModifier = [this, buildKey](Environment &env, bool enabled) {
                    if (enabled) {
                        env.prependOrSetLibrarySearchPaths(librarySearchPaths(this, buildKey));
                    }
                };

                appTargetList.append(bti);
            }
        }

        return appTargetList;
    }

    QStringList XMakeBuildSystem::buildTargetTitles() const {
        auto nonAutogenTargets = filtered(m_buildTargets, [](const XMakeBuildTarget &target) {
                                              return !XMakeBuildSystem::filteredOutTarget(target);
                                          });
        return transform(nonAutogenTargets, &XMakeBuildTarget::title);
    }

    const QList<XMakeBuildTarget> &XMakeBuildSystem::buildTargets() const {
        return m_buildTargets;
    }

    bool XMakeBuildSystem::filteredOutTarget(const XMakeBuildTarget &target) {
        return target.title.endsWith("_autogen") ||
               target.title.endsWith("_autogen_timestamp_deps");
    }

    bool XMakeBuildSystem::isMultiConfig() const {
        return m_isMultiConfig;
    }

    void XMakeBuildSystem::setIsMultiConfig(bool isMultiConfig) {
        m_isMultiConfig = isMultiConfig;
    }

    bool XMakeBuildSystem::isMultiConfigReader() const {
        return m_reader.isMultiConfig();
    }

    bool XMakeBuildSystem::usesAllCapsTargets() const {
        return m_reader.usesAllCapsTargets();
    }

    XMakeProject *XMakeBuildSystem::project() const {
        return static_cast<XMakeProject *>(ProjectExplorer::BuildSystem::project());
    }

    const QList<TestCaseInfo> XMakeBuildSystem::testcasesInfo() const {
        return m_testNames;
    }

    CommandLine XMakeBuildSystem::commandLineForTests(const QList<QString> &tests,
                                                      const QStringList &options) const {
        QStringList args = options;
        const QSet<QString> testsSet = Utils::toSet(tests);
        auto current = Utils::transform<QSet<QString>>(m_testNames, &TestCaseInfo::name);
        if (tests.isEmpty() || current == testsSet) {
            return { m_ctestPath, args };
        }

        QString testNumbers("0,0,0"); // start, end, stride
        for (const TestCaseInfo &info : m_testNames) {
            if (testsSet.contains(info.name)) {
                testNumbers += QString(",%1").arg(info.number);
            }
        }
        args << "-I" << testNumbers;
        return { m_ctestPath, args };
    }

    DeploymentData XMakeBuildSystem::deploymentDataFromFile() const {
        DeploymentData result;

        FilePath sourceDir = project()->projectDirectory();
        FilePath buildDir = buildConfiguration()->buildDirectory();

        QString deploymentPrefix;
        FilePath deploymentFilePath = sourceDir.pathAppended("QtCreatorDeployment.txt");

        bool hasDeploymentFile = deploymentFilePath.exists();
        if (!hasDeploymentFile) {
            deploymentFilePath = buildDir.pathAppended("QtCreatorDeployment.txt");
            hasDeploymentFile = deploymentFilePath.exists();
        }
        if (!hasDeploymentFile) {
            return result;
        }

        deploymentPrefix = result.addFilesFromDeploymentFile(deploymentFilePath, sourceDir);
        for (const XMakeBuildTarget &ct : m_buildTargets) {
            if (ct.targetType == ExecutableType || ct.targetType == DynamicLibraryType) {
                if (!ct.executable.isEmpty()
                    && result.deployableForLocalFile(ct.executable).localFilePath() != ct.executable) {
                    result.addFile(ct.executable,
                                   deploymentPrefix + buildDir.relativeChildPath(ct.executable).toString(),
                                   DeployableFile::TypeExecutable);
                }
            }
        }

        return result;
    }

    QList<ExtraCompiler *> XMakeBuildSystem::findExtraCompilers() {
        qCDebug(xmakeBuildSystemLog) << "Finding Extra Compilers: start.";

        QList<ExtraCompiler *> extraCompilers;
        const QList<ExtraCompilerFactory *> factories = ExtraCompilerFactory::extraCompilerFactories();

        qCDebug(xmakeBuildSystemLog) << "Finding Extra Compilers: Got factories.";

        const QSet<QString> fileExtensions = Utils::transform<QSet>(factories,
                                                                    &ExtraCompilerFactory::sourceTag);

        qCDebug(xmakeBuildSystemLog) << "Finding Extra Compilers: Got file extensions:"
                                     << fileExtensions;

        // Find all files generated by any of the extra compilers, in a rather crude way.
        Project *p = project();
        const FilePaths fileList = p->files([&fileExtensions](const Node *n) {
                                                if (!Project::SourceFiles(n) || !n->isEnabled()) { // isEnabled excludes nodes from the file system tree
                                                    return false;
                                                }
                                                const QString suffix = n->filePath().suffix();
                                                return !suffix.isEmpty() && fileExtensions.contains(suffix);
                                            });

        qCDebug(xmakeBuildSystemLog) << "Finding Extra Compilers: Got list of files to check.";

        // Generate the necessary information:
        for (const FilePath &file : fileList) {
            qCDebug(xmakeBuildSystemLog)
                << "Finding Extra Compilers: Processing" << file.toUserOutput();
            ExtraCompilerFactory *factory = Utils::findOrDefault(factories,
                                                                 [&file](const ExtraCompilerFactory *f) {
                                                                     return file.endsWith(
                                                                         '.' + f->sourceTag());
                                                                 });
            QTC_ASSERT(factory, continue);

            FilePaths generated = filesGeneratedFrom(file);
            qCDebug(xmakeBuildSystemLog)
                << "Finding Extra Compilers:     generated files:" << generated;
            if (generated.isEmpty()) {
                continue;
            }

            extraCompilers.append(factory->create(p, file, generated));
            qCDebug(xmakeBuildSystemLog)
                << "Finding Extra Compilers:     done with" << file.toUserOutput();
        }

        qCDebug(xmakeBuildSystemLog) << "Finding Extra Compilers: done.";

        return extraCompilers;
    }

    void XMakeBuildSystem::updateQmlJSCodeModel(const QStringList &extraHeaderPaths,
                                                const QList<QByteArray> &moduleMappings) {
        QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();

        if (!modelManager) {
            return;
        }

        Project *p = project();
        QmlJS::ModelManagerInterface::ProjectInfo projectInfo
            = modelManager->defaultProjectInfoForProject(p, p->files(Project::HiddenRccFolders));

        projectInfo.importPaths.clear();

        auto addImports = [&projectInfo](const QString &imports) {
            const QStringList importList = XMakeConfigItem::xmakeSplitValue(imports);
            for (const QString &import : importList) {
                projectInfo.importPaths.maybeInsert(FilePath::fromUserInput(import), QmlJS::Dialect::Qml);
            }
        };

        const XMakeConfig &cm = configurationFromXMake();
        addImports(cm.stringValueOf("QML_IMPORT_PATH"));
        addImports(kit()->value(QtSupport::Constants::KIT_QML_IMPORT_PATH).toString());

        for (const QString &extraHeaderPath : extraHeaderPaths) {
            projectInfo.importPaths.maybeInsert(FilePath::fromString(extraHeaderPath),
                                                QmlJS::Dialect::Qml);
        }

        for (const QByteArray &mm : moduleMappings) {
            auto kvPair = mm.split('=');
            if (kvPair.size() != 2) {
                continue;
            }
            QString from = QString::fromUtf8(kvPair.at(0).trimmed());
            QString to = QString::fromUtf8(kvPair.at(1).trimmed());
            if (!from.isEmpty() && !to.isEmpty() && from != to) {
                // The QML code-model does not support sub-projects, so if there are multiple mappings for a single module,
                // choose the shortest one.
                if (projectInfo.moduleMappings.contains(from)) {
                    if (to.size() < projectInfo.moduleMappings.value(from).size()) {
                        projectInfo.moduleMappings.insert(from, to);
                    }
                } else {
                    projectInfo.moduleMappings.insert(from, to);
                }
            }
        }

        project()->setProjectLanguage(ProjectExplorer::Constants::QMLJS_LANGUAGE_ID,
                                      !projectInfo.sourceFiles.isEmpty());
        modelManager->updateProjectInfo(projectInfo, p);
    }

    void XMakeBuildSystem::updateInitialXMakeExpandableVars() {
        const XMakeConfig &cm = configurationFromXMake();
        const XMakeConfig &initialConfig =
            xmakeBuildConfiguration()->initialXMakeArguments.xmakeConfiguration();

        XMakeConfig config;

        const FilePath projectDirectory = project()->projectDirectory();
        const auto samePath = [projectDirectory](const FilePath &first, const FilePath &second) {
            // if a path is relative, resolve it relative to the project directory
            // this is not 100% correct since XMake resolve them to CMAKE_CURRENT_SOURCE_DIR
            // depending on context, but we cannot do better here
            return first == second
                   || projectDirectory.resolvePath(first)
                   == projectDirectory.resolvePath(second)
                   || projectDirectory.resolvePath(first).canonicalPath()
                   == projectDirectory.resolvePath(second).canonicalPath();
        };

        // Replace path values that do not  exist on file system
        const QByteArrayList singlePathList = {
            "CMAKE_C_COMPILER",
            "CMAKE_CXX_COMPILER",
            "QT_QMAKE_EXECUTABLE",
            "QT_HOST_PATH",
            "CMAKE_TOOLCHAIN_FILE"
        };
        for (const auto &var : singlePathList) {
            auto it = std::find_if(cm.cbegin(), cm.cend(), [var](const XMakeConfigItem &item) {
                                       return item.key == var && !item.isInitial;
                                   });

            if (it != cm.cend()) {
                const QByteArray initialValue = initialConfig.expandedValueOf(kit(), var).toUtf8();
                const FilePath initialPath = FilePath::fromUserInput(QString::fromUtf8(initialValue));
                const FilePath path = FilePath::fromUserInput(QString::fromUtf8(it->value));

                if (!initialValue.isEmpty() && !samePath(path, initialPath) && !path.exists()) {
                    XMakeConfigItem item(*it);
                    item.value = initialValue;

                    config << item;
                }
            }
        }

        // Prepend new values to existing path lists
        const QByteArrayList multiplePathList = {
            "CMAKE_PREFIX_PATH",
            "CMAKE_FIND_ROOT_PATH"
        };
        for (const auto &var : multiplePathList) {
            auto it = std::find_if(cm.cbegin(), cm.cend(), [var](const XMakeConfigItem &item) {
                                       return item.key == var && !item.isInitial;
                                   });

            if (it != cm.cend()) {
                const QByteArrayList initialValueList = initialConfig.expandedValueOf(kit(), var).toUtf8().split(';');

                for (const auto &initialValue: initialValueList) {
                    const FilePath initialPath = FilePath::fromUserInput(QString::fromUtf8(initialValue));

                    const bool pathIsContained
                        = Utils::contains(it->value.split(';'), [samePath, initialPath](const QByteArray &p) {
                                              return samePath(FilePath::fromUserInput(QString::fromUtf8(p)), initialPath);
                                          });
                    if (!initialValue.isEmpty() && !pathIsContained) {
                        XMakeConfigItem item(*it);
                        item.value = initialValue;
                        item.value.append(";");
                        item.value.append(it->value);

                        config << item;
                    }
                }
            }
        }

        if (!config.isEmpty()) {
            emit configurationChanged(config);
        }
    }

    MakeInstallCommand XMakeBuildSystem::makeInstallCommand(const FilePath &installRoot) const {
        MakeInstallCommand cmd;
        if (XMakeTool *tool = XMakeKitAspect::xmakeTool(target()->kit())) {
            cmd.command.setExecutable(tool->xmakeExecutable());
        }

        QString installTarget = "install";
        if (usesAllCapsTargets()) {
            installTarget = "INSTALL";
        }

        FilePath buildDirectory = ".";
        if (auto bc = buildConfiguration()) {
            buildDirectory = bc->buildDirectory();
        }

        cmd.command.addArg("--build");
        cmd.command.addArg(XMakeToolManager::mappedFilePath(buildDirectory).path());
        cmd.command.addArg("--target");
        cmd.command.addArg(installTarget);

        if (isMultiConfigReader()) {
            cmd.command.addArgs({ "--config", xmakeBuildType() });
        }

        cmd.environment.set("DESTDIR", installRoot.nativePath());
        return cmd;
    }

    QList<QPair<Id, QString>> XMakeBuildSystem::generators() const {
        if (!buildConfiguration()) {
            return {};
        }
        const XMakeTool * const xmakeTool
            = XMakeKitAspect::xmakeTool(buildConfiguration()->target()->kit());
        if (!xmakeTool) {
            return {};
        }

        QList<QPair<Id, QString>> result;
        const QList<XMakeTool::Generator> &generators = xmakeTool->supportedGenerators();
        for (const XMakeTool::Generator &generator : generators) {
            result << qMakePair(Id::fromSetting(generator.name),
                                Tr::tr("%1 (via xmake)").arg(generator.name));
        }
        return result;
    }

    void XMakeBuildSystem::runGenerator(Id id) {
        QTC_ASSERT(xmakeBuildConfiguration(), return );
        const auto showError = [](const QString &detail) {
            Core::MessageManager::writeDisrupting(
                addXMakePrefix(Tr::tr("xmake generator failed: %1.").arg(detail)));
        };
        const XMakeTool * const xmakeTool
            = XMakeKitAspect::xmakeTool(buildConfiguration()->target()->kit());
        if (!xmakeTool) {
            showError(Tr::tr("Kit does not have a xmake binary set."));
            return;
        }
        const QString generator = id.toSetting().toString();
        const FilePath outDir = buildConfiguration()->buildDirectory()
            / ("qtc_" + FileUtils::fileSystemFriendlyName(generator));
        if (!outDir.ensureWritableDir()) {
            showError(Tr::tr("Cannot create output directory \"%1\".").arg(outDir.toString()));
            return;
        }
        CommandLine cmdLine(xmakeTool->xmakeExecutable(), { "-S", buildConfiguration()->target()
                                                            ->project()->projectDirectory().toUserOutput(), "-G", generator });
        if (!cmdLine.executable().isExecutableFile()) {
            showError(Tr::tr("No valid xmake executable."));
            return;
        }
        const auto itemFilter = [](const XMakeConfigItem &item) {
            return !item.isNull()
                   && item.type != XMakeConfigItem::STATIC
                   && item.type != XMakeConfigItem::INTERNAL
                   && !item.key.contains("GENERATOR");
        };
        QList<XMakeConfigItem> configItems = Utils::filtered(m_configurationChanges.toList(),
                                                             itemFilter);
        const QList<XMakeConfigItem> initialConfigItems
            = Utils::filtered(xmakeBuildConfiguration()->initialXMakeArguments.xmakeConfiguration().toList(),
                              itemFilter);
        for (const XMakeConfigItem &item : std::as_const(initialConfigItems)) {
            if (!Utils::contains(configItems, [&item](const XMakeConfigItem &existingItem) {
                                     return existingItem.key == item.key;
                                 })) {
                configItems << item;
            }
        }
        for (const XMakeConfigItem &item : std::as_const(configItems)) {
            cmdLine.addArg(item.toArgument(buildConfiguration()->macroExpander()));
        }

        cmdLine.addArgs(xmakeBuildConfiguration()->additionalXMakeOptions(), CommandLine::Raw);

        const auto proc = new Process(this);
        connect(proc, &Process::done, proc, &Process::deleteLater);
        connect(proc, &Process::readyReadStandardOutput, this, [proc] {
                    Core::MessageManager::writeFlashing(
                        addXMakePrefix(QString::fromLocal8Bit(proc->readAllRawStandardOutput()).split('\n')));
                });
        connect(proc, &Process::readyReadStandardError, this, [proc] {
                    Core::MessageManager::writeDisrupting(
                        addXMakePrefix(QString::fromLocal8Bit(proc->readAllRawStandardError()).split('\n')));
                });
        proc->setWorkingDirectory(outDir);
        proc->setEnvironment(buildConfiguration()->environment());
        proc->setCommand(cmdLine);
        Core::MessageManager::writeFlashing(addXMakePrefix(
            Tr::tr("Running in \"%1\": %2.").arg(outDir.toUserOutput(), cmdLine.toUserOutput())));
        proc->start();
    }

    ExtraCompiler *XMakeBuildSystem::findExtraCompiler(const ExtraCompilerFilter &filter) const {
        return Utils::findOrDefault(m_extraCompilers, filter);
    }
} // XMakeProjectManager::Internal
