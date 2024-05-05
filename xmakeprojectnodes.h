// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "xmakeconfigitem.h"

#include <projectexplorer/projectnodes.h>

namespace XMakeProjectManager::Internal {

class XMakeInputsNode : public ProjectExplorer::ProjectNode
{
public:
    XMakeInputsNode(const Utils::FilePath &xmakeLists);
};

class XMakePresetsNode : public ProjectExplorer::ProjectNode
{
public:
    XMakePresetsNode(const Utils::FilePath &projectPath);
};

class XMakeListsNode : public ProjectExplorer::ProjectNode
{
public:
    XMakeListsNode(const Utils::FilePath &xmakeListPath);

    bool showInSimpleTree() const final;
    std::optional<Utils::FilePath> visibleAfterAddFileAction() const override;
};

class XMakeProjectNode : public ProjectExplorer::ProjectNode
{
public:
    XMakeProjectNode(const Utils::FilePath &directory);

    QString tooltip() const final;
};

class XMakeTargetNode : public ProjectExplorer::ProjectNode
{
public:
    XMakeTargetNode(const Utils::FilePath &directory, const QString &target);

    void setTargetInformation(const QList<Utils::FilePath> &artifacts, const QString &type);

    QString tooltip() const final;
    QString buildKey() const final;
    Utils::FilePath buildDirectory() const;
    void setBuildDirectory(const Utils::FilePath &directory);

    std::optional<Utils::FilePath> visibleAfterAddFileAction() const override;

    void build() override;

    QVariant data(Utils::Id role) const override;
    void setConfig(const XMakeConfig &config);

private:
    QString m_tooltip;
    Utils::FilePath m_buildDirectory;
    Utils::FilePath m_artifact;
    XMakeConfig m_config;
};

} // XMakeProjectManager::Internal
