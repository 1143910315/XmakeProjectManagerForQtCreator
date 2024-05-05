// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "xmaketool.h"

#include "xmakeprojectmanagertr.h"
#include "xmaketoolmanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/temporarydirectory.h>

#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QSet>
#include <QXmlStreamReader>
#include <QUuid>
#include <QMessageBox>

#include <memory>

using namespace Utils;

namespace XMakeProjectManager {
    static Q_LOGGING_CATEGORY(xmakeToolLog, "qtc.xmake.tool", QtWarningMsg);

    const char XMAKE_INFORMATION_ID[] = "Id";
    const char XMAKE_INFORMATION_COMMAND[] = "Binary";
    const char XMAKE_INFORMATION_DISPLAYNAME[] = "DisplayName";
    const char XMAKE_INFORMATION_AUTORUN[] = "AutoRun";
    const char XMAKE_INFORMATION_QCH_FILE_PATH[] = "QchFile";
// obsolete since Qt Creator 5. Kept for backward compatibility
    const char XMAKE_INFORMATION_AUTO_CREATE_BUILD_DIRECTORY[] = "AutoCreateBuildDirectory";
    const char XMAKE_INFORMATION_AUTODETECTED[] = "AutoDetected";
    const char XMAKE_INFORMATION_DETECTIONSOURCE[] = "DetectionSource";
    const char XMAKE_INFORMATION_READERTYPE[] = "ReaderType";

    bool XMakeTool::Generator::matches(const QString &n) const {
        return n == name;
    }

    namespace Internal {
        const char READER_TYPE_FILEAPI[] = "fileapi";

        static std::optional<XMakeTool::ReaderType> readerTypeFromString(const QString &input) {
            // Do not try to be clever here, just use whatever is in the string!
            if (input == READER_TYPE_FILEAPI) {
                return XMakeTool::FileApi;
            }
            return {};
        }

        static QString readerTypeToString(const XMakeTool::ReaderType &type) {
            switch (type) {
                case XMakeTool::FileApi: {
                    return QString(READER_TYPE_FILEAPI);
                }
                default: {
                    return QString();
                }
            }
        }

// --------------------------------------------------------------------
// XMakeIntrospectionData:
// --------------------------------------------------------------------

        class FileApi {
public:
            QString kind;
            std::pair<int, int> version;
        };

        class IntrospectionData {
public:
            bool m_didAttemptToRun = false;
            bool m_haveCapabilitites = true;
            bool m_haveKeywords = false;

            QList<XMakeTool::Generator> m_generators;
            XMakeKeywords m_keywords;
            QMutex m_keywordsMutex;
            QVector<FileApi> m_fileApis;
            XMakeTool::Version m_version;
        };
    } // namespace Internal

///////////////////////////
// XMakeTool
///////////////////////////
    XMakeTool::XMakeTool(Detection d, const Id &id)
        : m_id(id)
        , m_isAutoDetected(d == AutoDetection)
        , m_introspection(std::make_unique<Internal::IntrospectionData>()) {
        QTC_ASSERT(m_id.isValid(), m_id = Id::fromString(QUuid::createUuid().toString()));
    }

    XMakeTool::XMakeTool(const Store &map, bool fromSdk) :
        XMakeTool(fromSdk ? XMakeTool::AutoDetection : XMakeTool::ManualDetection,
                  Id::fromSetting(map.value(XMAKE_INFORMATION_ID))) {
        m_displayName = map.value(XMAKE_INFORMATION_DISPLAYNAME).toString();
        m_isAutoRun = map.value(XMAKE_INFORMATION_AUTORUN, true).toBool();
        m_autoCreateBuildDirectory = map.value(XMAKE_INFORMATION_AUTO_CREATE_BUILD_DIRECTORY, false).toBool();
        m_readerType = Internal::readerTypeFromString(
            map.value(XMAKE_INFORMATION_READERTYPE).toString());

        // loading a XMakeTool from SDK is always autodetection
        if (!fromSdk) {
            m_isAutoDetected = map.value(XMAKE_INFORMATION_AUTODETECTED, false).toBool();
        }
        m_detectionSource = map.value(XMAKE_INFORMATION_DETECTIONSOURCE).toString();

        setFilePath(FilePath::fromString(map.value(XMAKE_INFORMATION_COMMAND).toString()));

        m_qchFilePath = FilePath::fromSettings(map.value(XMAKE_INFORMATION_QCH_FILE_PATH));

        if (m_qchFilePath.isEmpty()) {
            m_qchFilePath = searchQchFile(m_executable);
        }
    }

