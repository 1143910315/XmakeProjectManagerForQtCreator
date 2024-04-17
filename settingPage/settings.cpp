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

        xmakePath.setSettingsKey("Xmake.path");
        xmakePath.setLabelText(Tr::tr("Xmake path"));
        xmakePath.setToolTip(Tr::tr("Xmake path."));

        setLayouter([this] {
                        using namespace Layouting;
                        return Column {
                            xmakePath,
                            st,
                        };
                    });

        readSettings();
    }

    class XmakeSettingsPage final : public Core::IOptionsPage {
public:
        XmakeSettingsPage() {
            setId("A.XmakeProjectManager.SettingsPage.General");
            setDisplayName(Tr::tr("General"));
            setDisplayCategory(Tr::tr("Xmake"));
            setCategory(Constants::SettingsPage::CATEGORY);
            setCategoryIconPath(Constants::Icons::MESON_BW);
            setSettingsProvider([] {
                                    return &settings();
                                });
        }
    };

    const XmakeSettingsPage settingsPage;
} // MesonProjectManager::Internal
