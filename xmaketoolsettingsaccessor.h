// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/id.h>
#include <utils/settingsaccessor.h>
#include <utils/store.h>

namespace XMakeProjectManager {

class XMakeTool;

namespace Internal {

class XMakeToolSettingsAccessor : public Utils::UpgradingSettingsAccessor
{
public:
    XMakeToolSettingsAccessor();

    struct XMakeTools {
        Utils::Id defaultToolId;
        std::vector<std::unique_ptr<XMakeTool>> xmakeTools;
    };

    XMakeTools restoreXMakeTools(QWidget *parent) const;

    void saveXMakeTools(const QList<XMakeTool *> &xmakeTools,
                        const Utils::Id &defaultId,
                        QWidget *parent);

private:
    XMakeTools xmakeTools(const Utils::Store &data, bool fromSdk) const;
};

} // namespace Internal
} // namespace XMakeProjectManager
