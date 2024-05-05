// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "xmakeprojectimporter.h"

#include "xmakebuildconfiguration.h"
#include "xmakekitaspect.h"
#include "xmakeproject.h"
#include "xmakeprojectconstants.h"
#include "xmakeprojectmanagertr.h"
#include "xmaketoolmanager.h"
#include "presetsmacros.h"

#include <coreplugin/messagemanager.h>

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchainmanager.h>

#include <qtsupport/qtkitaspect.h>

#include <utils/algorithm.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/temporarydirectory.h>

#include <QApplication>
#include <QLoggingCategory>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;

using namespace std::chrono_literals;

namespace XMakeProjectManager::Internal {
    static Q_LOGGING_CATEGORY(cmInputLog, "qtc.xmake.import", QtWarningMsg);

    struct DirectoryData {
        // Project Stuff:
        QByteArray xmakeBuildType;
        FilePath buildDirectory;
        FilePath xmakeHomeDirectory;
        bool hasQmlDebugging = false;

        QString xmakePresetDisplayname;
        QString xmakePreset;

        // Kit Stuff
        FilePath xmakeBinary;
        QString generator;
        QString platform;
        QString toolset;
        FilePath sysroot;
        QtProjectImporter::QtVersionData qt;
        QVector<ToolchainDescription> toolchains;
    };

    static FilePaths scanDirectory(const FilePath &path, const QString &prefix) {
        FilePaths result;
        qCDebug(cmInputLog) << "Scanning for directories matching" << prefix << "in" << path;

        const FilePaths entries = path.dirEntries({ { prefix + "*" }, QDir::Dirs | QDir::NoDotAndDotDot });
        for (const FilePath &entry : entries) {
            QTC_ASSERT(entry.isDir(), continue);
            result.append(entry);
        }
        return result;
    }

    static QString baseXMakeToolDisplayName(XMakeTool &tool) {
        if (!tool.isValid()) {
            return QString("XMake");
        }

        XMakeTool::Version version = tool.version();
        return QString("XMake %1.%2.%3").arg(version.major).arg(version.minor).arg(version.patch);
    }

    static QString uniqueXMakeToolDisplayName(XMakeTool &tool) {
        QString baseName = baseXMakeToolDisplayName(tool);

        QStringList existingNames;
        for (const XMakeTool *t : XMakeToolManager::xmakeTools()) {
            existingNames << t->displayName();
        }
        return Utils::makeUniquelyNumbered(baseName, existingNames);
    }

// XMakeProjectImporter

    XMakeProjectImporter::XMakeProjectImporter(const FilePath &path, const XMakeProject *project)
        : QtProjectImporter(path)
        , m_project(project)
        , m_presetsTempDir("qtc-xmake-presets-XXXXXXXX") {
        useTemporaryKitAspect(XMakeKitAspect::id(),
                              [this](Kit *k, const QVariantList &vl) {
                                  cleanupTemporaryXMake(k, vl);
                              },
                              [this](Kit *k, const QVariantList &vl) {
                                  persistTemporaryXMake(k, vl);
                              });
    }

    using CharToHexList = QList<QPair<QString, QString>>;
    static const CharToHexList &charToHexList() {
        static const CharToHexList list = {
            { "<", "{3C}" },
            { ">", "{3E}" },
            { ":", "{3A}" },
            { "\"", "{22}" },
            { "\\", "{5C}" },
            { "/", "{2F}" },
            { "|", "{7C}" },
            { "?", "{3F}" },
            { "*", "{2A}" },
        };

        return list;
    }

    static QString presetNameToFileName(const QString &name) {
        QString fileName = name;
        for (const auto &p : charToHexList()) {
            fileName.replace(p.first, p.second);
        }
        return fileName;
    }

    static QString fileNameToPresetName(const QString &fileName) {
        QString name = fileName;
        for (const auto &p : charToHexList()) {
            name.replace(p.second, p.first);
        }
        return name;
    }

    static QString displayPresetName(const QString &presetName) {
        return QString("%1 (XMake preset)").arg(presetName);
    }

    FilePaths XMakeProjectImporter::importCandidates() {
        FilePaths candidates = presetCandidates();

        if (candidates.isEmpty()) {
            candidates << scanDirectory(projectFilePath().absolutePath(), "build");

            const QList<Kit *> kits = KitManager::kits();
            for (const Kit *k : kits) {
                FilePath shadowBuildDirectory
                    = XMakeBuildConfiguration::shadowBuildDirectory(projectFilePath(),
                                                                    k,
                                                                    QString(),
                                                                    BuildConfiguration::Unknown);
                candidates << scanDirectory(shadowBuildDirectory.absolutePath(), QString());
            }
        }

        const FilePaths finalists = Utils::filteredUnique(candidates);
        qCInfo(cmInputLog) << "import candidates:" << finalists;
        return finalists;
    }

    FilePaths XMakeProjectImporter::presetCandidates() {
        FilePaths candidates;

        for (const auto &configPreset : m_project->presetsData().configurePresets) {
            if (configPreset.hidden.value()) {
                continue;
            }

            if (configPreset.condition) {
                if (!XMakePresets::Macros::evaluatePresetCondition(configPreset, projectFilePath())) {
                    continue;
                }
            }

            const FilePath configPresetDir = m_presetsTempDir.filePath(
                presetNameToFileName(configPreset.name));
            configPresetDir.createDir();
            candidates << configPresetDir;

            // If the binaryFilePath exists, do not try to import the existing build, so that
            // we don't have duplicates, one from the preset and one from the previous configuration.
            if (configPreset.binaryDir) {
                Environment env = projectDirectory().deviceEnvironment();
                XMakePresets::Macros::expand(configPreset, env, projectDirectory());

                QString binaryDir = configPreset.binaryDir.value();
                XMakePresets::Macros::expand(configPreset, env, projectDirectory(), binaryDir);

                const FilePath binaryFilePath = FilePath::fromString(binaryDir);
                candidates.removeIf([&binaryFilePath](const FilePath &path) {
                                        return path == binaryFilePath;
                                    });
            }
        }

        return candidates;
    }

    Target *XMakeProjectImporter::preferredTarget(const QList<Target *> &possibleTargets) {
        for (Kit *kit : m_project->oldPresetKits()) {
            const bool haveKit = Utils::contains(possibleTargets, [kit](const auto &target) {
                                                     return target->kit() == kit;
                                                 });

            if (!haveKit) {
                KitManager::deregisterKit(kit);
            }
        }
        m_project->setOldPresetKits({});

        return ProjectImporter::preferredTarget(possibleTargets);
    }

