// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "settings.h"

#include "XmakeProjectManagerForQtCreatorconstants.h"
#include "XmakeProjectManagerForQtCreatortr.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/layoutbuilder.h>

namespace XmakeProjectManagerForQtCreator::Internal {
    XmakeSettings &settings() {
        static XmakeSettings theSettings;
        return theSettings;
    }

    XmakeSettings::XmakeSettings() {
        setAutoApply(false);
        setSettingsGroup("XmakeProjectManager");

        autorunXmake.setSettingsKey("Xmake.autorun");
        autorunXmake.setLabelText(Tr::tr("Autorun Xmake"));
        autorunXmake.setToolTip(Tr::tr("Automatically run Xmake when needed."));

        verboseNinja.setSettingsKey("ninja.verbose");
        verboseNinja.setLabelText(Tr::tr("Ninja verbose mode"));
        verboseNinja.setToolTip(Tr::tr("Enables verbose mode by default when invoking Ninja."));

        setLayouter([this] {
                        using namespace Layouting;
                        return Column {
                            autorunXmake,
                            verboseNinja,
                            st,
                        };
                    });

        readSettings();
    }

    class MesonSettingsPage final : public Core::IOptionsPage {
public:
        MesonSettingsPage() {
            setId("A.MesonProjectManager.SettingsPage.General");
            setDisplayName(Tr::tr("General"));
            setDisplayCategory(Tr::tr("Xmake"));
            setCategory(Constants::SettingsPage::CATEGORY);
            setCategoryIconPath(Constants::Icons::MESON_BW);
            setSettingsProvider([] {
                                    return &settings();
                                });
        }
    };

    const MesonSettingsPage settingsPage;
} // MesonProjectManager::Internal
