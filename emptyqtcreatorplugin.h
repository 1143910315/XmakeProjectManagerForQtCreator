#pragma once

#include "emptyqtcreatorplugin_global.h"

#include <extensionsystem/iplugin.h>

namespace EmptyQtCreatorPlugin::Internal {

class EmptyQtCreatorPluginPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "EmptyQtCreatorPlugin.json")

public:
    EmptyQtCreatorPluginPlugin();
    ~EmptyQtCreatorPluginPlugin() override;

    void initialize() override;
    void extensionsInitialized() override;
    ShutdownFlag aboutToShutdown() override;

private:
    void triggerAction();
};

} // namespace EmptyQtCreatorPlugin::Internal
