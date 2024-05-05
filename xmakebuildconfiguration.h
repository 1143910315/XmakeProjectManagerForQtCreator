// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "xmake_global.h"
#include "xmakeconfigitem.h"

#include <projectexplorer/buildaspects.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/environmentaspect.h>

#include <qtsupport/qtbuildaspects.h>

namespace XMakeProjectManager {
    class XMakeProject;

    namespace Internal {
        class XMakeBuildSystem;
        class XMakeBuildSettingsWidget;
        class XMakeProjectImporter;

        class InitialXMakeArgumentsAspect final : public Utils::StringAspect {
public:
            InitialXMakeArgumentsAspect(Utils::AspectContainer *container);

            const XMakeConfig &xmakeConfiguration() const;
            const QStringList allValues() const;
            void setAllValues(const QString &values, QStringList &additionalArguments);
            void setXMakeConfiguration(const XMakeConfig &config);

            void fromMap(const Utils::Store &map) final;
            void toMap(Utils::Store &map) const final;

private:
            XMakeConfig m_xmakeConfiguration;
        };

        class ConfigureEnvironmentAspect final : public ProjectExplorer::EnvironmentAspect {
public:
            ConfigureEnvironmentAspect(Utils::AspectContainer *container,
                                       ProjectExplorer::BuildConfiguration *buildConfig);

            void fromMap(const Utils::Store &map) override;
            void toMap(Utils::Store &map) const override;
        };
    } // namespace Internal

    class XMAKE_EXPORT XMakeBuildConfiguration : public ProjectExplorer::BuildConfiguration {
        Q_OBJECT
public:
        XMakeBuildConfiguration(ProjectExplorer::Target *target, Utils::Id id);
        ~XMakeBuildConfiguration() override;

        static Utils::FilePath shadowBuildDirectory(const Utils::FilePath &projectFilePath, const ProjectExplorer::Kit *k,
                                                    const QString &bcName, BuildConfiguration::BuildType buildType);
        static bool isIos(const ProjectExplorer::Kit *k);
        static bool hasQmlDebugging(const XMakeConfig &config);

        // Context menu action:
        void buildTarget(const QString &buildTarget);
        ProjectExplorer::BuildSystem *buildSystem() const final;

        void addToEnvironment(Utils::Environment &env) const override;

        Utils::Environment configureEnvironment() const;
        Internal::XMakeBuildSystem *xmakeBuildSystem() const;

        QStringList additionalXMakeArguments() const;
        void setAdditionalXMakeArguments(const QStringList &args);

        void setInitialXMakeArguments(const QStringList &args);
        void setXMakeBuildType(const QString &xmakeBuildType, bool quiet = false);

        Internal::InitialXMakeArgumentsAspect initialXMakeArguments { this };
        Utils::StringAspect additionalXMakeOptions { this };
        Utils::FilePathAspect sourceDirectory { this };
        Utils::StringAspect buildTypeAspect { this };
        QtSupport::QmlDebuggingAspect qmlDebugging { this };
        Internal::ConfigureEnvironmentAspect configureEnv { this, this };

signals:
        void signingFlagsChanged();
        void configureEnvironmentChanged();

private:
        BuildType buildType() const override;

        ProjectExplorer::NamedWidget *createConfigWidget() override;

        virtual XMakeConfig signingFlags() const;

        void setInitialBuildAndCleanSteps(const ProjectExplorer::Target *target);
        void setBuildPresetToBuildSteps(const ProjectExplorer::Target *target);
        void filterConfigArgumentsFromAdditionalXMakeArguments();

        Internal::XMakeBuildSystem *m_buildSystem = nullptr;

        friend class Internal::XMakeBuildSettingsWidget;
        friend class Internal::XMakeBuildSystem;
    };

    class XMAKE_EXPORT XMakeBuildConfigurationFactory
        : public ProjectExplorer::BuildConfigurationFactory {
public:
        XMakeBuildConfigurationFactory();

        enum BuildType {
            BuildTypeNone = 0,
            BuildTypeXMake = 1,
            BuildTypeLast = 2
        };
        static BuildType buildTypeFromByteArray(const QByteArray &in);
        static ProjectExplorer::BuildConfiguration::BuildType xmakeBuildTypeToBuildType(const BuildType &in);

private:
        static ProjectExplorer::BuildInfo createBuildInfo(BuildType buildType);

        friend class Internal::XMakeProjectImporter;
    };

    namespace Internal { void setupXMakeBuildConfiguration(); }
} // namespace XMakeProjectManager
