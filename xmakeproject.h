// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "xmake_global.h"
#include "presetsparser.h"

#include <projectexplorer/project.h>

namespace XMakeProjectManager {
    namespace Internal { class XMakeProjectImporter; }

    class CMAKE_EXPORT XMakeProject final : public ProjectExplorer::Project {
        Q_OBJECT
public:
        explicit XMakeProject(const Utils::FilePath &filename);
        ~XMakeProject() final;

        ProjectExplorer::Tasks projectIssues(const ProjectExplorer::Kit *k) const final;

        ProjectExplorer::ProjectImporter *projectImporter() const final;

        using IssueType = ProjectExplorer::Task::TaskType;
        void addIssue(IssueType type, const QString &text);
        void clearIssues();

        Internal::PresetsData presetsData() const;
        void readPresets();

        void setOldPresetKits(const QList<ProjectExplorer::Kit *> &presetKits) const;
        QList<ProjectExplorer::Kit *> oldPresetKits() const;

protected:
        bool setupTarget(ProjectExplorer::Target *t) final;

private:
        ProjectExplorer::DeploymentKnowledge deploymentKnowledge() const override;
        void configureAsExampleProject(ProjectExplorer::Kit *kit) override;

        Internal::PresetsData combinePresets(Internal::PresetsData &xmakePresetsData,
                                             Internal::PresetsData &xmakeUserPresetsData);
        void setupBuildPresets(Internal::PresetsData &presetsData);

        mutable Internal::XMakeProjectImporter *m_projectImporter = nullptr;
        mutable QList<ProjectExplorer::Kit *> m_oldPresetKits;

        ProjectExplorer::Tasks m_issues;
        Internal::PresetsData m_presetsData;
    };
} // namespace XMakeProjectManager
