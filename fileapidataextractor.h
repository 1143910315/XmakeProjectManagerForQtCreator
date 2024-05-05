// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "xmakebuildtarget.h"
#include "xmakeprojectnodes.h"
#include "3rdparty/xmake/cmListFileCache.h"

#include <projectexplorer/rawprojectpart.h>

#include <utils/filepath.h>

#include <QList>
#include <QSet>
#include <QString>

#include <memory>
#include <optional>

namespace XMakeProjectManager::Internal {

class FileApiData;

class XMakeFileInfo
{
public:
    bool operator==(const XMakeFileInfo& other) const { return path == other.path; }
    friend size_t qHash(const XMakeFileInfo &info, uint seed = 0) { return qHash(info.path, seed); }

    bool operator<(const XMakeFileInfo &other) const { return path < other.path; }

    Utils::FilePath path;
    bool isXMake = false;
    bool isXMakeListsDotTxt = false;
    bool isExternal = false;
    bool isGenerated = false;
    cmListFile xmakeListFile;
};

class FileApiQtcData
{
public:
    QString errorMessage;
    XMakeConfig cache;
    QSet<XMakeFileInfo> xmakeFiles;
    QList<XMakeBuildTarget> buildTargets;
    ProjectExplorer::RawProjectParts projectParts;
    std::unique_ptr<XMakeProjectNode> rootProjectNode;
    QString ctestPath;
    bool isMultiConfig = false;
    bool usesAllCapsTargets = false;
};

FileApiQtcData extractData(const QFuture<void> &cancelFuture, FileApiData &input,
                           const Utils::FilePath &sourceDir, const Utils::FilePath &buildDir);

} // XMakeProjectManager::Internal
