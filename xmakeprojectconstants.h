// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace XMakeProjectManager {
namespace Constants {

const char XMAKE_EDITOR_ID[] = "XMakeProject.XMakeEditor";
const char RUN_XMAKE[] = "XMakeProject.RunXMake";
const char RUN_XMAKE_PROFILER[] = "XMakeProject.RunXMakeProfiler";
const char RUN_XMAKE_DEBUGGER[] = "XMakeProject.RunXMakeDebugger";
const char CLEAR_XMAKE_CACHE[] = "XMakeProject.ClearCache";
const char RESCAN_PROJECT[] = "XMakeProject.RescanProject";
const char RUN_XMAKE_CONTEXT_MENU[] = "XMakeProject.RunXMakeContextMenu";
const char BUILD_FILE_CONTEXT_MENU[] = "XMakeProject.BuildFileContextMenu";
const char BUILD_FILE[] = "XMakeProject.BuildFile";
const char XMAKE_HOME_DIR[] = "XMakeProject.HomeDirectory";
const char QML_DEBUG_SETTING[] = "XMakeProject.EnableQmlDebugging";
const char RELOAD_XMAKE_PRESETS[] = "XMakeProject.ReloadXMakePresets";

const char XMAKEFORMATTER_SETTINGS_GROUP[] = "XMakeFormatter";
const char XMAKEFORMATTER_GENERAL_GROUP[] = "General";
const char XMAKEFORMATTER_ACTION_ID[] = "XMakeFormatter.Action";
const char XMAKEFORMATTER_MENU_ID[] = "XMakeFormatter.Menu";
const char XMAKE_DEBUGGING_GROUP[] = "Debugger.Group.XMake";

const char PACKAGE_MANAGER_DIR[] = ".qtc/package-manager";

const char XMAKE_LISTS_TXT[] = "XMakeLists.txt";
const char XMAKE_CACHE_TXT[] = "XMakeCache.txt";
const char XMAKE_CACHE_TXT_PREV[] = "XMakeCache.txt.prev";

// Project
const char XMAKE_PROJECT_ID[] = "XMakeProjectManager.XMakeProject";

const char XMAKE_BUILDCONFIGURATION_ID[] = "XMakeProjectManager.XMakeBuildConfiguration";

// Menu
const char M_CONTEXT[] = "XMakeEditor.ContextMenu";

namespace Settings {
const char GENERAL_ID[] = "XMakeSpecifcSettings";
const char TOOLS_ID[] = "K.XMake.Tools";
const char FORMATTER_ID[] = "K.XMake.Formatter";
const char CATEGORY[] = "K.XMake";
} // namespace Settings

// Snippets
const char XMAKE_SNIPPETS_GROUP_ID[] = "XMake";

namespace Icons {
const char FILE_OVERLAY[] = ":/xmakeproject/images/fileoverlay_xmake.png";
const char SETTINGS_CATEGORY[] = ":/xmakeproject/images/settingscategory_xmakeprojectmanager.png";
} // namespace Icons

// Actions
const char BUILD_TARGET_CONTEXT_MENU[] = "XMake.BuildTargetContextMenu";

// Build Step
const char XMAKE_BUILD_STEP_ID[] = "XMakeProjectManager.MakeStep";

// Install Step
const char XMAKE_INSTALL_STEP_ID[] = "XMakeProjectManager.InstallStep";


// Features
const char XMAKE_FEATURE_ID[] = "XMakeProjectManager.Wizard.FeatureXMake";

// Tool
const char TOOL_ID[] = "XMakeProjectManager.XMakeKitInformation";

// Data
const char BUILD_FOLDER_ROLE[] = "XMakeProjectManager.data.buildFolder";

// Output
const char OUTPUT_PREFIX[] = "[xmake] ";

} // namespace Constants
} // namespace XMakeProjectManager