    bool XMakeProjectImporter::filter(ProjectExplorer::Kit *k) const {
        if (!m_project->presetsData().havePresets) {
            return true;
        }

        const auto presetConfigItem = XMakeConfigurationKitAspect::xmakePresetConfigItem(k);
        if (presetConfigItem.isNull()) {
            return false;
        }

        const QString presetName = presetConfigItem.expandedValue(k);
        return std::find_if(m_project->presetsData().configurePresets.cbegin(),
                            m_project->presetsData().configurePresets.cend(),
                            [&presetName](const auto &preset) {
                                return presetName == preset.name;
                            })
               != m_project->presetsData().configurePresets.cend();
    }

    static XMakeConfig configurationFromPresetProbe(
        const FilePath &importPath,
        const FilePath &sourceDirectory,
        const PresetsDetails::ConfigurePreset &configurePreset) {
        const FilePath xmakeListTxt = importPath / Constants::XMAKE_LISTS_TXT;
        xmakeListTxt.writeFileContents(QByteArray("xmake_minimum_required(VERSION 3.15)\n"
                                                  "\n"
                                                  "project(preset-probe)\n"
                                                  "set(XMAKE_C_COMPILER \"${XMAKE_C_COMPILER}\" CACHE FILEPATH \"\" FORCE)\n"
                                                  "set(XMAKE_CXX_COMPILER \"${XMAKE_CXX_COMPILER}\" CACHE FILEPATH \"\" FORCE)\n"
                                                  "\n"));

        Process xmake;
        xmake.setDisableUnixTerminal();

        const FilePath xmakeExecutable = FilePath::fromString(configurePreset.xmakeExecutable.value());

        Environment env = xmakeExecutable.deviceEnvironment();
        XMakePresets::Macros::expand(configurePreset, env, sourceDirectory);

        env.setupEnglishOutput();
        xmake.setEnvironment(env);

        QStringList args;

        if (configurePreset.generator) {
        }
        if (configurePreset.architecture && configurePreset.architecture.value().value) {
            if (!configurePreset.architecture->strategy
                || configurePreset.architecture->strategy
                != PresetsDetails::ValueStrategyPair::Strategy::external) {
            }
        }
        if (configurePreset.toolset && configurePreset.toolset.value().value) {
            if (!configurePreset.toolset->strategy
                || configurePreset.toolset->strategy
                != PresetsDetails::ValueStrategyPair::Strategy::external) {
            }
        }

        if (configurePreset.cacheVariables) {
            const XMakeConfig cache = configurePreset.cacheVariables
                                      ? configurePreset.cacheVariables.value()
                                      : XMakeConfig();

            const QString xmakeMakeProgram = cache.stringValueOf("XMAKE_MAKE_PROGRAM");
            const QString toolchainFile = cache.stringValueOf("XMAKE_TOOLCHAIN_FILE");
            const QString prefixPath = cache.stringValueOf("XMAKE_PREFIX_PATH");
            const QString findRootPath = cache.stringValueOf("XMAKE_FIND_ROOT_PATH");
            const QString qtHostPath = cache.stringValueOf("QT_HOST_PATH");
            const QString sysRoot = cache.stringValueOf("XMAKE_SYSROOT");

            if (!xmakeMakeProgram.isEmpty()) {
            }
            if (!toolchainFile.isEmpty()) {
            }
            if (!prefixPath.isEmpty()) {
            }
            if (!findRootPath.isEmpty()) {
            }
            if (!qtHostPath.isEmpty()) {
            }
            if (!sysRoot.isEmpty()) {
            }
        }

        qCDebug(cmInputLog) << "XMake probing for compilers: " << xmakeExecutable.toUserOutput()
                            << args;
        xmake.setCommand({ xmakeExecutable, args });
        xmake.runBlocking(30s);

        QString errorMessage;
        const XMakeConfig config = XMakeConfig::fromFile(importPath.pathAppended(
            "build/XMakeCache.txt"),
                                                         &errorMessage);

        return config;
    }

    struct QMakeAndXMakePrefixPath {
        FilePath qmakePath;
        QString xmakePrefixPath; // can be a semicolon-separated list
    };

