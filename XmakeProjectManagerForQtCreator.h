#pragma once

#include "XmakeProjectManagerForQtCreator_global.h"

#include <extensionsystem/iplugin.h>

namespace XmakeProjectManagerForQtCreator::Internal {
    class XmakeProjectManagerForQtCreatorPlugin : public ExtensionSystem::IPlugin {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "XmakeProjectManagerForQtCreator.json")

public:
        XmakeProjectManagerForQtCreatorPlugin();
        ~XmakeProjectManagerForQtCreatorPlugin() override;

        void initialize() override;
        void extensionsInitialized() override;
        ShutdownFlag aboutToShutdown() override;

private:
        void triggerAction();
    };
} // namespace XmakeProjectManagerForQtCreator::Internal
