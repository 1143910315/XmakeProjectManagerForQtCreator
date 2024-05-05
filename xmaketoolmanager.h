// Copyright (C) 2016 Canonical Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "xmake_global.h"

#include "xmaketool.h"

#include <utils/filepath.h>
#include <utils/id.h>

#include <QObject>

#include <memory>

namespace XMakeProjectManager {

class XMAKE_EXPORT XMakeToolManager : public QObject
{
    Q_OBJECT
public:
    XMakeToolManager();
    ~XMakeToolManager();

    static XMakeToolManager *instance();

    static QList<XMakeTool *> xmakeTools();

    static bool registerXMakeTool(std::unique_ptr<XMakeTool> &&tool);
    static void deregisterXMakeTool(const Utils::Id &id);

    static XMakeTool *defaultProjectOrDefaultXMakeTool();

    static XMakeTool *defaultXMakeTool();
    static void setDefaultXMakeTool(const Utils::Id &id);
    static XMakeTool *findByCommand(const Utils::FilePath &command);
    static XMakeTool *findById(const Utils::Id &id);

    static void notifyAboutUpdate(XMakeTool *);
    static void restoreXMakeTools();

    static void updateDocumentation();

    static QString toolTipForRstHelpFile(const Utils::FilePath &helpFile);

    static Utils::FilePath mappedFilePath(const Utils::FilePath &path);

public slots:
    QList<Utils::Id> autoDetectXMakeForDevice(const Utils::FilePaths &searchPaths,
                                  const QString &detectionSource,
                                  QString *logMessage);
    Utils::Id registerXMakeByPath(const Utils::FilePath &xmakePath,
                             const QString &detectionSource);
    void removeDetectedXMake(const QString &detectionSource, QString *logMessage);
    void listDetectedXMake(const QString &detectionSource, QString *logMessage);

signals:
    void xmakeAdded (const Utils::Id &id);
    void xmakeRemoved (const Utils::Id &id);
    void xmakeUpdated (const Utils::Id &id);
    void xmakeToolsChanged ();
    void xmakeToolsLoaded ();
    void defaultXMakeChanged ();

private:
    static void saveXMakeTools();
    static void ensureDefaultXMakeToolIsValid();
};

namespace Internal { void setupXMakeToolManager(QObject *guard); }

} // namespace XMakeProjectManager

Q_DECLARE_METATYPE(QString *)