    static QMakeAndXMakePrefixPath qtInfoFromXMakeCache(const XMakeConfig &config,
                                                        const Environment &env) {
        // Qt4 way to define things (more convenient for us, so try this first;-)
        const FilePath qmake = config.filePathValueOf("QT_QMAKE_EXECUTABLE");
        qCDebug(cmInputLog) << "QT_QMAKE_EXECUTABLE=" << qmake.toUserOutput();

        // Check Qt5 settings: oh, the horror!
        const FilePath qtXMakeDir = [config, env] {
            FilePath tmp;
            // Check the XMake "<package-name>_DIR" variable
            for (const auto &var : { "Qt6", "Qt6Core", "Qt5", "Qt5Core" }) {
                tmp = config.filePathValueOf(QByteArray(var) + "_DIR");
                if (!tmp.isEmpty()) {
                    break;
                }
            }
            return tmp;
        }();
        qCDebug(cmInputLog) << "QtXCore_DIR=" << qtXMakeDir.toUserOutput();
        const FilePath canQtXMakeDir = FilePath::fromString(qtXMakeDir.toFileInfo().canonicalFilePath());
        qCInfo(cmInputLog) << "QtXCore_DIR (canonical)=" << canQtXMakeDir.toUserOutput();

        const QString prefixPath = [qtXMakeDir, canQtXMakeDir, config, env] {
            QString result;
            if (!qtXMakeDir.isEmpty()) {
                result = canQtXMakeDir.parentDir().parentDir().parentDir().path(); // Up 3 levels...
            } else {
                // Check the XMAKE_PREFIX_PATH and "<package-name>_ROOT" XMake or environment variables
                // This can be a single value or a semicolon-separated list
                for (const auto &var : { "XMAKE_PREFIX_PATH", "Qt6_ROOT", "Qt5_ROOT" }) {
                    result = config.stringValueOf(var);
                    if (result.isEmpty()) {
                        result = env.value(QString::fromUtf8(var));
                    }
                    if (!result.isEmpty()) {
                        break;
                    }
                }
            }
            return result;
        }();
        qCDebug(cmInputLog) << "PrefixPath:" << prefixPath;

        if (!qmake.isEmpty() && !prefixPath.isEmpty()) {
            return { qmake, prefixPath }
        }
        ;

        FilePath toolchainFile = config.filePathValueOf(QByteArray("XMAKE_TOOLCHAIN_FILE"));
        if (prefixPath.isEmpty() && toolchainFile.isEmpty()) {
            return { qmake, QString() }
        }
        ;

        // Run a XMake project that would do qmake probing
        TemporaryDirectory qtcQMakeProbeDir("qtc-xmake-qmake-probe-XXXXXXXX");

        FilePath xmakeListTxt(qtcQMakeProbeDir.filePath(Constants::XMAKE_LISTS_TXT));

        xmakeListTxt.writeFileContents(QByteArray(R"(
        xmake_minimum_required(VERSION 3.15)

        project(qmake-probe LANGUAGES NONE)

        # Bypass Qt6's usage of find_dependency, which would require compiler
        # and source code probing, which slows things unnecessarily
        file(WRITE "${XMAKE_SOURCE_DIR}/XMakeFindDependencyMacro.xmake"
        [=[
            macro(find_dependency dep)
            endmacro()
        ]=])
        set(XMAKE_MODULE_PATH "${XMAKE_SOURCE_DIR}")

        find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
        find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)

        if (XMAKE_CROSSCOMPILING)
            find_program(qmake_binary
                NAMES qmake qmake.bat
                PATHS "${Qt${QT_VERSION_MAJOR}_DIR}/../../../bin"
                NO_DEFAULT_PATH)
            file(WRITE "${XMAKE_SOURCE_DIR}/qmake-location.txt" "${qmake_binary}")
        else()
            file(GENERATE
                OUTPUT "${XMAKE_SOURCE_DIR}/qmake-location.txt"
                CONTENT "$<TARGET_PROPERTY:Qt${QT_VERSION_MAJOR}::qmake,IMPORTED_LOCATION>")
        endif()

        # Remove a Qt XMake hack that adds lib/xmake at the end of every path in XMAKE_PREFIX_PATH
        list(REMOVE_DUPLICATES XMAKE_PREFIX_PATH)
        list(TRANSFORM XMAKE_PREFIX_PATH REPLACE "/lib/xmake$" "")
        file(WRITE "${XMAKE_SOURCE_DIR}/xmake-prefix-path.txt" "${XMAKE_PREFIX_PATH}")
    )"));

        Process xmake;
        xmake.setDisableUnixTerminal();

        Environment xmakeEnv(env);
        xmakeEnv.setupEnglishOutput();
        xmake.setEnvironment(xmakeEnv);

        QString xmakeGenerator = config.stringValueOf(QByteArray("XMAKE_GENERATOR"));
        QString xmakeGeneratorPlatform = config.stringValueOf(QByteArray("XMAKE_GENERATOR_PLATFORM"));
        QString xmakeGeneratorToolset = config.stringValueOf(QByteArray("XMAKE_GENERATOR_TOOLSET"));
        FilePath xmakeExecutable = config.filePathValueOf(QByteArray("XMAKE_COMMAND"));
        FilePath xmakeMakeProgram = config.filePathValueOf(QByteArray("XMAKE_MAKE_PROGRAM"));
        FilePath hostPath = config.filePathValueOf(QByteArray("QT_HOST_PATH"));
        const QString findRootPath = config.stringValueOf("XMAKE_FIND_ROOT_PATH");

        QStringList args;
        args.push_back("-S");
        args.push_back(qtcQMakeProbeDir.path().path());
        args.push_back("-B");
        args.push_back(qtcQMakeProbeDir.filePath("build").path());
        if (!xmakeGenerator.isEmpty()) {
            args.push_back("-G");
            args.push_back(xmakeGenerator);
        }
        if (!xmakeGeneratorPlatform.isEmpty()) {
            args.push_back("-A");
            args.push_back(xmakeGeneratorPlatform);
        }
        if (!xmakeGeneratorToolset.isEmpty()) {
            args.push_back("-T");
            args.push_back(xmakeGeneratorToolset);
        }

        if (!xmakeMakeProgram.isEmpty()) {
            args.push_back(QStringLiteral("-DXMAKE_MAKE_PROGRAM=%1").arg(xmakeMakeProgram.toString()));
        }

        if (!toolchainFile.isEmpty()) {
            args.push_back(QStringLiteral("-DXMAKE_TOOLCHAIN_FILE=%1").arg(toolchainFile.toString()));
        }
        if (!prefixPath.isEmpty()) {
            args.push_back(QStringLiteral("-DXMAKE_PREFIX_PATH=%1").arg(prefixPath));
        }
        if (!findRootPath.isEmpty()) {
            args.push_back(QStringLiteral("-DXMAKE_FIND_ROOT_PATH=%1").arg(findRootPath));
        }
        if (!hostPath.isEmpty()) {
            args.push_back(QStringLiteral("-DQT_HOST_PATH=%1").arg(hostPath.toString()));
        }

        qCDebug(cmInputLog) << "XMake probing for qmake path: " << xmakeExecutable.toUserOutput() << args;
        xmake.setCommand({ xmakeExecutable, args });
        xmake.runBlocking(5s);

        const FilePath qmakeLocationTxt = qtcQMakeProbeDir.filePath("qmake-location.txt");
        const FilePath qmakeLocation = FilePath::fromUtf8(
            qmakeLocationTxt.fileContents().value_or(QByteArray()));
        qCDebug(cmInputLog) << "qmake location: " << qmakeLocation.toUserOutput();

        const FilePath prefixPathTxt = qtcQMakeProbeDir.filePath("xmake-prefix-path.txt");
        const QString resultedPrefixPath = QString::fromUtf8(
            prefixPathTxt.fileContents().value_or(QByteArray()));
        qCDebug(cmInputLog) << "PrefixPath [after qmake probe]: " << resultedPrefixPath;

        return { qmakeLocation, resultedPrefixPath };
    }

