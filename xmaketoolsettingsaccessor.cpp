// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "xmaketoolsettingsaccessor.h"

#include "xmakeprojectmanagertr.h"
#include "xmakespecificsettings.h"
#include "xmaketool.h"

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/environment.h>

#include <QDebug>
#include <QGuiApplication>

using namespace Utils;

namespace XMakeProjectManager::Internal {

// --------------------------------------------------------------------
// XMakeToolSettingsUpgraders:
// --------------------------------------------------------------------

class XMakeToolSettingsUpgraderV0 : public VersionUpgrader
{
    // Necessary to make Version 1 supported.
public:
    XMakeToolSettingsUpgraderV0() : VersionUpgrader(0, "4.6") { }

    // NOOP
    Store upgrade(const Store &data) final { return data; }
};

// --------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------

const char XMAKE_TOOL_COUNT_KEY[] = "XMakeTools.Count";
const char XMAKE_TOOL_DATA_KEY[] = "XMakeTools.";
const char XMAKE_TOOL_DEFAULT_KEY[] = "XMakeTools.Default";
const char XMAKE_TOOL_FILENAME[] = "xmaketools.xml";

static std::vector<std::unique_ptr<XMakeTool>> autoDetectXMakeTools()
{
    FilePaths extraDirs;

    if (HostOsInfo::isWindowsHost()) {
        for (const auto &envVar : QStringList{"ProgramFiles", "ProgramFiles(x86)", "ProgramW6432"}) {
            if (qtcEnvironmentVariableIsSet(envVar)) {
                const QString progFiles = qtcEnvironmentVariable(envVar);
                extraDirs.append(FilePath::fromUserInput(progFiles + "/XMake"));
                extraDirs.append(FilePath::fromUserInput(progFiles + "/XMake/bin"));
            }
        }
    }

    if (HostOsInfo::isMacHost()) {
        extraDirs.append("/Applications/XMake.app/Contents/bin");
        extraDirs.append("/usr/local/bin");    // homebrew intel
        extraDirs.append("/opt/homebrew/bin"); // homebrew arm
        extraDirs.append("/opt/local/bin");    // macports
    }

    const FilePaths suspects = FilePath("xmake").searchAllInPath(extraDirs);

    std::vector<std::unique_ptr<XMakeTool>> found;
    for (const FilePath &command : std::as_const(suspects)) {
        auto item = std::make_unique<XMakeTool>(XMakeTool::AutoDetection, XMakeTool::createId());
        item->setFilePath(command);
        item->setDisplayName(Tr::tr("System XMake at %1").arg(command.toUserOutput()));

        found.emplace_back(std::move(item));
    }

    return found;
}


static std::vector<std::unique_ptr<XMakeTool>>
mergeTools(std::vector<std::unique_ptr<XMakeTool>> &sdkTools,
           std::vector<std::unique_ptr<XMakeTool>> &userTools,
           std::vector<std::unique_ptr<XMakeTool>> &autoDetectedTools)
{
    std::vector<std::unique_ptr<XMakeTool>> result = std::move(sdkTools);
    while (userTools.size() > 0) {
        std::unique_ptr<XMakeTool> userTool = std::move(userTools[0]);
        userTools.erase(std::begin(userTools));

        int userToolIndex = Utils::indexOf(result, [&userTool](const std::unique_ptr<XMakeTool> &tool) {
            // Id should be sufficient, but we have older "mis-registered" docker based items.
            // Make sure that these don't override better new values from the sdk by
            // also checking the actual executable.
            return userTool->id() == tool->id() && userTool->xmakeExecutable() == tool->xmakeExecutable();
        });
        if (userToolIndex >= 0) {
            // Replace the sdk tool with the user tool, so any user changes do not get lost
            result[userToolIndex] = std::move(userTool);
        } else {
            if (userTool->isAutoDetected()
                    && !Utils::contains(autoDetectedTools, Utils::equal(&XMakeTool::xmakeExecutable,
                                                                        userTool->xmakeExecutable()))) {

                qWarning() << QString::fromLatin1("Previously SDK provided XMakeTool \"%1\" (%2) dropped.")
                              .arg(userTool->xmakeExecutable().toUserOutput(), userTool->id().toString());
                continue;
            }
            result.emplace_back(std::move(userTool));
        }
    }

    // add all the autodetected tools that are not known yet
    while (autoDetectedTools.size() > 0) {
        std::unique_ptr<XMakeTool> autoDetectedTool = std::move(autoDetectedTools[0]);
        autoDetectedTools.erase(std::begin(autoDetectedTools));

        if (!Utils::contains(result,
                             Utils::equal(&XMakeTool::xmakeExecutable, autoDetectedTool->xmakeExecutable())))
            result.emplace_back(std::move(autoDetectedTool));
    }

    return result;
}


// --------------------------------------------------------------------
// XMakeToolSettingsAccessor:
// --------------------------------------------------------------------

XMakeToolSettingsAccessor::XMakeToolSettingsAccessor()
{
    setDocType("QtCreatorXMakeTools");
    setApplicationDisplayName(QGuiApplication::applicationDisplayName());
    setBaseFilePath(Core::ICore::userResourcePath(XMAKE_TOOL_FILENAME));

    addVersionUpgrader(std::make_unique<XMakeToolSettingsUpgraderV0>());
}

XMakeToolSettingsAccessor::XMakeTools XMakeToolSettingsAccessor::restoreXMakeTools(QWidget *parent) const
{
    XMakeTools result;

    const FilePath sdkSettingsFile = Core::ICore::installerResourcePath(XMAKE_TOOL_FILENAME);

    XMakeTools sdkTools = xmakeTools(restoreSettings(sdkSettingsFile, parent), true);

    //read the tools from the user settings file
    XMakeTools userTools = xmakeTools(restoreSettings(parent), false);

    //autodetect tools
    std::vector<std::unique_ptr<XMakeTool>> autoDetectedTools = autoDetectXMakeTools();

    //filter out the tools that were stored in SDK
    std::vector<std::unique_ptr<XMakeTool>> toRegister = mergeTools(sdkTools.xmakeTools,
                                                                    userTools.xmakeTools,
                                                                    autoDetectedTools);

    // Store all tools
    for (auto it = std::begin(toRegister); it != std::end(toRegister); ++it)
        result.xmakeTools.emplace_back(std::move(*it));

    result.defaultToolId = userTools.defaultToolId.isValid() ? userTools.defaultToolId : sdkTools.defaultToolId;

    // Set default TC...
    return result;
}

void XMakeToolSettingsAccessor::saveXMakeTools(const QList<XMakeTool *> &xmakeTools,
                                               const Id &defaultId,
                                               QWidget *parent)
{
    Store data;
    data.insert(XMAKE_TOOL_DEFAULT_KEY, defaultId.toSetting());

    int count = 0;
    const bool autoRun = settings().autorunXMake();
    for (XMakeTool *item : xmakeTools) {
        Utils::FilePath fi = item->xmakeExecutable();

        // Gobal Autorun value will be set for all tools
        // TODO: Remove in Qt Creator 13
        item->setAutorun(autoRun);

        if (fi.needsDevice() || fi.isExecutableFile()) { // be graceful for device related stuff
            Store tmp = item->toMap();
            if (tmp.isEmpty())
                continue;
            data.insert(numberedKey(XMAKE_TOOL_DATA_KEY, count), variantFromStore(tmp));
            ++count;
        }
    }
    data.insert(XMAKE_TOOL_COUNT_KEY, count);

    saveSettings(data, parent);
}

XMakeToolSettingsAccessor::XMakeTools
XMakeToolSettingsAccessor::xmakeTools(const Store &data, bool fromSdk) const
{
    XMakeTools result;

    int count = data.value(XMAKE_TOOL_COUNT_KEY, 0).toInt();
    for (int i = 0; i < count; ++i) {
        const Key key = numberedKey(XMAKE_TOOL_DATA_KEY, i);
        if (!data.contains(key))
            continue;

        const Store dbMap = storeFromVariant(data.value(key));
        auto item = std::make_unique<XMakeTool>(dbMap, fromSdk);
        const FilePath xmakeExecutable = item->xmakeExecutable();
        if (item->isAutoDetected() && !xmakeExecutable.needsDevice() && !xmakeExecutable.isExecutableFile()) {
            qWarning() << QString("XMakeTool \"%1\" (%2) dropped since the command is not executable.")
                          .arg(xmakeExecutable.toUserOutput(), item->id().toString());
            continue;
        }

        result.xmakeTools.emplace_back(std::move(item));
    }

    result.defaultToolId = Id::fromSetting(data.value(XMAKE_TOOL_DEFAULT_KEY, Id().toSetting()));

    return result;
}

} // XMakeProjectManager::Internal
