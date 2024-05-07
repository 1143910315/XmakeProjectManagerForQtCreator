// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fileapireader.h"

#include "xmakeprocess.h"
#include "xmakeprojectmanagertr.h"
#include "xmakeprojectconstants.h"
#include "xmakespecificsettings.h"
#include "fileapidataextractor.h"
#include "fileapiparser.h"

#include <coreplugin/messagemanager.h>

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/projectexplorer.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/futuresynchronizer.h>
#include <utils/qtcassert.h>
#include <utils/temporarydirectory.h>

#include <QLoggingCategory>

using namespace ProjectExplorer;
using namespace Utils;

namespace XMakeProjectManager::Internal {

static Q_LOGGING_CATEGORY(xmakeFileApiMode, "qtc.xmake.fileApiMode", QtWarningMsg);

using namespace FileApiDetails;

// --------------------------------------------------------------------
// FileApiReader:
// --------------------------------------------------------------------

FileApiReader::FileApiReader()
    : m_lastReplyTimestamp()
{
    QObject::connect(&m_watcher,
                     &FileSystemWatcher::directoryChanged,
                     this,
                     &FileApiReader::replyDirectoryHasChanged);
}

FileApiReader::~FileApiReader()
{
    stop();
    resetData();
}

void FileApiReader::setParameters(const BuildDirParameters &p)
{
    qCDebug(xmakeFileApiMode)
        << "\n\n\n\n\n=============================================================\n";

    // Update:
    m_parameters = p;
    qCDebug(xmakeFileApiMode) << "Work directory:" << m_parameters.buildDirectory.toUserOutput();

    // Reset watcher:
    m_watcher.clear();

    FileApiParser::setupXMakeFileApi(m_parameters.buildDirectory);

    resetData();
}

void FileApiReader::resetData()
{
    m_xmakeFiles.clear();
    if (!m_parameters.sourceDirectory.isEmpty()) {
        XMakeFileInfo xmakeListsTxt;
        xmakeListsTxt.path = m_parameters.sourceDirectory.pathAppended(Constants::PROJECT_FILE_NAME);
        xmakeListsTxt.isXMakeListsDotTxt = true;
        m_xmakeFiles.insert(xmakeListsTxt);
    }

    m_cache.clear();
    m_buildTargets.clear();
    m_projectParts.clear();
    m_rootProjectNode.reset();
}

void FileApiReader::parse(bool forceXMakeRun,
                          bool forceInitialConfiguration,
                          bool forceExtraConfiguration,
                          bool debugging,
                          bool profiling)
{
    qCDebug(xmakeFileApiMode) << "Parse called with arguments: ForceXMakeRun:" << forceXMakeRun
                              << " - forceConfiguration:" << forceInitialConfiguration
                              << " - forceExtraConfiguration:" << forceExtraConfiguration;
    startState();

    QStringList args = (forceInitialConfiguration ? m_parameters.initialXMakeArguments
                                                        : QStringList())
                             + (forceExtraConfiguration
                                    ? (m_parameters.configurationChangesArguments
                                       + m_parameters.additionalXMakeArguments)
                                    : QStringList());
    if (debugging) {
        if (TemporaryDirectory::masterDirectoryFilePath().osType() == Utils::OsType::OsTypeWindows) {
            args << "--debugger"
                 << "--debugger-pipe \\\\.\\pipe\\xmake-dap";
        } else {
            FilePath file = TemporaryDirectory::masterDirectoryFilePath() / "xmake-dap.sock";
            file.removeFile();
            args << "--debugger"
                 << "--debugger-pipe=" + file.path();
        }
    }

    if (profiling) {
        const FilePath file = TemporaryDirectory::masterDirectoryFilePath() / "xmake-profile.json";
        args << "--profiling-format=google-trace"
             << "--profiling-output=" + file.path();
    }

    qCDebug(xmakeFileApiMode) << "Parameters request these XMake arguments:" << args;

    const FilePath replyFile = FileApiParser::scanForXMakeReplyFile(m_parameters.buildDirectory);
    // Only need to update when one of the following conditions is met:
    //  * The user forces the xmake run,
    //  * The user provided arguments,
    //  * There is no reply file,
    //  * One of the xmakefiles is newer than the replyFile and the user asked
    //    for creator to run XMake as needed,
    //  * A query file is newer than the reply file
    const bool hasArguments = !args.isEmpty();
    const bool replyFileMissing = !replyFile.exists();
    const bool xmakeFilesChanged = m_parameters.xmakeTool() && settings().autorunXMake()
                                   && anyOf(m_xmakeFiles, [&replyFile](const XMakeFileInfo &info) {
                                          return !info.isGenerated
                                                 && info.path.lastModified() > replyFile.lastModified();
                                      });
    const bool queryFileChanged = anyOf(FileApiParser::xmakeQueryFilePaths(m_parameters.buildDirectory),
                                        [&replyFile](const FilePath &qf) {
                                            return qf.lastModified() > replyFile.lastModified();
                                        });

    const bool mustUpdate = forceXMakeRun || hasArguments || replyFileMissing || xmakeFilesChanged
                            || queryFileChanged;
    qCDebug(xmakeFileApiMode) << QString("Do I need to run XMake? %1 "
                                         "(force: %2 | args: %3 | missing reply: %4 | "
                                         "xmakeFilesChanged: %5 | "
                                         "queryFileChanged: %6)")
                                     .arg(mustUpdate)
                                     .arg(forceXMakeRun)
                                     .arg(hasArguments)
                                     .arg(replyFileMissing)
                                     .arg(xmakeFilesChanged)
                                     .arg(queryFileChanged);

    if (mustUpdate) {
        qCDebug(xmakeFileApiMode) << QString("FileApiReader: Starting XMake with \"%1\".")
                                         .arg(args.join("\", \""));
        startXMakeState(args);
    } else {
        endState(replyFile, false);
    }
}

void FileApiReader::stop()
{
    if (m_xmakeProcess)
        disconnect(m_xmakeProcess.get(), nullptr, this, nullptr);
    m_xmakeProcess.reset();

    if (m_future) {
        m_future->cancel();
        ExtensionSystem::PluginManager::futureSynchronizer()->addFuture(*m_future);
    }
    m_future = {};
    m_isParsing = false;
}

void FileApiReader::stopXMakeRun()
{
    if (m_xmakeProcess)
        m_xmakeProcess->stop();
}

bool FileApiReader::isParsing() const
{
    return m_isParsing;
}

QList<XMakeBuildTarget> FileApiReader::takeBuildTargets(QString &errorMessage){
    Q_UNUSED(errorMessage)

    return std::exchange(m_buildTargets, {});
}

QSet<XMakeFileInfo> FileApiReader::takeXMakeFileInfos(QString &errorMessage)
{
    Q_UNUSED(errorMessage)

    return std::exchange(m_xmakeFiles, {});
}

XMakeConfig FileApiReader::takeParsedConfiguration(QString &errorMessage)
{
    if (m_lastXMakeExitCode != 0)
        errorMessage = Tr::tr("XMake returned error code: %1").arg(m_lastXMakeExitCode);

    return std::exchange(m_cache, {});
}

QString FileApiReader::ctestPath() const
{
    // if we failed to run xmake we should not offer ctest information either
    return m_lastXMakeExitCode == 0 ? m_ctestPath : QString();
}

bool FileApiReader::isMultiConfig() const
{
    return m_isMultiConfig;
}

bool FileApiReader::usesAllCapsTargets() const
{
    return m_usesAllCapsTargets;
}

RawProjectParts FileApiReader::createRawProjectParts(QString &errorMessage)
{
    Q_UNUSED(errorMessage)

    return std::exchange(m_projectParts, {});
}

void FileApiReader::startState()
{
    qCDebug(xmakeFileApiMode) << "FileApiReader: START STATE.";
    QTC_ASSERT(!m_isParsing, return );
    QTC_ASSERT(!m_future.has_value(), return );

    m_isParsing = true;

    qCDebug(xmakeFileApiMode) << "FileApiReader: CONFIGURATION STARTED SIGNAL";
    emit configurationStarted();
}

void FileApiReader::endState(const FilePath &replyFilePath, bool restoredFromBackup)
{
    qCDebug(xmakeFileApiMode) << "FileApiReader: END STATE.";
    QTC_ASSERT(m_isParsing, return );
    QTC_ASSERT(!m_future.has_value(), return );

    const FilePath sourceDirectory = m_parameters.sourceDirectory;
    const FilePath buildDirectory = m_parameters.buildDirectory;
    const QString xmakeBuildType = m_parameters.xmakeBuildType == "Build"
                                       ? "" : m_parameters.xmakeBuildType;

    m_lastReplyTimestamp = replyFilePath.lastModified();

    m_future = Utils::asyncRun(ProjectExplorerPlugin::sharedThreadPool(),
                        [replyFilePath, sourceDirectory, buildDirectory, xmakeBuildType](
                            QPromise<std::shared_ptr<FileApiQtcData>> &promise) {
                            auto result = std::make_shared<FileApiQtcData>();
                            FileApiData data = FileApiParser::parseData(promise,
                                                                        replyFilePath,
                                                                        buildDirectory,
                                                                        xmakeBuildType,
                                                                        result->errorMessage);
                            if (result->errorMessage.isEmpty()) {
                                *result = extractData(QFuture<void>(promise.future()), data,
                                                      sourceDirectory, buildDirectory);
                            } else {
                                qWarning() << result->errorMessage;
                                result->cache = std::move(data.cache);
                            }

                            promise.addResult(result);
                        });
    onResultReady(m_future.value(),
                  this,
                  [this, sourceDirectory, buildDirectory, restoredFromBackup](
                      const std::shared_ptr<FileApiQtcData> &value) {
                      m_isParsing = false;
                      m_cache = std::move(value->cache);
                      m_xmakeFiles = std::move(value->xmakeFiles);
                      m_buildTargets = std::move(value->buildTargets);
                      m_projectParts = std::move(value->projectParts);
                      m_rootProjectNode = std::move(value->rootProjectNode);
                      m_ctestPath = std::move(value->ctestPath);
                      m_isMultiConfig = value->isMultiConfig;
                      m_usesAllCapsTargets = value->usesAllCapsTargets;

                      if (value->errorMessage.isEmpty()) {
                          emit this->dataAvailable(restoredFromBackup);
                      } else {
                          emit this->errorOccurred(value->errorMessage);
                      }
                      m_future = {};
                  });
}

void FileApiReader::makeBackupConfiguration(bool store)
{
    FilePath reply = m_parameters.buildDirectory.pathAppended(".xmake/api/v1/reply");
    FilePath replyPrev = m_parameters.buildDirectory.pathAppended(".xmake/api/v1/reply.prev");
    if (!store)
        std::swap(reply, replyPrev);

    if (reply.exists()) {
        if (replyPrev.exists())
            replyPrev.removeRecursively();
        QTC_CHECK(!replyPrev.exists());
        if (!reply.renameFile(replyPrev))
            Core::MessageManager::writeFlashing(
                addXMakePrefix(Tr::tr("Failed to rename \"%1\" to \"%2\".")
                                   .arg(reply.toString(), replyPrev.toString())));
    }

    FilePath xmakeCacheTxt = m_parameters.buildDirectory.pathAppended(Constants::XMAKE_CACHE_TXT);
    FilePath xmakeCacheTxtPrev = m_parameters.buildDirectory.pathAppended(Constants::XMAKE_CACHE_TXT_PREV);
    if (!store)
        std::swap(xmakeCacheTxt, xmakeCacheTxtPrev);

    if (xmakeCacheTxt.exists())
        if (!FileUtils::copyIfDifferent(xmakeCacheTxt, xmakeCacheTxtPrev))
            Core::MessageManager::writeFlashing(
                addXMakePrefix(Tr::tr("Failed to copy \"%1\" to \"%2\".")
                                   .arg(xmakeCacheTxt.toString(), xmakeCacheTxtPrev.toString())));
}

void FileApiReader::writeConfigurationIntoBuildDirectory(const QStringList &configurationArguments)
{
    const FilePath buildDir = m_parameters.buildDirectory;
    QTC_CHECK(buildDir.ensureWritableDir());

    QByteArray contents;
    QStringList unknownOptions;
    contents.append("# This file is managed by Qt Creator, do not edit!\n\n");
    contents.append(
        transform(XMakeConfig::fromArguments(configurationArguments, unknownOptions).toList(),
                  [](const XMakeConfigItem &item) { return item.toXMakeSetLine(nullptr); })
            .join('\n')
            .toUtf8());

    const FilePath settingsFile = buildDir / "qtcsettings.xmake";
    QTC_CHECK(settingsFile.writeFileContents(contents));
}

std::unique_ptr<XMakeProjectNode> FileApiReader::rootProjectNode()
{
    return std::exchange(m_rootProjectNode, {});
}

FilePath FileApiReader::topCmakeFile() const
{
    return m_xmakeFiles.size() == 1 ? (*m_xmakeFiles.begin()).path : FilePath{};
}

int FileApiReader::lastXMakeExitCode() const
{
    return m_lastXMakeExitCode;
}

void FileApiReader::startXMakeState(const QStringList &configurationArguments)
{
    qCDebug(xmakeFileApiMode) << "FileApiReader: START XMAKE STATE.";
    QTC_ASSERT(!m_xmakeProcess, return );

    m_xmakeProcess = std::make_unique<XMakeProcess>();

    connect(m_xmakeProcess.get(), &XMakeProcess::finished, this, &FileApiReader::xmakeFinishedState);
    connect(m_xmakeProcess.get(), &XMakeProcess::stdOutReady, this, [this](const QString &data) {
        if (data.endsWith("Waiting for debugger client to connect...\n"))
            emit debuggingStarted();
    });

    qCDebug(xmakeFileApiMode) << ">>>>>> Running xmake with arguments:" << configurationArguments;
    // Reset watcher:
    m_watcher.removeFiles(m_watcher.filePaths());
    m_watcher.removeDirectories(m_watcher.directoryPaths());

    makeBackupConfiguration(true);
    writeConfigurationIntoBuildDirectory(configurationArguments);
    m_xmakeProcess->run(m_parameters, configurationArguments);
}

void FileApiReader::xmakeFinishedState(int exitCode)
{
    qCDebug(xmakeFileApiMode) << "FileApiReader: XMAKE FINISHED STATE.";

    m_lastXMakeExitCode = exitCode;
    m_xmakeProcess.release()->deleteLater();

    if (m_lastXMakeExitCode != 0)
        makeBackupConfiguration(false);

    FileApiParser::setupXMakeFileApi(m_parameters.buildDirectory);

    m_watcher.addDirectory(FileApiParser::xmakeReplyDirectory(m_parameters.buildDirectory).path(),
                           FileSystemWatcher::WatchAllChanges);

    endState(FileApiParser::scanForXMakeReplyFile(m_parameters.buildDirectory),
             m_lastXMakeExitCode != 0);
}

void FileApiReader::replyDirectoryHasChanged(const QString &directory) const
{
    if (m_isParsing)
        return; // This has been triggered by ourselves, ignore.

    const FilePath reply = FileApiParser::scanForXMakeReplyFile(m_parameters.buildDirectory);
    const FilePath dir = reply.absolutePath();
    if (dir.isEmpty())
        return; // XMake started to fill the result dir, but has not written a result file yet
    QTC_CHECK(!dir.needsDevice());
    QTC_ASSERT(dir.path() == directory, return);

    if (m_lastReplyTimestamp.isValid() && reply.lastModified() > m_lastReplyTimestamp)
        emit dirty();
}

} // XMakeProjectManager::Internal