    static QVector<ToolchainDescription> extractToolchainsFromCache(const XMakeConfig &config) {
        QVector<ToolchainDescription> result;
        bool haveCCxxCompiler = false;
        for (const XMakeConfigItem &i : config) {
            if (!i.key.startsWith("XMAKE_") || !i.key.endsWith("_COMPILER")) {
                continue;
            }
            const QByteArray language = i.key.mid(6, i.key.size() - 6 - 9); // skip "XMAKE_" and "_COMPILER"
            Id languageId;
            if (language == "CXX") {
                haveCCxxCompiler = true;
                languageId = ProjectExplorer::Constants::CXX_LANGUAGE_ID;
            } else if (language == "C") {
                haveCCxxCompiler = true;
                languageId = ProjectExplorer::Constants::C_LANGUAGE_ID;
            } else {
                languageId = Id::fromName(language);
            }
            result.append({ FilePath::fromUtf8(i.value), languageId });
        }

        if (!haveCCxxCompiler) {
            const QByteArray generator = config.valueOf("XMAKE_GENERATOR");
            QString cCompilerName;
            QString cxxCompilerName;
            if (generator.contains("Visual Studio")) {
                cCompilerName = "cl.exe";
                cxxCompilerName = "cl.exe";
            } else if (generator.contains("Xcode")) {
                cCompilerName = "clang";
                cxxCompilerName = "clang++";
            }

            if (!cCompilerName.isEmpty() && !cxxCompilerName.isEmpty()) {
                const FilePath linker = config.filePathValueOf("XMAKE_LINKER");
                if (!linker.isEmpty()) {
                    const FilePath compilerPath = linker.parentDir();
                    result.append({ compilerPath.pathAppended(cCompilerName),
                                    ProjectExplorer::Constants::C_LANGUAGE_ID });
                    result.append({ compilerPath.pathAppended(cxxCompilerName),
                                    ProjectExplorer::Constants::CXX_LANGUAGE_ID });
                }
            }
        }

        return result;
    }

    static QString extractVisualStudioPlatformFromConfig(const XMakeConfig &config) {
        const QString xmakeGenerator = config.stringValueOf(QByteArray("XMAKE_GENERATOR"));
        QString platform;
        if (xmakeGenerator.contains("Visual Studio")) {
            const FilePath linker = config.filePathValueOf("XMAKE_LINKER");
            const QString toolsDir = linker.parentDir().fileName();
            if (toolsDir.compare("x64", Qt::CaseInsensitive) == 0) {
                platform = "x64";
            } else if (toolsDir.compare("x86", Qt::CaseInsensitive) == 0) {
                platform = "Win32";
            } else if (toolsDir.compare("arm64", Qt::CaseInsensitive) == 0) {
                platform = "ARM64";
            } else if (toolsDir.compare("arm", Qt::CaseInsensitive) == 0) {
                platform = "ARM";
            }
        }

        return platform;
    }

    void updateCompilerPaths(XMakeConfig &config, const Environment &env) {
        auto updateRelativePath = [&config, env](const QByteArray &key) {
            FilePath pathValue = config.filePathValueOf(key);

            if (pathValue.isAbsolutePath() || pathValue.isEmpty()) {
                return;
            }

            pathValue = env.searchInPath(pathValue.fileName());

            auto it = std::find_if(config.begin(), config.end(), [&key](const XMakeConfigItem &item) {
                                       return item.key == key;
                                   });
            QTC_ASSERT(it != config.end(), return );

            it->value = pathValue.path().toUtf8();
        };

        updateRelativePath("XMAKE_C_COMPILER");
        updateRelativePath("XMAKE_CXX_COMPILER");
    }

    void updateConfigWithDirectoryData(XMakeConfig &config, const std::unique_ptr<DirectoryData> &data) {
        auto updateCompilerValue = [&config, &data](const QByteArray &key, const Utils::Id &language) {
            auto it = std::find_if(config.begin(), config.end(), [&key](const XMakeConfigItem &ci) {
                                       return ci.key == key;
                                   });

            auto tcd = Utils::findOrDefault(data->toolchains,
                                            [&language](const ToolchainDescription &t) {
                                                return t.language == language;
                                            });

            if (it != config.end() && it->value.isEmpty()) {
                it->value = tcd.compilerPath.toString().toUtf8();
            } else {
                config << XMakeConfigItem(key,
                                          XMakeConfigItem::FILEPATH,
                                          tcd.compilerPath.toString().toUtf8());
            }
        };

        updateCompilerValue("XMAKE_C_COMPILER", ProjectExplorer::Constants::C_LANGUAGE_ID);
        updateCompilerValue("XMAKE_CXX_COMPILER", ProjectExplorer::Constants::CXX_LANGUAGE_ID);

        if (data->qt.qt) {
            config << XMakeConfigItem("QT_QMAKE_EXECUTABLE",
                                      XMakeConfigItem::FILEPATH,
                                      data->qt.qt->qmakeFilePath().toString().toUtf8());
        }
    }

    Toolchain *findExternalToolchain(const QString &presetArchitecture, const QString &presetToolset) {
        // A compiler path example. Note that the compiler version is not the same version from MsvcToolchain
        // ... \MSVC\14.29.30133\bin\Hostx64\x64\cl.exe
        //
        // And the XMakePresets.json
        //
        // "toolset": {
        // "value": "v142,host=x64,version=14.29.30133",
        // "strategy": "external"
        // },
        // "architecture": {
        // "value": "x64",
        // "strategy": "external"
        // }

        auto msvcToolchains = ToolchainManager::toolchains([](const Toolchain *tc) {
                                                               return tc->typeId() ==  ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID;
                                                           });

        const QSet<Abi::OSFlavor> msvcFlavors = Utils::toSet(Utils::transform(msvcToolchains, [](const Toolchain *tc) {
                                                                                  return tc->targetAbi().osFlavor();
                                                                              }));

        return ToolchainManager::toolchain(
            [presetArchitecture, presetToolset, msvcFlavors](const Toolchain *tc) -> bool {
                if (tc->typeId() != ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID) {
                    return false;
                }

                const FilePath compilerPath = tc->compilerCommand();
                const QString architecture = compilerPath.parentDir().fileName().toLower();
                const QString host
                    = compilerPath.parentDir().parentDir().fileName().toLower().replace("host", "host=");
                const QString version
                    = QString("version=%1")
                        .arg(compilerPath.parentDir().parentDir().parentDir().parentDir().fileName());

                static std::pair<QString, Abi::OSFlavor> abiTable[] = {
                    { QStringLiteral("v143"), Abi::WindowsMsvc2022Flavor },
                    { QStringLiteral("v142"), Abi::WindowsMsvc2019Flavor },
                    { QStringLiteral("v141"), Abi::WindowsMsvc2017Flavor },
                };

                Abi::OSFlavor toolsetAbi = Abi::UnknownFlavor;
                for (const auto &abiPair : abiTable) {
                    if (presetToolset.contains(abiPair.first)) {
                        toolsetAbi = abiPair.second;
                        break;
                    }
                }

                // User didn't specify any flavor, so pick the highest toolchain available
                if (toolsetAbi == Abi::UnknownFlavor) {
                    for (const auto &abiPair : abiTable) {
                        if (msvcFlavors.contains(abiPair.second)) {
                            toolsetAbi = abiPair.second;
                            break;
                        }
                    }
                }

                if (toolsetAbi != tc->targetAbi().osFlavor()) {
                    return false;
                }

                if (presetToolset.contains("host=") && !presetToolset.contains(host)) {
                    return false;
                }

                // Make sure we match also version=14.29
                auto versionIndex = presetToolset.indexOf("version=");
                if (versionIndex != -1 && !version.startsWith(presetToolset.mid(versionIndex))) {
                    return false;
                }

                if (presetArchitecture != architecture) {
                    return false;
                }

                qCDebug(cmInputLog) << "For external architecture" << presetArchitecture
                                    << "and toolset" << presetToolset
                                    << "the following toolchain was selected:\n"
                                    << compilerPath.toString();
                return true;
            });
    }

