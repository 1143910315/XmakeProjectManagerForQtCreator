// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "xmaketool.h"

#include <utils/environment.h>
#include <utils/filepath.h>

#include <functional>

namespace Utils {
class MacroExpander;
class OutputLineParser;
} // namespace Utils

namespace XMakeProjectManager::Internal {

class XMakeBuildSystem;

class BuildDirParameters
{
public:
    BuildDirParameters();
    explicit BuildDirParameters(XMakeBuildSystem *buildSystem);

    bool isValid() const;
    XMakeTool *xmakeTool() const;

    QString projectName;

    Utils::FilePath sourceDirectory;
    Utils::FilePath buildDirectory;
    QString xmakeBuildType;

    Utils::Environment environment;

    Utils::Id xmakeToolId;

    QStringList initialXMakeArguments;
    QStringList configurationChangesArguments;
    QStringList additionalXMakeArguments;

    Utils::MacroExpander* expander = nullptr;

    QList<Utils::OutputLineParser*> outputParsers() const;

private:
    using OutputParserGenerator = std::function<QList<Utils::OutputLineParser*>()>;
    OutputParserGenerator outputParserGenerator;
};

} // XMakeProjectManager::Internal