    XMakeTool::~XMakeTool() = default;

    Id XMakeTool::createId() {
        return Id::fromString(QUuid::createUuid().toString());
    }

    void XMakeTool::setFilePath(const FilePath &executable) {
        if (m_executable == executable) {
            return;
        }

        m_introspection = std::make_unique<Internal::IntrospectionData>();

        m_executable = executable;
        XMakeToolManager::notifyAboutUpdate(this);
    }

    FilePath XMakeTool::filePath() const {
        return m_executable;
    }

    bool XMakeTool::isValid() const {
        if (!m_id.isValid() || !m_introspection) {
            return false;
        }

        if (!m_introspection->m_didAttemptToRun) {
            readInformation();
        }

        return m_introspection->m_haveCapabilitites && !m_introspection->m_fileApis.isEmpty();
    }

    void XMakeTool::runXMake(Process &xmake, const QStringList &args, int timeoutS) const {
        const FilePath executable = xmakeExecutable();
        xmake.setDisableUnixTerminal();
        Environment env = executable.deviceEnvironment();
        env.setupEnglishOutput();
        xmake.setEnvironment(env);
        xmake.setCommand({ executable, args });
        xmake.runBlocking(std::chrono::seconds(timeoutS));
    }

    Store XMakeTool::toMap() const {
        Store data;
        data.insert(XMAKE_INFORMATION_DISPLAYNAME, m_displayName);
        data.insert(XMAKE_INFORMATION_ID, m_id.toSetting());
        data.insert(XMAKE_INFORMATION_COMMAND, m_executable.toString());
        data.insert(XMAKE_INFORMATION_QCH_FILE_PATH, m_qchFilePath.toString());
        data.insert(XMAKE_INFORMATION_AUTORUN, m_isAutoRun);
        data.insert(XMAKE_INFORMATION_AUTO_CREATE_BUILD_DIRECTORY, m_autoCreateBuildDirectory);
        if (m_readerType) {
            data.insert(XMAKE_INFORMATION_READERTYPE,
                        Internal::readerTypeToString(m_readerType.value()));
        }
        data.insert(XMAKE_INFORMATION_AUTODETECTED, m_isAutoDetected);
        data.insert(XMAKE_INFORMATION_DETECTIONSOURCE, m_detectionSource);
        return data;
    }

    FilePath XMakeTool::xmakeExecutable() const {
        return xmakeExecutable(m_executable);
    }

    void XMakeTool::setQchFilePath(const FilePath &path) {
        m_qchFilePath = path;
    }

    FilePath XMakeTool::qchFilePath() const {
        return m_qchFilePath;
    }

    FilePath XMakeTool::xmakeExecutable(const FilePath &path) {
        if (path.osType() == OsTypeMac) {
            const QString executableString = path.toString();
            const int appIndex = executableString.lastIndexOf(".app");
            const int appCutIndex = appIndex + 4;
            const bool endsWithApp = appIndex >= 0 && appCutIndex >= executableString.size();
            const bool containsApp = appIndex >= 0 && !endsWithApp
                && executableString.at(appCutIndex) == '/';
            if (endsWithApp || containsApp) {
                const FilePath toTest = FilePath::fromString(executableString.left(appCutIndex))
                    .pathAppended("Contents/bin/xmake");
                if (toTest.exists()) {
                    return toTest.canonicalPath();
                }
            }
        }

        FilePath resolvedPath = path.canonicalPath();
        // Evil hack to make snap-packages of XMake work. See QTCREATORBUG-23376
        if (path.osType() == OsTypeLinux && resolvedPath.fileName() == "snap") {
            return path;
        }

        return resolvedPath;
    }

    bool XMakeTool::isAutoRun() const {
        return m_isAutoRun;
    }

    QList<XMakeTool::Generator> XMakeTool::supportedGenerators() const {
        return isValid() ? m_introspection->m_generators : QList<XMakeTool::Generator>();
    }