    QList<void *> XMakeProjectImporter::examineDirectory(const FilePath &importPath,
                                                         QString *warningMessage) const {
        QList<void *> result;
        qCInfo(cmInputLog) << "Examining directory:" << importPath.toUserOutput();

        if (importPath.isChildOf(m_presetsTempDir.path())) {
            auto data = std::make_unique<DirectoryData>();

            const QString presetName = fileNameToPresetName(importPath.fileName());
            PresetsDetails::ConfigurePreset configurePreset
                = Utils::findOrDefault(m_project->presetsData().configurePresets,
                                       [presetName](const PresetsDetails::ConfigurePreset &preset) {
                                           return preset.name == presetName;
                                       });

            Environment env = projectDirectory().deviceEnvironment();
            XMakePresets::Macros::expand(configurePreset, env, projectDirectory());

            if (configurePreset.displayName) {
                data->xmakePresetDisplayname = configurePreset.displayName.value();
            } else {
                data->xmakePresetDisplayname = configurePreset.name;
            }
            data->xmakePreset = configurePreset.name;

            if (!configurePreset.xmakeExecutable) {
                const XMakeTool *xmakeTool = XMakeToolManager::defaultXMakeTool();
                if (xmakeTool) {
                    configurePreset.xmakeExecutable = xmakeTool->xmakeExecutable().toString();
                } else {
                    configurePreset.xmakeExecutable = QString();
                    TaskHub::addTask(
                        BuildSystemTask(Task::TaskType::Error, Tr::tr("<No XMake Tool available>")));
                    TaskHub::requestPopup();
                }
            } else {
                QString xmakeExecutable = configurePreset.xmakeExecutable.value();
                XMakePresets::Macros::expand(configurePreset, env, projectDirectory(), xmakeExecutable);

                configurePreset.xmakeExecutable = FilePath::fromUserInput(xmakeExecutable).path();
            }

            data->xmakeBinary = Utils::FilePath::fromString(configurePreset.xmakeExecutable.value());
            if (configurePreset.generator) {
                data->generator = configurePreset.generator.value();
            }

            if (configurePreset.binaryDir) {
                QString binaryDir = configurePreset.binaryDir.value();
                XMakePresets::Macros::expand(configurePreset, env, projectDirectory(), binaryDir);
                data->buildDirectory = Utils::FilePath::fromString(binaryDir);
            }

            const bool architectureExternalStrategy
                = configurePreset.architecture && configurePreset.architecture->strategy
                    && configurePreset.architecture->strategy
                    == PresetsDetails::ValueStrategyPair::Strategy::external;

            const bool toolsetExternalStrategy
                = configurePreset.toolset && configurePreset.toolset->strategy
                    && configurePreset.toolset->strategy
                    == PresetsDetails::ValueStrategyPair::Strategy::external;

            if (!architectureExternalStrategy && configurePreset.architecture
                && configurePreset.architecture.value().value) {
                data->platform = configurePreset.architecture.value().value.value();
            }

            if (!toolsetExternalStrategy && configurePreset.toolset && configurePreset.toolset.value().value) {
                data->toolset = configurePreset.toolset.value().value.value();
            }

            if (architectureExternalStrategy && toolsetExternalStrategy) {
                const Toolchain *tc
                    = findExternalToolchain(configurePreset.architecture->value.value_or(QString()),
                                            configurePreset.toolset->value.value_or(QString()));
                if (tc) {
                    tc->addToEnvironment(env);
                }
            }

            XMakePresets::Macros::updateToolchainFile(configurePreset,
                                                      env,
                                                      projectDirectory(),
                                                      data->buildDirectory);

            XMakePresets::Macros::updateCacheVariables(configurePreset, env, projectDirectory());

            const XMakeConfig cache = configurePreset.cacheVariables
                                      ? configurePreset.cacheVariables.value()
                                      : XMakeConfig();
            XMakeConfig config;
            const bool noCompilers = cache.valueOf("XMAKE_C_COMPILER").isEmpty()
                && cache.valueOf("XMAKE_CXX_COMPILER").isEmpty();
            if (noCompilers || !configurePreset.generator) {
                QApplication::setOverrideCursor(Qt::WaitCursor);
                config = configurationFromPresetProbe(importPath, projectDirectory(), configurePreset);
                QApplication::restoreOverrideCursor();

                if (!configurePreset.generator) {
                    QString xmakeGenerator = config.stringValueOf(QByteArray("XMAKE_GENERATOR"));
                    configurePreset.generator = xmakeGenerator;
                    data->generator = xmakeGenerator;
                    data->platform = extractVisualStudioPlatformFromConfig(config);
                    if (!data->platform.isEmpty()) {
                        configurePreset.architecture = PresetsDetails::ValueStrategyPair();
                        configurePreset.architecture->value = data->platform;
                    }
                }
            } else {
                config = cache;
                updateCompilerPaths(config, env);
                config << XMakeConfigItem("XMAKE_COMMAND",
                                          XMakeConfigItem::PATH,
                                          configurePreset.xmakeExecutable.value().toUtf8());
                if (configurePreset.generator) {
                    config << XMakeConfigItem("XMAKE_GENERATOR",
                                              XMakeConfigItem::STRING,
                                              configurePreset.generator.value().toUtf8());
                }
            }

            data->sysroot = config.filePathValueOf("XMAKE_SYSROOT");

            const auto [qmake, xmakePrefixPath] = qtInfoFromXMakeCache(config, env);
            if (!qmake.isEmpty()) {
                data->qt = findOrCreateQtVersion(qmake);
            }

            if (!xmakePrefixPath.isEmpty() && config.valueOf("XMAKE_PREFIX_PATH").isEmpty()) {
                config << XMakeConfigItem("XMAKE_PREFIX_PATH",
                                          XMakeConfigItem::PATH,
                                          xmakePrefixPath.toUtf8());
            }

            // Toolchains:
            data->toolchains = extractToolchainsFromCache(config);

            // Update QT_QMAKE_EXECUTABLE and XMAKE_C|XX_COMPILER config values
            updateConfigWithDirectoryData(config, data);

            data->hasQmlDebugging = XMakeBuildConfiguration::hasQmlDebugging(config);

            QByteArrayList buildConfigurationTypes = { cache.valueOf("XMAKE_BUILD_TYPE") };
            if (buildConfigurationTypes.front().isEmpty()) {
                buildConfigurationTypes.clear();
                QByteArray buildConfigurationTypesString = cache.valueOf("XMAKE_CONFIGURATION_TYPES");
                if (!buildConfigurationTypesString.isEmpty()) {
                    buildConfigurationTypes = buildConfigurationTypesString.split(';');
                } else {
                    for (int type = XMakeBuildConfigurationFactory::BuildTypeXMake;
                         type != XMakeBuildConfigurationFactory::BuildTypeLast;
                         ++type) {
                        BuildInfo info = XMakeBuildConfigurationFactory::createBuildInfo(
                            XMakeBuildConfigurationFactory::BuildType(type));
                        buildConfigurationTypes << info.typeName.toUtf8();
                    }
                }
            }
            for (const auto &buildType : buildConfigurationTypes) {
                DirectoryData *newData = new DirectoryData(*data);
                newData->xmakeBuildType = buildType;

                // Handle QML Debugging
                auto type = XMakeBuildConfigurationFactory::buildTypeFromByteArray(
                    newData->xmakeBuildType);
                if (type == XMakeBuildConfigurationFactory::BuildTypeXMake) {
                    newData->hasQmlDebugging = true;
                }

                result.emplace_back(newData);
            }

            return result;
        }

        const FilePath cacheFile = importPath.pathAppended(Constants::XMAKE_CACHE_TXT);

        if (!cacheFile.exists()) {
            qCDebug(cmInputLog) << cacheFile.toUserOutput() << "does not exist, returning.";
            return result;
        }

        QString errorMessage;
        const XMakeConfig config = XMakeConfig::fromFile(cacheFile, &errorMessage);
        if (config.isEmpty() || !errorMessage.isEmpty()) {
            qCDebug(cmInputLog) << "Failed to read configuration from" << cacheFile << errorMessage;
            return result;
        }

        QByteArrayList buildConfigurationTypes = { config.valueOf("XMAKE_BUILD_TYPE") };
        if (buildConfigurationTypes.front().isEmpty()) {
            QByteArray buildConfigurationTypesString = config.valueOf("XMAKE_CONFIGURATION_TYPES");
            if (!buildConfigurationTypesString.isEmpty()) {
                buildConfigurationTypes = buildConfigurationTypesString.split(';');
            }
        }

        const Environment env = projectDirectory().deviceEnvironment();

        for (auto const &buildType: std::as_const(buildConfigurationTypes)) {
            auto data = std::make_unique<DirectoryData>();

            data->xmakeHomeDirectory =
                FilePath::fromUserInput(config.stringValueOf("XMAKE_HOME_DIRECTORY"))
                .canonicalPath();
            const FilePath canonicalProjectDirectory = projectDirectory().canonicalPath();
            if (data->xmakeHomeDirectory != canonicalProjectDirectory) {
                *warningMessage = Tr::tr("Unexpected source directory \"%1\", expected \"%2\". "
                                         "This can be correct in some situations, for example when "
                                         "importing a standalone Qt test, but usually this is an error. "
                                         "Import the build anyway?")
                    .arg(data->xmakeHomeDirectory.toUserOutput(),
                         canonicalProjectDirectory.toUserOutput());
            }

            data->hasQmlDebugging = XMakeBuildConfiguration::hasQmlDebugging(config);

            data->buildDirectory = importPath;
            data->xmakeBuildType = buildType;

            data->xmakeBinary = config.filePathValueOf("XMAKE_COMMAND");
            data->generator = config.stringValueOf("XMAKE_GENERATOR");
            data->platform = config.stringValueOf("XMAKE_GENERATOR_PLATFORM");
            if (data->platform.isEmpty()) {
                data->platform = extractVisualStudioPlatformFromConfig(config);
            }
            data->toolset = config.stringValueOf("XMAKE_GENERATOR_TOOLSET");
            data->sysroot = config.filePathValueOf("XMAKE_SYSROOT");

            // Qt:
            const auto info = qtInfoFromXMakeCache(config, env);
            if (!info.qmakePath.isEmpty()) {
                data->qt = findOrCreateQtVersion(info.qmakePath);
            }

            // Toolchains:
            data->toolchains = extractToolchainsFromCache(config);

            qCInfo(cmInputLog) << "Offering to import" << importPath.toUserOutput();
            result.push_back(static_cast<void *>(data.release()));
        }
        return result;
    }

