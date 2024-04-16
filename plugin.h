#pragma once

#include "plugin_global.h"

#include <extensionsystem/iplugin.h>

namespace Plugin::Internal {

class PluginPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Plugin.json")

public:
    PluginPlugin();
    ~PluginPlugin() override;

    void initialize() override;
    void extensionsInitialized() override;
    ShutdownFlag aboutToShutdown() override;

private:
    void triggerAction();
};

} // namespace Plugin::Internal
