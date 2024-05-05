// Copyright (C) 2016 Canonical Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "xmake_global.h"

#include "xmakeconfigitem.h"

#include <projectexplorer/kitmanager.h>

namespace XMakeProjectManager {

class XMakeTool;

class XMAKE_EXPORT XMakeKitAspect
{
public:
    static Utils::Id id();

    static Utils::Id xmakeToolId(const ProjectExplorer::Kit *k);
    static XMakeTool *xmakeTool(const ProjectExplorer::Kit *k);
    static void setXMakeTool(ProjectExplorer::Kit *k, const Utils::Id id);
    static QString msgUnsupportedVersion(const QByteArray &versionString);

    static ProjectExplorer::KitAspect *createKitAspect(ProjectExplorer::Kit *k);

private:
    friend class XMakeToolManager;
};

class XMAKE_EXPORT XMakeGeneratorKitAspect
{
public:
    static QString generator(const ProjectExplorer::Kit *k);
    static QString platform(const ProjectExplorer::Kit *k);
    static QString toolset(const ProjectExplorer::Kit *k);
    static void setGenerator(ProjectExplorer::Kit *k, const QString &generator);
    static void setPlatform(ProjectExplorer::Kit *k, const QString &platform);
    static void setToolset(ProjectExplorer::Kit *k, const QString &toolset);
    static void set(ProjectExplorer::Kit *k, const QString &generator,
                    const QString &platform, const QString &toolset);
    static QStringList generatorArguments(const ProjectExplorer::Kit *k);
    static XMakeConfig generatorXMakeConfig(const ProjectExplorer::Kit *k);
    static bool isMultiConfigGenerator(const ProjectExplorer::Kit *k);

    static ProjectExplorer::KitAspect *createKitAspect(ProjectExplorer::Kit *k);
};

class XMAKE_EXPORT XMakeConfigurationKitAspect
{
public:
    static XMakeConfig configuration(const ProjectExplorer::Kit *k);
    static void setConfiguration(ProjectExplorer::Kit *k, const XMakeConfig &config);

    static QString additionalConfiguration(const ProjectExplorer::Kit *k);
    static void setAdditionalConfiguration(ProjectExplorer::Kit *k, const QString &config);

    static QStringList toStringList(const ProjectExplorer::Kit *k);
    static void fromStringList(ProjectExplorer::Kit *k, const QStringList &in);

    static QStringList toArgumentsList(const ProjectExplorer::Kit *k);

    static XMakeConfig defaultConfiguration(const ProjectExplorer::Kit *k);

    static void setXMakePreset(ProjectExplorer::Kit *k, const QString &presetName);
    static XMakeConfigItem xmakePresetConfigItem(const ProjectExplorer::Kit *k);

    static ProjectExplorer::KitAspect *createKitAspect(ProjectExplorer::Kit *k);
};

namespace Internal { void setupXMakeKitAspects(); }

} // XMakeProjectManager