    void XMakeProjectImporter::ensureBuildDirectory(DirectoryData &data, const Kit *k) const {
        if (!data.buildDirectory.isEmpty()) {
            return;
        }

        const auto xmakeBuildType = XMakeBuildConfigurationFactory::buildTypeFromByteArray(
            data.xmakeBuildType);
        auto buildInfo = XMakeBuildConfigurationFactory::createBuildInfo(xmakeBuildType);

        data.buildDirectory = XMakeBuildConfiguration::shadowBuildDirectory(projectFilePath(),
                                                                            k,
                                                                            buildInfo.typeName,
                                                                            buildInfo.buildType);
    }

    bool XMakeProjectImporter::matchKit(void *directoryData, const Kit *k) const {
        DirectoryData *data = static_cast<DirectoryData *>(directoryData);

        XMakeTool *cm = XMakeKitAspect::xmakeTool(k);
        if (!cm || cm->xmakeExecutable() != data->xmakeBinary) {
            return false;
        }

        if (XMakeGeneratorKitAspect::generator(k) != data->generator
            || XMakeGeneratorKitAspect::platform(k) != data->platform
            || XMakeGeneratorKitAspect::toolset(k) != data->toolset) {
            return false;
        }

        if (SysRootKitAspect::sysRoot(k) != data->sysroot) {
            return false;
        }

        if (data->qt.qt && QtSupport::QtKitAspect::qtVersionId(k) != data->qt.qt->uniqueId()) {
            return false;
        }

        const bool compilersMatch = [k, data] {
            const QList<Id> allLanguages = ToolchainManager::allLanguages();
            for (const ToolchainDescription &tcd : data->toolchains) {
                if (!Utils::contains(allLanguages,
                                     [&tcd](const Id &language) {
                                         return language == tcd.language;
                                     })) {
                    continue;
                }
                Toolchain *tc = ToolchainKitAspect::toolchain(k, tcd.language);
                if ((!tc || !tc->matchesCompilerCommand(tcd.compilerPath))) {
                    return false;
                }
            }
            return true;
        }();
        const bool noCompilers = [k, data] {
            const QList<Id> allLanguages = ToolchainManager::allLanguages();
            for (const ToolchainDescription &tcd : data->toolchains) {
                if (!Utils::contains(allLanguages,
                                     [&tcd](const Id &language) {
                                         return language == tcd.language;
                                     })) {
                    continue;
                }
                Toolchain *tc = ToolchainKitAspect::toolchain(k, tcd.language);
                if (tc && tc->matchesCompilerCommand(tcd.compilerPath)) {
                    return false;
                }
            }
            return true;
        }();

        bool haveXMakePreset = false;
        if (!data->xmakePreset.isEmpty()) {
            const auto presetConfigItem = XMakeConfigurationKitAspect::xmakePresetConfigItem(k);

            const QString presetName = presetConfigItem.expandedValue(k);
            if (data->xmakePreset != presetName) {
                return false;
            }

            if (!k->unexpandedDisplayName().contains(displayPresetName(data->xmakePresetDisplayname))) {
                return false;
            }

            ensureBuildDirectory(*data, k);
            haveXMakePreset = true;
        }

        if (!compilersMatch && !(haveXMakePreset && noCompilers)) {
            return false;
        }

        qCDebug(cmInputLog) << k->displayName()
                            << "matches directoryData for" << data->buildDirectory.toUserOutput();
        return true;
    }

