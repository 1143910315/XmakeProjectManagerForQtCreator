// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "xmakespecificsettings.h"

#include "xmakeprojectconstants.h"
#include "xmakeprojectmanagertr.h"

#include <coreplugin/icore.h>
#include <coreplugin/dialogs/ioptionspage.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>

using namespace Utils;

namespace XMakeProjectManager::Internal {

XMakeSpecificSettings &settings()
{
    static XMakeSpecificSettings theSettings;
    return theSettings;
}

XMakeSpecificSettings::XMakeSpecificSettings()
{
    setLayouter([this] {
        using namespace Layouting;
        return Column {
            autorunXMake,
            packageManagerAutoSetup,
            askBeforeReConfigureInitialParams,
            askBeforePresetsReload,
            showSourceSubFolders,
            showAdvancedOptionsByDefault,
            useJunctionsForSourceAndBuildDirectories,
            st
        };
    });

    // TODO: fixup of QTCREATORBUG-26289 , remove in Qt Creator 7 or so
    Core::ICore::settings()->remove("XMakeSpecificSettings/NinjaPath");

    setSettingsGroup("XMakeSpecificSettings");
    setAutoApply(false);

    autorunXMake.setSettingsKey("AutorunXMake");
    autorunXMake.setDefaultValue(true);
    autorunXMake.setLabelText(::XMakeProjectManager::Tr::tr("Autorun XMake"));
    autorunXMake.setToolTip(::XMakeProjectManager::Tr::tr(
        "Automatically run XMake after changes to XMake project files."));

    ninjaPath.setSettingsKey("NinjaPath");
    // never save this to the settings:
    ninjaPath.setToSettingsTransformation(
        [](const QVariant &) { return QVariant::fromValue(QString()); });
    ninjaPath.setFromSettingsTransformation([](const QVariant &from) {
        // Sometimes the installer appends the same ninja path to the qtcreator.ini file
        const QString path = from.canConvert<QStringList>() ? from.toStringList().last()
                                                            : from.toString();
        return FilePath::fromUserInput(path).toVariant();
    });

    packageManagerAutoSetup.setSettingsKey("PackageManagerAutoSetup");
    packageManagerAutoSetup.setDefaultValue(true);
    packageManagerAutoSetup.setLabelText(::XMakeProjectManager::Tr::tr("Package manager auto setup"));
    packageManagerAutoSetup.setToolTip(::XMakeProjectManager::Tr::tr("Add the XMAKE_PROJECT_INCLUDE_BEFORE variable "
        "pointing to a XMake script that will install dependencies from the conanfile.txt, "
        "conanfile.py, or vcpkg.json file from the project source directory."));

    askBeforeReConfigureInitialParams.setSettingsKey("AskReConfigureInitialParams");
    askBeforeReConfigureInitialParams.setDefaultValue(true);
    askBeforeReConfigureInitialParams.setLabelText(::XMakeProjectManager::Tr::tr("Ask before re-configuring with "
        "initial parameters"));

    askBeforePresetsReload.setSettingsKey("AskBeforePresetsReload");
    askBeforePresetsReload.setDefaultValue(true);
    askBeforePresetsReload.setLabelText(::XMakeProjectManager::Tr::tr("Ask before reloading XMake Presets"));

    showSourceSubFolders.setSettingsKey("ShowSourceSubFolders");
    showSourceSubFolders.setDefaultValue(true);
    showSourceSubFolders.setLabelText(
                ::XMakeProjectManager::Tr::tr("Show subfolders inside source group folders"));

    showAdvancedOptionsByDefault.setSettingsKey("ShowAdvancedOptionsByDefault");
    showAdvancedOptionsByDefault.setDefaultValue(false);
    showAdvancedOptionsByDefault.setLabelText(
                ::XMakeProjectManager::Tr::tr("Show advanced options by default"));

    useJunctionsForSourceAndBuildDirectories.setSettingsKey(
        "UseJunctionsForSourceAndBuildDirectories");
    useJunctionsForSourceAndBuildDirectories.setDefaultValue(false);
    useJunctionsForSourceAndBuildDirectories.setLabelText(::XMakeProjectManager::Tr::tr(
        "Use junctions for XMake configuration and build operations"));
    useJunctionsForSourceAndBuildDirectories.setVisible(Utils::HostOsInfo().isWindowsHost());
    useJunctionsForSourceAndBuildDirectories.setToolTip(::XMakeProjectManager::Tr::tr(
        "Create and use junctions for the source and build directories to overcome "
        "issues with long paths on Windows.<br><br>"
        "Junctions are stored under <tt>C:\\ProgramData\\QtCreator\\Links</tt> (overridable via "
        "the <tt>QTC_XMAKE_JUNCTIONS_DIR</tt> environment variable).<br><br>"
        "With <tt>QTC_XMAKE_JUNCTIONS_HASH_LENGTH</tt>, you can shorten the MD5 hash key length "
        "to a value smaller than the default length value of 32.<br><br>"
        "Junctions are used for XMake configure, build and install operations."));

    readSettings();
}

class XMakeSpecificSettingsPage final : public Core::IOptionsPage
{
public:
    XMakeSpecificSettingsPage()
    {
        setId(Constants::Settings::GENERAL_ID);
        setDisplayName(::XMakeProjectManager::Tr::tr("General"));
        setDisplayCategory("XMake");
        setCategory(Constants::Settings::CATEGORY);
        setCategoryIconPath(Constants::Icons::SETTINGS_CATEGORY);
        setSettingsProvider([] { return &settings(); });
    }
};

const XMakeSpecificSettingsPage settingsPage;

} // XMakeProjectManager::Internal
