// Copyright (C) 2016 Canonical Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "xmaketoolmanager.h"

#include "xmakekitaspect.h"
#include "xmakeprojectmanagertr.h"
#include "xmakespecificsettings.h"
#include "xmaketoolsettingsaccessor.h"

#include "3rdparty/rstparser/rstparser.h"

#include <extensionsystem/pluginmanager.h>

#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/target.h>

#include <utils/environment.h>
#include <utils/pointeralgorithm.h>
#include <utils/qtcassert.h>

#include <nanotrace/nanotrace.h>

#include <QCryptographicHash>
#include <QStandardPaths>
#include <stack>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <winioctl.h>

// taken from qtbase/src/corelib/io/qfilesystemengine_win.cpp
#if !defined(REPARSE_DATA_BUFFER_HEADER_SIZE)
typedef struct _REPARSE_DATA_BUFFER {
    ULONG  ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;
    union {
        struct {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            ULONG  Flags;
            WCHAR  PathBuffer[1];
        } SymbolicLinkReparseBuffer;
        struct {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            WCHAR  PathBuffer[1];
        } MountPointReparseBuffer;
        struct {
            UCHAR  DataBuffer[1];
        } GenericReparseBuffer;
    };
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;
#  define REPARSE_DATA_BUFFER_HEADER_SIZE  FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer)
#endif // !defined(REPARSE_DATA_BUFFER_HEADER_SIZE)

#ifndef FSCTL_SET_REPARSE_POINT
#define FSCTL_SET_REPARSE_POINT CTL_CODE(FILE_DEVICE_FILE_SYSTEM,41,METHOD_BUFFERED,FILE_ANY_ACCESS)
#endif
#endif

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace XMakeProjectManager {

#ifdef Q_OS_WIN
static Q_LOGGING_CATEGORY(xmakeToolManagerLog, "qtc.xmake.toolmanager", QtWarningMsg);
#endif

class XMakeToolManagerPrivate
{
public:
    Id m_defaultXMake;
    std::vector<std::unique_ptr<XMakeTool>> m_xmakeTools;
    Internal::XMakeToolSettingsAccessor m_accessor;
    FilePath m_junctionsDir;
    int m_junctionsHashLength = 32;

    XMakeToolManagerPrivate();
};

class HtmlHandler : public rst::ContentHandler
{
private:
    std::stack<QString> m_tags;

    QStringList m_p;
    QStringList m_h3;
    QStringList m_xmake_code;

    QString m_last_directive_type;
    QString m_last_directive_class;

    void StartBlock(rst::BlockType type) final
    {
        QString tag;
        switch (type) {
        case rst::REFERENCE_LINK:
            // not used, HandleReferenceLink is used instead
            break;
        case rst::H1:
            tag = "h1";
            break;
        case rst::H2:
            tag = "h2";
            break;
        case rst::H3:
            tag = "h3";
            break;
        case rst::H4:
            tag = "h4";
            break;
        case rst::H5:
            tag = "h5";
            break;
        case rst::CODE:
            tag = "code";
            break;
        case rst::PARAGRAPH:
            tag = "p";
            break;
        case rst::LINE_BLOCK:
            tag = "pre";
            break;
        case rst::BLOCK_QUOTE:
            if (m_last_directive_type == "code-block" && m_last_directive_class == "xmake")
                tag = "xmake-code";
            else
                tag = "blockquote";
            break;
        case rst::BULLET_LIST:
            tag = "ul";
            break;
        case rst::LIST_ITEM:
            tag = "li";
            break;
        case rst::LITERAL_BLOCK:
            tag = "pre";
            break;
        }

        if (tag == "p")
            m_p.push_back(QString());
        if (tag == "h3")
            m_h3.push_back(QString());
        if (tag == "xmake-code")
            m_xmake_code.push_back(QString());

        if (tag == "code" && m_tags.top() == "p")
            m_p.last().append("`");

        m_tags.push(tag);
    }

    void EndBlock() final
    {
        // Add a new "p" collector for any `code` markup that comes afterwads
        // since we are insterested only in the first paragraph.
        if (m_tags.top() == "p")
            m_p.push_back(QString());

        if (m_tags.top() == "code" && !m_p.isEmpty()) {
            m_tags.pop();

            if (m_tags.size() > 0 && m_tags.top() == "p")
                m_p.last().append("`");
        } else {
            m_tags.pop();
        }
    }

    void HandleText(const char *text, std::size_t size) final
    {
        if (m_last_directive_type.endsWith("replace"))
            return;

        QString str = QString::fromUtf8(text, size);

        if (m_tags.top() == "h3")
            m_h3.last().append(str);
        if (m_tags.top() == "p")
            m_p.last().append(str);
        if (m_tags.top() == "xmake-code")
            m_xmake_code.last().append(str);
        if (m_tags.top() == "code" && !m_p.isEmpty())
            m_p.last().append(str);
    }

    void HandleDirective(const std::string &type, const std::string &name) final
    {
        m_last_directive_type = QString::fromStdString(type);
        m_last_directive_class = QString::fromStdString(name);
    }

    void HandleReferenceLink(const std::string &type, const std::string &text) final
    {
        Q_UNUSED(type)
        if (!m_p.isEmpty())
            m_p.last().append(QString::fromStdString(text));
    }

public:
    QString content() const
    {
        const QString title = m_h3.isEmpty() ? QString() : m_h3.first();
        const QString description = m_p.isEmpty() ? QString() : m_p.first();
        const QString xmakeCode = m_xmake_code.isEmpty() ? QString() : m_xmake_code.first();

        return QString("### %1\n\n%2\n\n````\n%3\n````").arg(title, description, xmakeCode);
    }
};

static XMakeToolManagerPrivate *d = nullptr;
static XMakeToolManager *m_instance = nullptr;

XMakeToolManager::XMakeToolManager()
{
    qRegisterMetaType<QString *>();

    d = new XMakeToolManagerPrivate;
    connect(ICore::instance(), &ICore::saveSettingsRequested,
            this, &XMakeToolManager::saveXMakeTools);

    connect(this, &XMakeToolManager::xmakeAdded, this, &XMakeToolManager::xmakeToolsChanged);
    connect(this, &XMakeToolManager::xmakeRemoved, this, &XMakeToolManager::xmakeToolsChanged);
    connect(this, &XMakeToolManager::xmakeUpdated, this, &XMakeToolManager::xmakeToolsChanged);

    setObjectName("XMakeToolManager");
    ExtensionSystem::PluginManager::addObject(this);
}

XMakeToolManager::~XMakeToolManager()
{
    ExtensionSystem::PluginManager::removeObject(this);
    delete d;
}

XMakeToolManager *XMakeToolManager::instance()
{
    return m_instance;
}

QList<XMakeTool *> XMakeToolManager::xmakeTools()
{
    return Utils::toRawPointer<QList>(d->m_xmakeTools);
}

bool XMakeToolManager::registerXMakeTool(std::unique_ptr<XMakeTool> &&tool)
{
    if (!tool || Utils::contains(d->m_xmakeTools, tool.get()))
        return true;

    const Utils::Id toolId = tool->id();
    QTC_ASSERT(toolId.isValid(),return false);

    //make sure the same id was not used before
    QTC_ASSERT(!Utils::contains(d->m_xmakeTools, [toolId](const std::unique_ptr<XMakeTool> &known) {
        return toolId == known->id();
    }), return false);

    d->m_xmakeTools.emplace_back(std::move(tool));

    emit m_instance->xmakeAdded(toolId);

    ensureDefaultXMakeToolIsValid();

    updateDocumentation();

    return true;
}

void XMakeToolManager::deregisterXMakeTool(const Id &id)
{
    auto toRemove = Utils::take(d->m_xmakeTools, Utils::equal(&XMakeTool::id, id));
    if (toRemove.has_value()) {
        ensureDefaultXMakeToolIsValid();

        updateDocumentation();

        emit m_instance->xmakeRemoved(id);
    }
}

XMakeTool *XMakeToolManager::defaultProjectOrDefaultXMakeTool()
{
    XMakeTool *tool = nullptr;

    if (auto bs = ProjectExplorer::ProjectTree::currentBuildSystem())
        tool = XMakeKitAspect::xmakeTool(bs->target()->kit());
    if (!tool)
        tool = XMakeToolManager::defaultXMakeTool();

    return tool;
}

XMakeTool *XMakeToolManager::defaultXMakeTool()
{
    return findById(d->m_defaultXMake);
}

void XMakeToolManager::setDefaultXMakeTool(const Id &id)
{
    if (d->m_defaultXMake != id && findById(id)) {
        d->m_defaultXMake = id;
        emit m_instance->defaultXMakeChanged();
        return;
    }

    ensureDefaultXMakeToolIsValid();
}

XMakeTool *XMakeToolManager::findByCommand(const Utils::FilePath &command)
{
    return Utils::findOrDefault(d->m_xmakeTools, Utils::equal(&XMakeTool::xmakeExecutable, command));
}

XMakeTool *XMakeToolManager::findById(const Id &id)
{
    return Utils::findOrDefault(d->m_xmakeTools, Utils::equal(&XMakeTool::id, id));
}

void XMakeToolManager::restoreXMakeTools()
{
    NANOTRACE_SCOPE("XMakeProjectManager", "XMakeToolManager::restoreXMakeTools");
    Internal::XMakeToolSettingsAccessor::XMakeTools tools
            = d->m_accessor.restoreXMakeTools(ICore::dialogParent());
    d->m_xmakeTools = std::move(tools.xmakeTools);
    setDefaultXMakeTool(tools.defaultToolId);

    updateDocumentation();

    emit m_instance->xmakeToolsLoaded();

    // Store the default XMake tool "Autorun XMake" value globally
    // TODO: Remove in Qt Creator 13
    Internal::XMakeSpecificSettings &s = Internal::settings();
    if (s.autorunXMake() == s.autorunXMake.defaultValue()) {
        XMakeTool *xmake = defaultXMakeTool();
        s.autorunXMake.setValue(xmake ? xmake->isAutoRun() : true);
        s.writeSettings();
    }
}

void XMakeToolManager::updateDocumentation()
{
    const QList<XMakeTool *> tools = xmakeTools();
    QStringList docs;
    for (const auto tool : tools) {
        if (!tool->qchFilePath().isEmpty())
            docs.append(tool->qchFilePath().toString());
    }
    Core::HelpManager::registerDocumentation(docs);
}

static void createJunction(const FilePath &from, const FilePath &to)
{
#ifdef Q_OS_WIN
    to.createDir();
    const QString toString = to.path();

    HANDLE handle = ::CreateFile((wchar_t *) toString.utf16(),
                                 GENERIC_WRITE,
                                 0,
                                 nullptr,
                                 OPEN_EXISTING,
                                 FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                                 nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        qCDebug(xmakeToolManagerLog())
            << "Failed to open" << toString << "to create a junction." << ::GetLastError();
        return;
    }

    QString fromString("\\??\\");
    fromString.append(from.absoluteFilePath().nativePath());

    auto fromStringLength = uint16_t(fromString.length() * sizeof(wchar_t));
    auto toStringLength = uint16_t(toString.length() * sizeof(wchar_t));
    auto reparseDataLength = fromStringLength + toStringLength + 12;

    std::vector<char> buf(reparseDataLength + REPARSE_DATA_BUFFER_HEADER_SIZE, 0);
    REPARSE_DATA_BUFFER &reparse = *reinterpret_cast<REPARSE_DATA_BUFFER *>(buf.data());

    reparse.ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
    reparse.ReparseDataLength = reparseDataLength;

    reparse.MountPointReparseBuffer.SubstituteNameOffset = 0;
    reparse.MountPointReparseBuffer.SubstituteNameLength = fromStringLength;
    fromString.toWCharArray(reparse.MountPointReparseBuffer.PathBuffer);

    reparse.MountPointReparseBuffer.PrintNameOffset = fromStringLength + sizeof(UNICODE_NULL);
    reparse.MountPointReparseBuffer.PrintNameLength = toStringLength;
    toString.toWCharArray(reparse.MountPointReparseBuffer.PathBuffer + fromString.length() + 1);

    DWORD retsize = 0;
    if (!::DeviceIoControl(handle,
                           FSCTL_SET_REPARSE_POINT,
                           &reparse,
                           uint16_t(buf.size()),
                           nullptr,
                           0,
                           &retsize,
                           nullptr)) {
        qCDebug(xmakeToolManagerLog()) << "Failed to create junction from" << fromString << "to"
                                       << toString << "GetLastError:" << ::GetLastError();
    }
    ::CloseHandle(handle);
#else
    Q_UNUSED(from)
    Q_UNUSED(to)
#endif
}

QString XMakeToolManager::toolTipForRstHelpFile(const FilePath &helpFile)
{
    static QHash<FilePath, QString> map;
    static QMutex mutex;
    QMutexLocker locker(&mutex);

    if (map.contains(helpFile))
        return map.value(helpFile);

    auto content = helpFile.fileContents(1024).value_or(QByteArray());
    content.replace("\r\n", "\n");

    HtmlHandler handler;
    rst::Parser parser(&handler);
    parser.Parse(content.left(content.lastIndexOf('\n')));

    const QString tooltip = handler.content();

    map[helpFile] = tooltip;
    return tooltip;
}

FilePath XMakeToolManager::mappedFilePath(const FilePath &path)
{
    if (!HostOsInfo::isWindowsHost())
        return path;

    if (path.needsDevice())
        return path;

    auto project = ProjectManager::startupProject();
    auto environment = Environment::systemEnvironment();
    if (project)
        environment.modify(project->additionalEnvironment());
    const bool enableJunctions
        = QVariant(
              environment.value_or("QTC_XMAKE_USE_JUNCTIONS",
                                   Internal::settings().useJunctionsForSourceAndBuildDirectories()
                                       ? "1"
                                       : "0"))
              .toBool();

    if (!enableJunctions)
        return path;

    if (!d->m_junctionsDir.isDir())
        return path;

    const auto hashPath = QString::fromUtf8(
        QCryptographicHash::hash(path.path().toUtf8(), QCryptographicHash::Md5).toHex(0));
    const auto fullHashPath = d->m_junctionsDir.pathAppended(
        hashPath.left(d->m_junctionsHashLength));

    if (!fullHashPath.exists())
        createJunction(path, fullHashPath);

    return fullHashPath.exists() ? fullHashPath : path;
}

QList<Id> XMakeToolManager::autoDetectXMakeForDevice(const FilePaths &searchPaths,
                                                const QString &detectionSource,
                                                QString *logMessage)
{
    QList<Id> result;
    QStringList messages{Tr::tr("Searching XMake binaries...")};
    for (const FilePath &path : searchPaths) {
        const FilePath xmake = path.pathAppended("xmake").withExecutableSuffix();
        if (xmake.isExecutableFile()) {
            const Id currentId = registerXMakeByPath(xmake, detectionSource);
            if (currentId.isValid())
                result.push_back(currentId);
            messages.append(Tr::tr("Found \"%1\"").arg(xmake.toUserOutput()));
        }
    }
    if (logMessage)
        *logMessage = messages.join('\n');

    return result;
}


Id XMakeToolManager::registerXMakeByPath(const FilePath &xmakePath, const QString &detectionSource)
{
    Id id = Id::fromString(xmakePath.toUserOutput());

    XMakeTool *xmakeTool = findById(id);
    if (xmakeTool)
        return xmakeTool->id();

    auto newTool = std::make_unique<XMakeTool>(XMakeTool::ManualDetection, id);
    newTool->setFilePath(xmakePath);
    newTool->setDetectionSource(detectionSource);
    newTool->setDisplayName(xmakePath.toUserOutput());
    id = newTool->id();
    registerXMakeTool(std::move(newTool));

    return id;
}

void XMakeToolManager::removeDetectedXMake(const QString &detectionSource, QString *logMessage)
{
    QStringList logMessages{Tr::tr("Removing XMake entries...")};
    while (true) {
        auto toRemove = Utils::take(d->m_xmakeTools, Utils::equal(&XMakeTool::detectionSource, detectionSource));
        if (!toRemove.has_value())
            break;
        logMessages.append(Tr::tr("Removed \"%1\"").arg((*toRemove)->displayName()));
        emit m_instance->xmakeRemoved((*toRemove)->id());
    }

    ensureDefaultXMakeToolIsValid();
    updateDocumentation();
    if (logMessage)
        *logMessage = logMessages.join('\n');
}

void XMakeToolManager::listDetectedXMake(const QString &detectionSource, QString *logMessage)
{
    QTC_ASSERT(logMessage, return);
    QStringList logMessages{Tr::tr("XMake:")};
    for (const auto &tool : std::as_const(d->m_xmakeTools)) {
        if (tool->detectionSource() == detectionSource)
            logMessages.append(tool->displayName());
    }
    *logMessage = logMessages.join('\n');
}

void XMakeToolManager::notifyAboutUpdate(XMakeTool *tool)
{
    if (!tool || !Utils::contains(d->m_xmakeTools, tool))
        return;
    emit m_instance->xmakeUpdated(tool->id());
}

void XMakeToolManager::saveXMakeTools()
{
    d->m_accessor.saveXMakeTools(xmakeTools(), d->m_defaultXMake, ICore::dialogParent());
}

void XMakeToolManager::ensureDefaultXMakeToolIsValid()
{
    const Utils::Id oldId = d->m_defaultXMake;
    if (d->m_xmakeTools.size() == 0) {
        d->m_defaultXMake = Utils::Id();
    } else {
        if (findById(d->m_defaultXMake))
            return;
        auto xmakeTool = Utils::findOrDefault(xmakeTools(), [](XMakeTool *tool) {
            return tool->detectionSource().isEmpty() && !tool->xmakeExecutable().needsDevice();
        });
        if (xmakeTool)
            d->m_defaultXMake = xmakeTool->id();
    }

    // signaling:
    if (oldId != d->m_defaultXMake)
        emit m_instance->defaultXMakeChanged();
}

void Internal::setupXMakeToolManager(QObject *guard)
{
    m_instance = new XMakeToolManager;
    m_instance->setParent(guard);
}

XMakeToolManagerPrivate::XMakeToolManagerPrivate()
{
    if (HostOsInfo::isWindowsHost()) {
        const QStringList locations = QStandardPaths::standardLocations(
            QStandardPaths::GenericConfigLocation);
        m_junctionsDir = FilePath::fromString(*std::min_element(locations.begin(), locations.end()))
                             .pathAppended("QtCreator/Links");

        auto project = ProjectManager::startupProject();
        auto environment = Environment::systemEnvironment();
        if (project)
            environment.modify(project->additionalEnvironment());

        if (environment.hasKey("QTC_XMAKE_JUNCTIONS_DIR"))
            m_junctionsDir = FilePath::fromUserInput(environment.value("QTC_XMAKE_JUNCTIONS_DIR"));

        if (environment.hasKey("QTC_XMAKE_JUNCTIONS_HASH_LENGTH")) {
            bool ok = false;
            const int hashLength = environment.value("QTC_XMAKE_JUNCTIONS_HASH_LENGTH").toInt(&ok);
            if (ok && hashLength >= 4 && hashLength < 32)
                m_junctionsHashLength = hashLength;
        }
        if (!m_junctionsDir.exists())
            m_junctionsDir.createDir();
    }
}

} // XMakeProjectManager