    XMakeKeywords XMakeTool::keywords() {
        if (!isValid()) {
            return {}
        }
        ;

        if (!m_introspection->m_haveKeywords && m_introspection->m_haveCapabilitites) {
            QMutexLocker locker(&m_introspection->m_keywordsMutex);
            if (m_introspection->m_haveKeywords) {
                return m_introspection->m_keywords;
            }

            Process proc;

            const FilePath findXMakeRoot = TemporaryDirectory::masterDirectoryFilePath()
                / "find-root.xmake";
            findXMakeRoot.writeFileContents("message(${XMAKE_ROOT})");

            FilePath xmakeRoot;
            runXMake(proc, { "-P", findXMakeRoot.nativePath() }, 5);
            if (proc.result() == ProcessResult::FinishedWithSuccess) {
                QStringList output = filtered(proc.allOutput().split('\n'),
                                              std::not_fn(&QString::isEmpty));
                if (output.size() > 0) {
                    xmakeRoot = FilePath::fromString(output[0]);
                }
            }

            const struct {
                const QString helpPath;
                QMap<QString, FilePath> &targetMap;
            } introspections[] = {
                // Functions
                { "Help/command", m_introspection->m_keywords.functions },
                // Properties
                { "Help/prop_dir", m_introspection->m_keywords.directoryProperties },
                { "Help/prop_sf", m_introspection->m_keywords.sourceProperties },
                { "Help/prop_test", m_introspection->m_keywords.testProperties },
                { "Help/prop_tgt", m_introspection->m_keywords.targetProperties },
                { "Help/prop_gbl", m_introspection->m_keywords.properties },
                // Variables
                { "Help/variable", m_introspection->m_keywords.variables },
                // Policies
                { "Help/policy", m_introspection->m_keywords.policies },
                // Environment Variables
                { "Help/envvar", m_introspection->m_keywords.environmentVariables },
            };
            for (auto &i : introspections) {
                const FilePaths files = xmakeRoot.pathAppended(i.helpPath)
                    .dirEntries({ { "*.rst" }, QDir::Files }, QDir::Name);
                for (const auto &filePath : files) {
                    i.targetMap[filePath.completeBaseName()] = filePath;
                }
            }

            for (const auto &map : { m_introspection->m_keywords.directoryProperties,
                                     m_introspection->m_keywords.sourceProperties,
                                     m_introspection->m_keywords.testProperties,
                                     m_introspection->m_keywords.targetProperties }) {
                m_introspection->m_keywords.properties.insert(map);
            }

            // Modules
            const FilePaths files
                = xmakeRoot.pathAppended("Help/module").dirEntries({ { "*.rst" }, QDir::Files }, QDir::Name);
            for (const FilePath &filePath : files) {
                const QString fileName = filePath.completeBaseName();
                if (fileName.startsWith("Find")) {
                    m_introspection->m_keywords.findModules[fileName.mid(4)] = filePath;
                } else {
                    m_introspection->m_keywords.includeStandardModules[fileName] = filePath;
                }
            }

            const QStringList moduleFunctions = parseSyntaxHighlightingXml();
            for (const auto &function : moduleFunctions) {
                m_introspection->m_keywords.functions[function] = FilePath();
            }

            m_introspection->m_haveKeywords = true;
        }

        return m_introspection->m_keywords;
    }

    bool XMakeTool::hasFileApi() const {
        return isValid() ? !m_introspection->m_fileApis.isEmpty() : false;
    }

    XMakeTool::Version XMakeTool::version() const {
        return isValid() ? m_introspection->m_version : XMakeTool::Version();
    }

    QString XMakeTool::versionDisplay() const {
        if (m_executable.isEmpty()) {
            return {}
        }
        ;

        if (!isValid()) {
            return Tr::tr("Version not parseable");
        }

        const Version &version = m_introspection->m_version;
        if (version.fullVersion.isEmpty()) {
            return QString::fromUtf8(version.fullVersion);
        }

        return QString("%1.%2.%3").arg(version.major).arg(version.minor).arg(version.patch);
    }

    bool XMakeTool::isAutoDetected() const {
        return m_isAutoDetected;
    }

    QString XMakeTool::displayName() const {
        return m_displayName;
    }