    Kit *XMakeProjectImporter::createKit(void *directoryData) const {
        DirectoryData *data = static_cast<DirectoryData *>(directoryData);

        return QtProjectImporter::createTemporaryKit(data->qt, [&data, this](Kit *k) {
                                                         const XMakeToolData cmtd = findOrCreateXMakeTool(data->xmakeBinary);
                                                         QTC_ASSERT(cmtd.xmakeTool, return );
                                                         if (cmtd.isTemporary) {
                                                             addTemporaryData(XMakeKitAspect::id(), cmtd.xmakeTool->id().toSetting(), k);
                                                         }
                                                         XMakeKitAspect::setXMakeTool(k, cmtd.xmakeTool->id());

                                                         XMakeGeneratorKitAspect::setGenerator(k, data->generator);
                                                         XMakeGeneratorKitAspect::setPlatform(k, data->platform);
                                                         XMakeGeneratorKitAspect::setToolset(k, data->toolset);

                                                         SysRootKitAspect::setSysRoot(k, data->sysroot);

                                                         for (const ToolchainDescription &cmtcd : data->toolchains) {
                                                             const ToolchainData tcd = findOrCreateToolchains(cmtcd);
                                                             QTC_ASSERT(!tcd.tcs.isEmpty(), continue);

                                                             if (tcd.areTemporary) {
                                                                 for (Toolchain *tc : tcd.tcs) {
                                                                     addTemporaryData(ToolchainKitAspect::id(), tc->id(), k);
                                                                 }
                                                             }

                                                             ToolchainKitAspect::setToolchain(k, tcd.tcs.at(0));
                                                         }

                                                         if (!data->xmakePresetDisplayname.isEmpty()) {
                                                             k->setUnexpandedDisplayName(displayPresetName(data->xmakePresetDisplayname));

                                                             XMakeConfigurationKitAspect::setXMakePreset(k, data->xmakePreset);
                                                         }
                                                         if (!data->xmakePreset.isEmpty()) {
                                                             ensureBuildDirectory(*data, k);
                                                         }

                                                         qCInfo(cmInputLog) << "Temporary Kit created.";
                                                     });
    }

    const QList<BuildInfo> XMakeProjectImporter::buildInfoList(void *directoryData) const {
        auto data = static_cast<const DirectoryData *>(directoryData);

        // create info:
        XMakeBuildConfigurationFactory::BuildType buildType
            = XMakeBuildConfigurationFactory::buildTypeFromByteArray(data->xmakeBuildType);
        // RelWithDebInfo + QML Debugging = Profile
        if (buildType == XMakeBuildConfigurationFactory::BuildTypeRelWithDebInfo
            && data->hasQmlDebugging) {
            buildType = XMakeBuildConfigurationFactory::BuildTypeProfile;
        }
        BuildInfo info = XMakeBuildConfigurationFactory::createBuildInfo(buildType);
        info.buildDirectory = data->buildDirectory;

        QVariantMap config = info.extraInfo.toMap(); // new empty, or existing one from createBuildInfo
        config.insert(Constants::XMAKE_HOME_DIR, data->xmakeHomeDirectory.toVariant());
        // Potentially overwrite the default QML Debugging settings for the build type as set by
        // createBuildInfo, in case we are importing a "Debug" XMake configuration without QML Debugging
        config.insert(Constants::QML_DEBUG_SETTING,
                      data->hasQmlDebugging ? TriState::Enabled.toVariant()
                                        : TriState::Default.toVariant());
        info.extraInfo = config;

        qCDebug(cmInputLog) << "BuildInfo configured.";
        return { info };
    }

    XMakeProjectImporter::XMakeToolData
    XMakeProjectImporter::findOrCreateXMakeTool(const FilePath &xmakeToolPath) const {
        XMakeToolData result;
        result.xmakeTool = XMakeToolManager::findByCommand(xmakeToolPath);
        if (!result.xmakeTool) {
            qCDebug(cmInputLog) << "Creating temporary XMakeTool for" << xmakeToolPath.toUserOutput();

            UpdateGuard guard(*this);

            auto newTool = std::make_unique<XMakeTool>(XMakeTool::ManualDetection, XMakeTool::createId());
            newTool->setFilePath(xmakeToolPath);
            newTool->setDisplayName(uniqueXMakeToolDisplayName(*newTool));

            result.xmakeTool = newTool.get();
            result.isTemporary = true;
            XMakeToolManager::registerXMakeTool(std::move(newTool));
        }
        return result;
    }

    void XMakeProjectImporter::deleteDirectoryData(void *directoryData) const {
        delete static_cast<DirectoryData *>(directoryData);
    }

