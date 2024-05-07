// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projecttreehelper.h"

#include "xmakeproject.h"
#include "xmakeprojectconstants.h"
#include "xmakeprojectmanagertr.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>

#include <utils/algorithm.h>
#include <utils/fsengine/fileiconprovider.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;

namespace XMakeProjectManager::Internal {

bool sourcesOrHeadersFolder(const QString &displayName)
{
    return displayName == "Source Files" || displayName == "Header Files";
}

std::unique_ptr<FolderNode> createXMakeVFolder(const Utils::FilePath &basePath,
                                               int priority,
                                               const QString &displayName)
{
    auto newFolder = std::make_unique<VirtualFolderNode>(basePath);
    newFolder->setPriority(priority);
    newFolder->setDisplayName(displayName);
    newFolder->setIsSourcesOrHeaders(sourcesOrHeadersFolder(displayName));
    return newFolder;
}

void addXMakeVFolder(FolderNode *base,
                     const Utils::FilePath &basePath,
                     int priority,
                     const QString &displayName,
                     std::vector<std::unique_ptr<FileNode>> &&files,
                     bool listInProject)
{
    if (files.size() == 0)
        return;
    FolderNode *folder = base;
    if (!displayName.isEmpty()) {
        auto newFolder = createXMakeVFolder(basePath, priority, displayName);
        folder = newFolder.get();
        base->addNode(std::move(newFolder));
    }
    if (!listInProject) {
        for (auto it = files.begin(); it != files.end(); ++it)
            (*it)->setListInProject(false);
    }
    folder->addNestedNodes(std::move(files));
    folder->forEachFolderNode([] (FolderNode *fn) { fn->compress(); });
}

std::vector<std::unique_ptr<FileNode>> &&removeKnownNodes(
    const QSet<Utils::FilePath> &knownFiles, std::vector<std::unique_ptr<FileNode>> &&files)
{
    Utils::erase(files, [&knownFiles](const std::unique_ptr<FileNode> &n) {
        return knownFiles.contains(n->filePath());
    });
    return std::move(files);
}

void addXMakeInputs(FolderNode *root,
                    const Utils::FilePath &sourceDir,
                    const Utils::FilePath &buildDir,
                    std::vector<std::unique_ptr<FileNode>> &&sourceInputs,
                    std::vector<std::unique_ptr<FileNode>> &&buildInputs,
                    std::vector<std::unique_ptr<FileNode>> &&rootInputs)
{
    std::unique_ptr<ProjectNode> xmakeVFolder = std::make_unique<XMakeInputsNode>(root->filePath());

    QSet<Utils::FilePath> knownFiles;
    root->forEachGenericNode([&knownFiles](const Node *n) { knownFiles.insert(n->filePath()); });

    addXMakeVFolder(xmakeVFolder.get(),
                    sourceDir,
                    1000,
                    QString(),
                    removeKnownNodes(knownFiles, std::move(sourceInputs)));
    addXMakeVFolder(xmakeVFolder.get(),
                    buildDir,
                    100,
                    Tr::tr("<Build Directory>"),
                    removeKnownNodes(knownFiles, std::move(buildInputs)));
    addXMakeVFolder(xmakeVFolder.get(),
                    Utils::FilePath(),
                    10,
                    Tr::tr("<Other Locations>"),
                    removeKnownNodes(knownFiles, std::move(rootInputs)),
                    /*listInProject=*/false);

    root->addNode(std::move(xmakeVFolder));
}

void addXMakePresets(FolderNode *root, const Utils::FilePath &sourceDir)
{
    QStringList presetFileNames;
    presetFileNames << "XMakePresets.json";
    presetFileNames << "XMakeUserPresets.json";

    const XMakeProject *cp = static_cast<const XMakeProject *>(
        ProjectManager::projectForFile(sourceDir.pathAppended(Constants::PROJECT_FILE_NAME)));

    if (cp && cp->presetsData().include)
        presetFileNames.append(cp->presetsData().include.value());

    std::vector<std::unique_ptr<FileNode>> presets;
    for (const auto &fileName : presetFileNames) {
        Utils::FilePath file = sourceDir.pathAppended(fileName);
        if (file.exists())
            presets.push_back(std::make_unique<FileNode>(file, Node::fileTypeForFileName(file)));
    }

    if (presets.empty())
        return;

    std::unique_ptr<ProjectNode> xmakeVFolder = std::make_unique<XMakePresetsNode>(root->filePath());
    addXMakeVFolder(xmakeVFolder.get(), sourceDir, 1000, QString(), std::move(presets));

    root->addNode(std::move(xmakeVFolder));
}

QHash<Utils::FilePath, ProjectNode *> addXMakeLists(
    XMakeProjectNode *root, std::vector<std::unique_ptr<FileNode>> &&xmakeLists)
{
    QHash<Utils::FilePath, ProjectNode *> xmakeListsNodes;
    xmakeListsNodes.insert(root->filePath(), root);

    const QSet<Utils::FilePath> xmakeDirs
        = Utils::transform<QSet>(xmakeLists, [](const std::unique_ptr<FileNode> &n) {
              return n->filePath().parentDir();
          });
    root->addNestedNodes(std::move(xmakeLists),
                         Utils::FilePath(),
                         [&xmakeDirs, &xmakeListsNodes](const Utils::FilePath &fp)
                             -> std::unique_ptr<ProjectExplorer::FolderNode> {
                             if (xmakeDirs.contains(fp)) {
                                 auto fn = std::make_unique<XMakeListsNode>(fp);
                                 xmakeListsNodes.insert(fp, fn.get());
                                 return fn;
                             }

                             return std::make_unique<FolderNode>(fp);
                         });
    root->compress();
    return xmakeListsNodes;
}

void createProjectNode(const QHash<Utils::FilePath, ProjectNode *> &xmakeListsNodes,
                       const Utils::FilePath &dir,
                       const QString &displayName)
{
    ProjectNode *cmln = xmakeListsNodes.value(dir);
    QTC_ASSERT(cmln, return );

    const Utils::FilePath projectName = dir.pathAppended(".project::" + displayName);

    ProjectNode *pn = cmln->projectNode(projectName);
    if (!pn) {
        auto newNode = std::make_unique<XMakeProjectNode>(projectName);
        pn = newNode.get();
        cmln->addNode(std::move(newNode));
    }
    pn->setDisplayName(displayName);
}

XMakeTargetNode *createTargetNode(const QHash<Utils::FilePath, ProjectNode *> &xmakeListsNodes,
                                  const Utils::FilePath &dir,
                                  const QString &displayName)
{
    ProjectNode *cmln = xmakeListsNodes.value(dir);
    QTC_ASSERT(cmln, return nullptr);

    QString targetId = displayName;

    XMakeTargetNode *tn = static_cast<XMakeTargetNode *>(
        cmln->findNode([&targetId](const Node *n) { return n->buildKey() == targetId; }));
    if (!tn) {
        auto newNode = std::make_unique<XMakeTargetNode>(dir, displayName);
        tn = newNode.get();
        cmln->addNode(std::move(newNode));
    }
    tn->setDisplayName(displayName);
    return tn;
}

template<typename Result>
static std::unique_ptr<Result> cloneFolderNode(FolderNode *node)
{
    auto folderNode = std::make_unique<Result>(node->filePath());
    folderNode->setDisplayName(node->displayName());
    for (Node *node : node->nodes()) {
        if (FileNode *fn = node->asFileNode()) {
            folderNode->addNode(std::unique_ptr<FileNode>(fn->clone()));
        } else if (FolderNode *fn = node->asFolderNode()) {
            folderNode->addNode(cloneFolderNode<FolderNode>(fn));
        } else {
            QTC_CHECK(false);
        }
    }
    return folderNode;
}

void addFileSystemNodes(ProjectNode *root, const std::shared_ptr<FolderNode> &folderNode)
{
    QTC_ASSERT(root, return );

    auto fileSystemNode = cloneFolderNode<VirtualFolderNode>(folderNode.get());
    // just before special nodes like "XMake Modules"
    fileSystemNode->setPriority(Node::DefaultPriority - 6);
    fileSystemNode->setDisplayName(Tr::tr("<File System>"));
    fileSystemNode->setIcon(DirectoryIcon(ProjectExplorer::Constants::FILEOVERLAY_UNKNOWN));

    if (!fileSystemNode->isEmpty()) {
        // make file system nodes less probable to be selected when syncing with the current document
        fileSystemNode->forEachGenericNode([](Node *n) {
            n->setPriority(n->priority() + Node::DefaultProjectFilePriority + 1);
            n->setEnabled(false);
        });
        root->addNode(std::move(fileSystemNode));
    }
}

} // XMakeProjectManager::Internal
