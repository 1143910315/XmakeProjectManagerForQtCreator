// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "builddirparameters.h"
#include "xmakebuildtarget.h"
#include "xmakeprojectnodes.h"
#include "fileapidataextractor.h"

#include <projectexplorer/rawprojectpart.h>
#include <projectexplorer/treescanner.h>

#include <utils/filesystemwatcher.h>

#include <QDateTime>
#include <QFuture>
#include <QObject>

#include <memory>
#include <optional>

namespace ProjectExplorer { class ProjectNode; }

namespace XMakeProjectManager::Internal {

class XMakeProcess;
class FileApiQtcData;

class FileApiReader final : public QObject
{
    Q_OBJECT

public:
    FileApiReader();
    ~FileApiReader();

    void setParameters(const BuildDirParameters &p);

    void resetData();
    void parse(bool forceXMakeRun,
               bool forceInitialConfiguration,
               bool forceExtraConfiguration,
               bool debugging,
               bool profiling);
    void stop();
    void stopXMakeRun();

    bool isParsing() const;

    QList<XMakeBuildTarget> takeBuildTargets(QString &errorMessage);
    QSet<XMakeFileInfo> takeXMakeFileInfos(QString &errorMessage);
    XMakeConfig takeParsedConfiguration(QString &errorMessage);
    QString ctestPath() const;
    ProjectExplorer::RawProjectParts createRawProjectParts(QString &errorMessage);

    bool isMultiConfig() const;
    bool usesAllCapsTargets() const;

    int lastXMakeExitCode() const;

    std::unique_ptr<XMakeProjectNode> rootProjectNode();

    Utils::FilePath topCmakeFile() const;

signals:
    void configurationStarted() const;
    void dataAvailable(bool restoredFromBackup) const;
    void dirty() const;
    void errorOccurred(const QString &message) const;
    void debuggingStarted() const;

private:
    void startState();
    void endState(const Utils::FilePath &replyFilePath, bool restoredFromBackup);
    void startXMakeState(const QStringList &configurationArguments);
    void xmakeFinishedState(int exitCode);

    void replyDirectoryHasChanged(const QString &directory) const;
    void makeBackupConfiguration(bool store);

    void writeConfigurationIntoBuildDirectory(const QStringList &configuration);

    std::unique_ptr<XMakeProcess> m_xmakeProcess;

    // xmake data:
    XMakeConfig m_cache;
    QSet<XMakeFileInfo> m_xmakeFiles;
    QList<XMakeBuildTarget> m_buildTargets;
    ProjectExplorer::RawProjectParts m_projectParts;
    std::unique_ptr<XMakeProjectNode> m_rootProjectNode;
    QString m_ctestPath;
    bool m_isMultiConfig = false;
    bool m_usesAllCapsTargets = false;
    int m_lastXMakeExitCode = 0;

    std::optional<QFuture<std::shared_ptr<FileApiQtcData>>> m_future;

    // Update related:
    bool m_isParsing = false;
    BuildDirParameters m_parameters;

    // Notification on changes outside of creator:
    Utils::FileSystemWatcher m_watcher;
    QDateTime m_lastReplyTimestamp;
};

} // XMakeProjectManager::Internal
