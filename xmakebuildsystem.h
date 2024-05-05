// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "builddirparameters.h"
#include "xmakebuildtarget.h"
#include "fileapireader.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsystem.h>

#include <utils/temporarydirectory.h>

namespace ProjectExplorer {
    class ExtraCompiler;
    class FolderNode;
    class ProjectUpdater;
}

namespace Utils {
    class Process;
    class Link;
}

namespace XMakeProjectManager {
    class XMakeBuildConfiguration;
    class XMakeProject;

    namespace Internal {
// --------------------------------------------------------------------
// XMakeBuildSystem:
// --------------------------------------------------------------------

        class XMakeBuildSystem final : public ProjectExplorer::BuildSystem {
            Q_OBJECT
public:
            explicit XMakeBuildSystem(XMakeBuildConfiguration *bc);
            ~XMakeBuildSystem() final;

            void triggerParsing() final;
            void requestDebugging() final;

            bool supportsAction(ProjectExplorer::Node *context,
                                ProjectExplorer::ProjectAction action,
                                const ProjectExplorer::Node *node) const final;

            bool addFiles(ProjectExplorer::Node *context,
                          const Utils::FilePaths &filePaths, Utils::FilePaths *) final;

            ProjectExplorer::RemovedFilesFromProject removeFiles(ProjectExplorer::Node *context,
                                                                 const Utils::FilePaths &filePaths,
                                                                 Utils::FilePaths *notRemoved
                                                                 = nullptr) final;

            bool canRenameFile(ProjectExplorer::Node *context,
                               const Utils::FilePath &oldFilePath,
                               const Utils::FilePath &newFilePath) final;
            bool renameFile(ProjectExplorer::Node *context,
                            const Utils::FilePath &oldFilePath,
                            const Utils::FilePath &newFilePath) final;

            Utils::FilePaths filesGeneratedFrom(const Utils::FilePath &sourceFile) const final;
            QString name() const final {
                return QLatin1String("xmake");
            }

            // Actions:
            void runXMake();
            void runXMakeAndScanProjectTree();
            void runXMakeWithExtraArguments();
            void runXMakeWithProfiling();
            void stopXMakeRun();

            bool persistXMakeState();
            void clearXMakeCache();

            // Context menu actions:
            void buildXMakeTarget(const QString &buildTarget);

            // Queries:
            const QList<ProjectExplorer::BuildTargetInfo> appTargets() const;
            QStringList buildTargetTitles() const;
            const QList<XMakeBuildTarget> &buildTargets() const;
            ProjectExplorer::DeploymentData deploymentDataFromFile() const;

            XMakeBuildConfiguration *xmakeBuildConfiguration() const;

            QList<ProjectExplorer::TestCaseInfo> const testcasesInfo() const final;
            Utils::CommandLine commandLineForTests(const QList<QString> &tests,
                                                   const QStringList &options) const final;

            ProjectExplorer::MakeInstallCommand makeInstallCommand(
                const Utils::FilePath &installRoot) const final;

            static bool filteredOutTarget(const XMakeBuildTarget &target);

            bool isMultiConfig() const;
            void setIsMultiConfig(bool isMultiConfig);

            bool isMultiConfigReader() const;
            bool usesAllCapsTargets() const;

            XMakeProject *project() const;

            QString xmakeBuildType() const;
            ProjectExplorer::BuildConfiguration::BuildType buildType() const;

            XMakeConfig configurationFromXMake() const;
            XMakeConfig configurationChanges() const;

            QStringList configurationChangesArguments(bool initialParameters = false) const;

            void setConfigurationFromXMake(const XMakeConfig &config);
            void setConfigurationChanges(const XMakeConfig &config);

            QString error() const;
            QString warning() const;

            const QHash<QString, Utils::Link> &xmakeSymbolsHash() const {
                return m_xmakeSymbolsHash;
            }
            XMakeKeywords projectKeywords() const {
                return m_projectKeywords;
            }
            QStringList projectImportedTargets() const {
                return m_projectImportedTargets;
            }
            QStringList projectFindPackageVariables() const {
                return m_projectFindPackageVariables;
            }
            const QHash<QString, Utils::Link> &dotXMakeFilesHash() const {
                return m_dotXMakeFilesHash;
            }
            const QHash<QString, Utils::Link> &findPackagesFilesHash() const {
                return m_findPackagesFilesHash;
            }

signals:
            void configurationCleared();
            void configurationChanged(const XMakeConfig &config);
            void errorOccurred(const QString &message);
            void warningOccurred(const QString &message);

private:
            XMakeConfig initialXMakeConfiguration() const;

            QList<QPair<Utils::Id, QString>> generators() const override;
            void runGenerator(Utils::Id id) override;
            ProjectExplorer::ExtraCompiler *findExtraCompiler(
                const ExtraCompilerFilter &filter) const override;