    void XMakeTool::setDisplayName(const QString &displayName) {
        m_displayName = displayName;
        XMakeToolManager::notifyAboutUpdate(this);
    }

    void XMakeTool::setPathMapper(const XMakeTool::PathMapper &pathMapper) {
        m_pathMapper = pathMapper;
    }

    XMakeTool::PathMapper XMakeTool::pathMapper() const {
        if (m_pathMapper) {
            return m_pathMapper;
        }
        return [](const FilePath &fn) {
                   return fn;
               };
    }

    std::optional<XMakeTool::ReaderType> XMakeTool::readerType() const {
        if (m_readerType) {
            return m_readerType; // Allow overriding the auto-detected value via .user files
        }
        // Find best possible reader type:
        if (hasFileApi()) {
            return FileApi;
        }
        return {};
    }

    FilePath XMakeTool::searchQchFile(const FilePath &executable) {
        if (executable.isEmpty() || executable.needsDevice()) { // do not register docs from devices
            return {}
        }
        ;

        FilePath prefixDir = executable.parentDir().parentDir();
        QDir docDir { prefixDir.pathAppended("doc/xmake").toString() };
        if (!docDir.exists()) {
            docDir.setPath(prefixDir.pathAppended("share/doc/xmake").toString());
        }
        if (!docDir.exists()) {
            return {}
        }
        ;

        const QStringList files = docDir.entryList(QStringList("*.qch"));
        for (const QString &docFile : files) {
            if (docFile.startsWith("xmake", Qt::CaseInsensitive)) {
                return FilePath::fromString(docDir.absoluteFilePath(docFile));
            }
        }

        return {};
    }

    QString XMakeTool::documentationUrl(const Version &version, bool online) {
        if (online) {
            QString helpVersion = "latest";
            if (!(version.major == 0 && version.minor == 0)) {
                helpVersion = QString("v%1.%2").arg(version.major).arg(version.minor);
            }

            return QString("https://xmake.org/xmake/help/%1").arg(helpVersion);
        }

        return QString("qthelp://org.xmake.%1.%2.%3/doc")
               .arg(version.major)
               .arg(version.minor)
               .arg(version.patch);
    }

    void XMakeTool::openXMakeHelpUrl(const XMakeTool *tool, const QString &linkUrl) {
        bool online = true;
        Version version;
        if (tool && tool->isValid()) {
            online = tool->qchFilePath().isEmpty();
            version = tool->version();
        }

        Core::HelpManager::showHelpUrl(linkUrl.arg(documentationUrl(version, online)));
    }

    void XMakeTool::readInformation() const {
        QTC_ASSERT(m_introspection, return );
        if (!m_introspection->m_haveCapabilitites && m_introspection->m_didAttemptToRun) {
            return;
        }

        m_introspection->m_didAttemptToRun = true;

        fetchFromCapabilities();
    }

    static QStringList parseDefinition(const QString &definition) {
        QStringList result;
        QString word;
        bool ignoreWord = false;
        QVector<QChar> braceStack;

        for (const QChar &c : definition) {
            if (c == '[' || c == '<' || c == '(') {
                braceStack.append(c);
                ignoreWord = false;
            } else if (c == ']' || c == '>' || c == ')') {
                if (braceStack.isEmpty() || braceStack.takeLast() == '<') {
                    ignoreWord = true;
                }
            }

            if (c == ' ' || c == '[' || c == '<' || c == '(' || c == ']' || c == '>' || c == ')') {
                if (!ignoreWord && !word.isEmpty()) {
                    if (result.isEmpty()
                        || Utils::allOf(word, [](const QChar &c) {
                                            return c.isUpper() || c == '_';
                                        })) {
                        result.append(word);
                    }
                }
                word.clear();
                ignoreWord = false;
            } else {
                word.append(c);
            }
        }
        return result;
    }

