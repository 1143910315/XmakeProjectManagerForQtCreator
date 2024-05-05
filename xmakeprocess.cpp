// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "xmakeprocess.h"

#include "builddirparameters.h"
#include "xmakeparser.h"
#include "xmakeprojectconstants.h"
#include "xmakeprojectmanagertr.h"
#include "xmakespecificsettings.h"
#include "xmaketoolmanager.h"

#include <coreplugin/progressmanager/processprogress.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/kitchooser.h>

#include <extensionsystem/invoker.h>
#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/process.h>
#include <utils/processinfo.h>
#include <utils/processinterface.h>
#include <utils/stringutils.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace XMakeProjectManager::Internal {
    static QString stripTrailingNewline(QString str) {
        if (str.endsWith('\n')) {
            str.chop(1);
        }
        return str;
    }

    XMakeProcess::XMakeProcess() = default;

    XMakeProcess::~XMakeProcess() {
        m_parser.flush();
    }

    static const int failedToStartExitCode = 0xFF; // See ProcessPrivate::handleDone() impl

    void XMakeProcess::run(const BuildDirParameters &parameters, const QStringList &arguments) {
        QTC_ASSERT(!m_process, return );

        XMakeTool *xmake = parameters.xmakeTool();
        QTC_ASSERT(parameters.isValid() && xmake, return );

        const FilePath xmakeExecutable = xmake->xmakeExecutable();

        if (!xmakeExecutable.ensureReachable(parameters.sourceDirectory)) {
            const QString msg = ::XMakeProjectManager::Tr::tr(
                "The source directory %1 is not reachable by the XMake executable %2.")
                .arg(parameters.sourceDirectory.displayName()).arg(xmakeExecutable.displayName());
            BuildSystem::appendBuildSystemOutput(addXMakePrefix({ QString(), msg }).join('\n'));
            emit finished(failedToStartExitCode);
            return;
        }

        if (!xmakeExecutable.ensureReachable(parameters.buildDirectory)) {
            const QString msg = ::XMakeProjectManager::Tr::tr(
                "The build directory %1 is not reachable by the XMake executable %2.")
                .arg(parameters.buildDirectory.displayName()).arg(xmakeExecutable.displayName());
            BuildSystem::appendBuildSystemOutput(addXMakePrefix({ QString(), msg }).join('\n'));
            emit finished(failedToStartExitCode);
            return;
        }

        const FilePath sourceDirectory = xmakeExecutable.withNewMappedPath(parameters.sourceDirectory);
        const FilePath buildDirectory = parameters.buildDirectory;

        if (!buildDirectory.exists()) {
            const QString msg = ::XMakeProjectManager::Tr::tr(
                "The build directory \"%1\" does not exist").arg(buildDirectory.toUserOutput());
            BuildSystem::appendBuildSystemOutput(addXMakePrefix({ QString(), msg }).join('\n'));
            emit finished(failedToStartExitCode);
            return;
        }

        if (buildDirectory.needsDevice()) {
            if (!xmake->xmakeExecutable().isSameDevice(buildDirectory)) {
                const QString msg = ::XMakeProjectManager::Tr::tr(
                    "XMake executable \"%1\" and build directory \"%2\" must be on the same device.")
                    .arg(xmake->xmakeExecutable().toUserOutput(), buildDirectory.toUserOutput());
                BuildSystem::appendBuildSystemOutput(addXMakePrefix({ QString(), msg }).join('\n'));
                emit finished(failedToStartExitCode);
                return;
            }
        }

        // Copy the "package-manager" XMake code from the ${IDE:ResourcePath} to the build directory
        if (settings().packageManagerAutoSetup()) {
            const FilePath localPackageManagerDir = buildDirectory.pathAppended(Constants::PACKAGE_MANAGER_DIR);
            const FilePath idePackageManagerDir = FilePath::fromString(
                parameters.expander->expand(QStringLiteral("%{IDE:ResourcePath}/package-manager")));

            if (!localPackageManagerDir.exists() && idePackageManagerDir.exists()) {
                idePackageManagerDir.copyRecursively(localPackageManagerDir);
            }
        }

        const auto parser = new XMakeParser;
        parser->setSourceDirectory(parameters.sourceDirectory);
        m_parser.addLineParser(parser);
        m_parser.addLineParsers(parameters.outputParsers());

        // Always use the sourceDir: If we are triggered because the build directory is getting deleted
        // then we are racing against XMakeCache.txt also getting deleted.

        m_process.reset(new Process);

        m_process->setWorkingDirectory(buildDirectory);
        m_process->setEnvironment(parameters.environment);

        m_process->setStdOutLineCallback([this](const QString &s) {
                                             BuildSystem::appendBuildSystemOutput(addXMakePrefix(stripTrailingNewline(s)));
                                             emit stdOutReady(s);
                                         });

        m_process->setStdErrLineCallback([this](const QString &s) {
                                             m_parser.appendMessage(s, StdErrFormat);
                                             BuildSystem::appendBuildSystemOutput(addXMakePrefix(stripTrailingNewline(s)));
                                         });

        connect(m_process.get(), &Process::done, this, [this] {
                    if (m_process->result() != ProcessResult::FinishedWithSuccess) {
                        const QString message = m_process->exitMessage();
                        BuildSystem::appendBuildSystemOutput(addXMakePrefix({ {}, message }).join('\n'));
                        TaskHub::addTask(BuildSystemTask(Task::Error, message));
                    }

                    emit finished(m_process->exitCode());

                    const QString elapsedTime = Utils::formatElapsedTime(m_elapsed.elapsed());
                    BuildSystem::appendBuildSystemOutput(addXMakePrefix({ {}, elapsedTime }).join('\n'));
                });

        CommandLine commandLine(xmakeExecutable);

        TaskHub::clearTasks(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);

        BuildSystem::startNewBuildSystemOutput(
            addXMakePrefix(::XMakeProjectManager::Tr::tr("Running %1 in %2.")
                           .arg(commandLine.toUserOutput(), buildDirectory.toUserOutput())));

        ProcessProgress *progress = new ProcessProgress(m_process.get());
        progress->setDisplayName(::XMakeProjectManager::Tr::tr("Configuring \"%1\"")
                                 .arg(parameters.projectName));
        m_process->setCommand(commandLine);
        m_elapsed.start();
        m_process->start();
    }

    void XMakeProcess::stop() {
        if (m_process) {
            m_process->stop();
        }
    }

    QString addXMakePrefix(const QString &str) {
        auto qColorToAnsiCode = [](const QColor &color) {
            return QString::fromLatin1("\033[38;2;%1;%2;%3m")
                   .arg(color.red()).arg(color.green()).arg(color.blue());
        };
        static const QColor bgColor = creatorTheme()->color(Theme::BackgroundColorNormal);
        static const QColor fgColor = creatorTheme()->color(Theme::TextColorNormal);
        static const QColor grey = StyleHelper::mergedColors(fgColor, bgColor, 80);
        static const QString prefixString = qColorToAnsiCode(grey) + Constants::OUTPUT_PREFIX
            + qColorToAnsiCode(fgColor);
        return QString("%1%2").arg(prefixString, str);
    }

    QStringList addXMakePrefix(const QStringList &list) {
        return Utils::transform(list, [](const QString &str) {
                                    return addXMakePrefix(str);
                                });
    }
} // XMakeProjectManager::Internal