    void XMakeProjectImporter::cleanupTemporaryXMake(Kit *k, const QVariantList &vl) {
        if (vl.isEmpty()) {
            return; // No temporary XMake
        }
        QTC_ASSERT(vl.count() == 1, return );
        XMakeKitAspect::setXMakeTool(k, Id()); // Always mark Kit as not using this Qt
        XMakeToolManager::deregisterXMakeTool(Id::fromSetting(vl.at(0)));
        qCDebug(cmInputLog) << "Temporary XMake tool cleaned up.";
    }

    void XMakeProjectImporter::persistTemporaryXMake(Kit *k, const QVariantList &vl) {
        if (vl.isEmpty()) {
            return; // No temporary XMake
        }
        QTC_ASSERT(vl.count() == 1, return );
        const QVariant &data = vl.at(0);
        XMakeTool *tmpCmake = XMakeToolManager::findById(Id::fromSetting(data));
        XMakeTool *actualCmake = XMakeKitAspect::xmakeTool(k);

        // User changed Kit away from temporary XMake that was set up:
        if (tmpCmake && actualCmake != tmpCmake) {
            XMakeToolManager::deregisterXMakeTool(tmpCmake->id());
        }

        qCDebug(cmInputLog) << "Temporary XMake tool made persistent.";
    }
} // XMakeProjectManager::Internal

#ifdef WITH_TESTS

#include <QTest>

namespace XMakeProjectManager::Internal {
    class XMakeProjectImporterTest final : public QObject {
        Q_OBJECT

private slots:
        void testXMakeProjectImporterQt_data();
        void testXMakeProjectImporterQt();

        void testXMakeProjectImporterToolchain_data();
        void testXMakeProjectImporterToolchain();
    };

    void XMakeProjectImporterTest::testXMakeProjectImporterQt_data() {
        QTest::addColumn<QStringList>("cache");
        QTest::addColumn<QString>("expectedQmake");

        QTest::newRow("Empty input")
            << QStringList() << QString();

        QTest::newRow("Qt4")
            << QStringList({ QString::fromLatin1("QT_QMAKE_EXECUTABLE=/usr/bin/xxx/qmake") })
            << "/usr/bin/xxx/qmake";

        // Everything else will require Qt installations!
    }

    void XMakeProjectImporterTest::testXMakeProjectImporterQt() {
        QFETCH(QStringList, cache);
        QFETCH(QString, expectedQmake);

        XMakeConfig config;
        for (const QString &c : std::as_const(cache)) {
            const int pos = c.indexOf('=');
            Q_ASSERT(pos > 0);
            const QString key = c.left(pos);
            const QString value = c.mid(pos + 1);
            config.append(XMakeConfigItem(key.toUtf8(), value.toUtf8()));
        }

        auto [realQmake, xmakePrefixPath] = qtInfoFromXMakeCache(config,
                                                                 Environment::systemEnvironment());
        QCOMPARE(realQmake.path(), expectedQmake);
    }
    void XMakeProjectImporterTest::testXMakeProjectImporterToolchain_data() {
        QTest::addColumn<QStringList>("cache");
        QTest::addColumn<QByteArrayList>("expectedLanguages");
        QTest::addColumn<QStringList>("expectedToolchains");

        QTest::newRow("Empty input")
            << QStringList() << QByteArrayList() << QStringList();

        QTest::newRow("Unrelated input")
            << QStringList("XMAKE_SOMETHING_ELSE=/tmp") << QByteArrayList() << QStringList();
        QTest::newRow("CXX compiler")
            << QStringList({ "XMAKE_CXX_COMPILER=/usr/bin/g++" })
            << QByteArrayList({ "Cxx" })
            << QStringList({ "/usr/bin/g++" });
        QTest::newRow("CXX compiler, C compiler")
            << QStringList({ "XMAKE_CXX_COMPILER=/usr/bin/g++", "XMAKE_C_COMPILER=/usr/bin/clang" })
            << QByteArrayList({ "Cxx", "C" })
            << QStringList({ "/usr/bin/g++", "/usr/bin/clang" });
        QTest::newRow("CXX compiler, C compiler, strange compiler")
            << QStringList({ "XMAKE_CXX_COMPILER=/usr/bin/g++",
                             "XMAKE_C_COMPILER=/usr/bin/clang",
                             "XMAKE_STRANGE_LANGUAGE_COMPILER=/tmp/strange/compiler" })
            << QByteArrayList({ "Cxx", "C", "STRANGE_LANGUAGE" })
            << QStringList({ "/usr/bin/g++", "/usr/bin/clang", "/tmp/strange/compiler" });
        QTest::newRow("CXX compiler, C compiler, strange compiler (with junk)")
            << QStringList({ "FOO=test",
                             "XMAKE_CXX_COMPILER=/usr/bin/g++",
                             "XMAKE_BUILD_TYPE=debug",
                             "XMAKE_C_COMPILER=/usr/bin/clang",
                             "SOMETHING_COMPILER=/usr/bin/something",
                             "XMAKE_STRANGE_LANGUAGE_COMPILER=/tmp/strange/compiler",
                             "BAR=more test" })
            << QByteArrayList({ "Cxx", "C", "STRANGE_LANGUAGE" })
            << QStringList({ "/usr/bin/g++", "/usr/bin/clang", "/tmp/strange/compiler" });
    }

    void XMakeProjectImporterTest::testXMakeProjectImporterToolchain() {
        QFETCH(QStringList, cache);
        QFETCH(QByteArrayList, expectedLanguages);
        QFETCH(QStringList, expectedToolchains);

        QCOMPARE(expectedLanguages.count(), expectedToolchains.count());

        XMakeConfig config;
        for (const QString &c : std::as_const(cache)) {
            const int pos = c.indexOf('=');
            Q_ASSERT(pos > 0);
            const QString key = c.left(pos);
            const QString value = c.mid(pos + 1);
            config.append(XMakeConfigItem(key.toUtf8(), value.toUtf8()));
        }

        const QVector<ToolchainDescription> tcs = extractToolchainsFromCache(config);
        QCOMPARE(tcs.count(), expectedLanguages.count());
        for (int i = 0; i < tcs.count(); ++i) {
            QCOMPARE(tcs.at(i).language, expectedLanguages.at(i));
            QCOMPARE(tcs.at(i).compilerPath.toString(), expectedToolchains.at(i));
        }
    }

    QObject *createXMakeProjectImporterTest() {
        return new XMakeProjectImporterTest;
    }
} // XMakeProjectManager::Internal

#endif

#include "xmakeprojectimporter.moc"
