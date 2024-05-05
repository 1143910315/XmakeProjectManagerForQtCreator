// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qtsupport/qtprojectimporter.h>

#include <utils/temporarydirectory.h>

namespace XMakeProjectManager {

class XMakeProject;
class XMakeTool;

namespace Internal {

struct DirectoryData;

class XMakeProjectImporter : public QtSupport::QtProjectImporter
{
public:
    XMakeProjectImporter(const Utils::FilePath &path,
                         const XMakeProjectManager::XMakeProject *project);

    Utils::FilePaths importCandidates() final;
    ProjectExplorer::Target *preferredTarget(const QList<ProjectExplorer::Target *> &possibleTargets) final;
    bool filter(ProjectExplorer::Kit *k) const final;

    Utils::FilePaths presetCandidates();
private:
    QList<void *> examineDirectory(const Utils::FilePath &importPath,
                                   QString *warningMessage) const final;
    bool matchKit(void *directoryData, const ProjectExplorer::Kit *k) const final;
    ProjectExplorer::Kit *createKit(void *directoryData) const final;
    const QList<ProjectExplorer::BuildInfo> buildInfoList(void *directoryData) const final;

    struct XMakeToolData {
        bool isTemporary = false;
        XMakeTool *xmakeTool = nullptr;
    };
    XMakeToolData findOrCreateXMakeTool(const Utils::FilePath &xmakeToolPath) const;

    void deleteDirectoryData(void *directoryData) const final;

    void cleanupTemporaryXMake(ProjectExplorer::Kit *k, const QVariantList &vl);
    void persistTemporaryXMake(ProjectExplorer::Kit *k, const QVariantList &vl);

    void ensureBuildDirectory(DirectoryData &data, const ProjectExplorer::Kit *k) const;

    const XMakeProject *m_project;
    Utils::TemporaryDirectory m_presetsTempDir;
};

#ifdef WITH_TESTS
QObject *createXMakeProjectImporterTest();
#endif

} // namespace Internal
} // namespace XMakeProjectManager
