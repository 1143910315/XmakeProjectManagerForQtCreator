// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/outputformatter.h>

#include <QElapsedTimer>
#include <QObject>
#include <QStringList>

#include <memory>

namespace Utils {
class ProcessResultData;
class Process;
}

namespace XMakeProjectManager::Internal {

class BuildDirParameters;

class XMakeProcess : public QObject
{
    Q_OBJECT

public:
    XMakeProcess();
    ~XMakeProcess();

    void run(const BuildDirParameters &parameters, const QStringList &arguments);
    void stop();

signals:
    void finished(int exitCode);
    void stdOutReady(const QString &s);

private:
    std::unique_ptr<Utils::Process> m_process;
    Utils::OutputFormatter m_parser;
    QElapsedTimer m_elapsed;
};

QString addXMakePrefix(const QString &str);
QStringList addXMakePrefix(const QStringList &list);

} // XMakeProjectManager::Internal