            enum ForceEnabledChanged { False, True };
            void clearError(ForceEnabledChanged fec = ForceEnabledChanged::False);

            void setError(const QString &message);
            void setWarning(const QString &message);

            bool addSrcFiles(ProjectExplorer::Node *context, const Utils::FilePaths &filePaths,
                             Utils::FilePaths *);
            bool addTsFiles(ProjectExplorer::Node *context, const Utils::FilePaths &filePaths,
                            Utils::FilePaths *);

            // Actually ask for parsing:
            enum ReparseParameters {
                REPARSE_DEFAULT = 0, // Nothing special:-)
                REPARSE_FORCE_XMAKE_RUN
                    = (1 << 0), // Force xmake to run, apply extraXMakeArguments if non-empty
                REPARSE_FORCE_INITIAL_CONFIGURATION
                    = (1 << 1), // Force initial configuration arguments to xmake
                REPARSE_FORCE_EXTRA_CONFIGURATION = (1 << 2), // Force extra configuration arguments to xmake
                REPARSE_URGENT = (1 << 3),            // Do not delay the parser run by 1s
                REPARSE_DEBUG = (1 << 4),             // Start with debugging
                REPARSE_PROFILING = (1 << 5),         // Start profiling
            };
            void reparse(int reparseParameters);
            QString reparseParametersString(int reparseFlags);
            void setParametersAndRequestParse(const BuildDirParameters &parameters,
                                              const int reparseParameters);

            bool mustApplyConfigurationChangesArguments(const BuildDirParameters &parameters) const;

            // State handling:
            // Parser states:
            void handleParsingSuccess();
            void handleParsingError();

            // Treescanner states:
            void handleTreeScanningFinished();

            // Combining Treescanner and Parser states:
            void combineScanAndParse(bool restoredFromBackup);
            void checkAndReportError(QString &errorMessage);

            void updateXMakeConfiguration(QString &errorMessage);

            void updateProjectData();
            void updateFallbackProjectData();
            QList<ProjectExplorer::ExtraCompiler *> findExtraCompilers();
            void updateQmlJSCodeModel(const QStringList &extraHeaderPaths,
                                      const QList<QByteArray> &moduleMappings);
            void updateInitialXMakeExpandableVars();

            void updateFileSystemNodes();

            void handleParsingSucceeded(bool restoredFromBackup);
            void handleParsingFailed(const QString &msg);

            void wireUpConnections();

            void ensureBuildDirectory(const BuildDirParameters &parameters);
            void stopParsingAndClearState();
            void becameDirty();

            void updateReparseParameters(const int parameters);
            int takeReparseParameters();

            void runCTest();

            void setupXMakeSymbolsHash();

            struct ProjectFileArgumentPosition {
                cmListFileArgument argumentPosition;
                Utils::FilePath xmakeFile;
                QString relativeFileName;
                bool fromGlobbing = false;
            };
            std::optional<ProjectFileArgumentPosition> projectFileArgumentPosition(
                const QString &targetName, const QString &fileName);

            ProjectExplorer::TreeScanner m_treeScanner;
            std::shared_ptr<ProjectExplorer::FolderNode> m_allFiles;
            QHash<QString, bool> m_mimeBinaryCache;

            bool m_waitingForParse = false;
            bool m_combinedScanAndParseResult = false;

            bool m_isMultiConfig = false;

            ParseGuard m_currentGuard;

            ProjectExplorer::ProjectUpdater *m_cppCodeModelUpdater = nullptr;
            QList<ProjectExplorer::ExtraCompiler *> m_extraCompilers;
            QList<XMakeBuildTarget> m_buildTargets;
            QSet<XMakeFileInfo> m_xmakeFiles;
            QHash<QString, Utils::Link> m_xmakeSymbolsHash;
            QHash<QString, Utils::Link> m_dotXMakeFilesHash;
            QHash<QString, Utils::Link> m_findPackagesFilesHash;
            XMakeKeywords m_projectKeywords;
            QStringList m_projectImportedTargets;
            QStringList m_projectFindPackageVariables;

            QHash<QString, ProjectFileArgumentPosition> m_filesToBeRenamed;

            // Parsing state:
            BuildDirParameters m_parameters;
            int m_reparseParameters = REPARSE_DEFAULT;
            FileApiReader m_reader;
            mutable bool m_isHandlingError = false;

            // CTest integration
            Utils::FilePath m_ctestPath;
            std::unique_ptr<Utils::Process> m_ctestProcess;
            QList<ProjectExplorer::TestCaseInfo> m_testNames;

            XMakeConfig m_configurationFromXMake;
            XMakeConfig m_configurationChanges;

            QString m_error;
            QString m_warning;
        };
    } // namespace Internal
} // namespace XMakeProjectManager