    void XMakeTool::parseFunctionDetailsOutput(const QString &output) {
        bool expectDefinition = false;
        QString currentDefinition;

        const QStringList lines = output.split('\n');
        for (int i = 0; i < lines.count(); ++i) {
            const QString &line = lines.at(i);

            if (line == "::") {
                expectDefinition = true;
                continue;
            }

            if (expectDefinition) {
                if (!line.startsWith(' ') && !line.isEmpty()) {
                    expectDefinition = false;
                    QStringList words = parseDefinition(currentDefinition);
                    if (!words.isEmpty()) {
                        const QString command = words.takeFirst();
                        if (m_introspection->m_keywords.functions.contains(command)) {
                            const QStringList tmp = Utils::sorted(
                                words + m_introspection->m_keywords.functionArgs[command]);
                            m_introspection->m_keywords.functionArgs[command] = Utils::filteredUnique(
                                tmp);
                        }
                    }
                    if (!words.isEmpty() && m_introspection->m_keywords.functions.contains(words.at(0))) {
                        m_introspection->m_keywords.functionArgs[words.at(0)];
                    }
                    currentDefinition.clear();
                } else {
                    currentDefinition.append(line.trimmed() + ' ');
                }
            }
        }
    }

    QStringList XMakeTool::parseVariableOutput(const QString &output) {
        const QStringList variableList = Utils::filtered(output.split('\n'),
                                                         std::not_fn(&QString::isEmpty));
        QStringList result;
        for (const QString &v : variableList) {
            if (v.startsWith("XMAKE_COMPILER_IS_GNU<LANG>")) { // This key takes a compiler name :-/
                result << "XMAKE_COMPILER_IS_GNUCC"
                       << "XMAKE_COMPILER_IS_GNUCXX";
            } else if (v.contains("<CONFIG>") && v.contains("<LANG>")) {
                const QString tmp = QString(v).replace("<CONFIG>", "%1").replace("<LANG>", "%2");
                result << tmp.arg("DEBUG").arg("C") << tmp.arg("DEBUG").arg("CXX")
                       << tmp.arg("RELEASE").arg("C") << tmp.arg("RELEASE").arg("CXX")
                       << tmp.arg("MINSIZEREL").arg("C") << tmp.arg("MINSIZEREL").arg("CXX")
                       << tmp.arg("RELWITHDEBINFO").arg("C") << tmp.arg("RELWITHDEBINFO").arg("CXX");
            } else if (v.contains("<CONFIG>")) {
                const QString tmp = QString(v).replace("<CONFIG>", "%1");
                result << tmp.arg("DEBUG") << tmp.arg("RELEASE") << tmp.arg("MINSIZEREL")
                       << tmp.arg("RELWITHDEBINFO");
            } else if (v.contains("<LANG>")) {
                const QString tmp = QString(v).replace("<LANG>", "%1");
                result << tmp.arg("C") << tmp.arg("CXX");
            } else if (!v.contains('<') && !v.contains('[')) {
                result << v;
            }
        }
        return result;
    }

    QStringList XMakeTool::parseSyntaxHighlightingXml() {
        QStringList moduleFunctions;

        const FilePath xmakeXml = Core::ICore::resourcePath("generic-highlighter/syntax/xmake.xml");
        QXmlStreamReader reader(xmakeXml.fileContents().value_or(QByteArray()));

        auto readItemList = [](QXmlStreamReader &reader) -> QStringList {
            QStringList arguments;
            while (!reader.atEnd() && reader.readNextStartElement()) {
                if (reader.name() == u"item") {
                    arguments.append(reader.readElementText());
                } else {
                    reader.skipCurrentElement();
                }
            }
            return arguments;
        };

        while (!reader.atEnd() && reader.readNextStartElement()) {
            if (reader.name() != u"highlighting") {
                continue;
            }
            while (!reader.atEnd() && reader.readNextStartElement()) {
                if (reader.name() == u"list") {
                    const auto name = reader.attributes().value("name").toString();
                    if (name.endsWith(u"_sargs") || name.endsWith(u"_nargs")) {
                        const auto functionName = name.left(name.length() - 6);
                        QStringList arguments = readItemList(reader);

                        if (m_introspection->m_keywords.functionArgs.contains(functionName)) {
                            arguments.append(
                                m_introspection->m_keywords.functionArgs.value(functionName));
                        }

                        m_introspection->m_keywords.functionArgs[functionName] = arguments;

                        // Functions that are part of XMake modules like ExternalProject_Add
                        // which are not reported by xmake --help-list-commands
                        if (!m_introspection->m_keywords.functions.contains(functionName)) {
                            moduleFunctions << functionName;
                        }
                    } else if (name == u"generator-expressions") {
                        m_introspection->m_keywords.generatorExpressions = toSet(readItemList(reader));
                    } else {
                        reader.skipCurrentElement();
                    }
                } else {
                    reader.skipCurrentElement();
                }
            }
        }

        // Some commands have the same arguments as other commands and the `xmake.xml`
        // but their relationship is weirdly defined in the `xmake.xml` file.
        using ListStringPair = QList<QPair<QString, QString>>;
        const ListStringPair functionPairs = { { "if", "elseif" },
            { "while", "elseif" },
            { "find_path", "find_file" },
            { "find_program", "find_library" },
            { "target_link_libraries", "target_compile_definitions" },
            { "target_link_options", "target_compile_definitions" },
            { "target_link_directories", "target_compile_options" },
            { "set_target_properties", "set_directory_properties" },
            { "set_tests_properties", "set_directory_properties" } };
        for (const auto &pair : std::as_const(functionPairs)) {
            if (!m_introspection->m_keywords.functionArgs.contains(pair.first)) {
                m_introspection->m_keywords.functionArgs[pair.first]
                    = m_introspection->m_keywords.functionArgs.value(pair.second);
            }
        }

        // Special case for xmake_print_variables, which will print the names and values for variables
        // and needs to be as a known function
        const QString xmakePrintVariables("xmake_print_variables");
        m_introspection->m_keywords.functionArgs[xmakePrintVariables] = {};
        moduleFunctions << xmakePrintVariables;

        moduleFunctions.removeDuplicates();
        return moduleFunctions;
    }

    void XMakeTool::fetchFromCapabilities() const {
        Process xmake;
        runXMake(xmake, { "-h" }, 10);

        if (xmake.result() == ProcessResult::FinishedWithSuccess) {
            m_introspection->m_haveCapabilitites = true;
            parseFromCapabilities(xmake.cleanedStdOut());
        } else {
            qCCritical(xmakeToolLog) << "Fetching capabilities failed: " << xmake.allOutput() << xmake.error();
            m_introspection->m_haveCapabilitites = false;
        }
    }

    static int getVersion(const QVariantMap &obj, const QString &value) {
        bool ok;
        int result = obj.value(value).toInt(&ok);
        if (!ok) {
            return -1;
        }
        return result;
    }

    void XMakeTool::parseFromCapabilities(const QString &input) const {
        auto doc = QJsonDocument::fromJson(input.toUtf8());
        QMessageBox::information(nullptr, "标题", input);
        if (!doc.isObject()) {
            return;
        }

        const QVariantMap data = doc.object().toVariantMap();
        const QVariantList generatorList = data.value("generators").toList();
        for (const QVariant &v : generatorList) {
            const QVariantMap gen = v.toMap();
            m_introspection->m_generators.append(Generator(gen.value("name").toString(),
                                                           gen.value("extraGenerators").toStringList(),
                                                           gen.value("platformSupport").toBool(),
                                                           gen.value("toolsetSupport").toBool()));
        }

        const QVariantMap fileApis = data.value("fileApi").toMap();
        const QVariantList requests = fileApis.value("requests").toList();
        for (const QVariant &r : requests) {
            const QVariantMap object = r.toMap();
            const QString kind = object.value("kind").toString();
            const QVariantList versionList = object.value("version").toList();
            std::pair<int, int> highestVersion { -1, -1 };
            for (const QVariant &v : versionList) {
                const QVariantMap versionObject = v.toMap();
                const std::pair<int, int> version { getVersion(versionObject, "major"),
                                                    getVersion(versionObject, "minor") };
                if (version.first > highestVersion.first
                    || (version.first == highestVersion.first && version.second > highestVersion.second)) {
                    highestVersion = version;
                }
            }
            if (!kind.isNull() && highestVersion.first != -1 && highestVersion.second != -1) {
                m_introspection->m_fileApis.append({ kind, highestVersion });
            }
        }

        const QVariantMap versionInfo = data.value("version").toMap();
        m_introspection->m_version.major = versionInfo.value("major").toInt();
        m_introspection->m_version.minor = versionInfo.value("minor").toInt();
        m_introspection->m_version.patch = versionInfo.value("patch").toInt();
        m_introspection->m_version.fullVersion = versionInfo.value("string").toByteArray();
    }
} // namespace XMakeProjectManager
