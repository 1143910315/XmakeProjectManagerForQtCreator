// Copyright (C) 2016 Canonical Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "xmakekitaspect.h"

#include "xmakeconfigitem.h"
#include "xmakeprojectconstants.h"
#include "xmakeprojectmanagertr.h"
#include "xmakespecificsettings.h"
#include "xmaketool.h"
#include "xmaketoolmanager.h"

#include <coreplugin/icore.h>

#include <ios/iosconstants.h>

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorersettings.h>
#include <projectexplorer/task.h>
#include <projectexplorer/toolchain.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>

#include <utils/algorithm.h>
#include <utils/commandline.h>
#include <utils/elidinglabel.h>
#include <utils/environment.h>
#include <utils/guard.h>
#include <utils/layoutbuilder.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>
#include <utils/variablechooser.h>

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGuiApplication>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>

using namespace ProjectExplorer;
using namespace Utils;

namespace XMakeProjectManager {

static bool isIos(const Kit *k)
{
    const Id deviceType = DeviceTypeKitAspect::deviceTypeId(k);
    return deviceType == Ios::Constants::IOS_DEVICE_TYPE
           || deviceType == Ios::Constants::IOS_SIMULATOR_TYPE;
}

static Id defaultXMakeToolId()
{
    XMakeTool *defaultTool = XMakeToolManager::defaultXMakeTool();
    return defaultTool ? defaultTool->id() : Id();
}

// Factories

class XMakeKitAspectFactory : public KitAspectFactory
{
public:
    XMakeKitAspectFactory();

    // KitAspect interface
    Tasks validate(const Kit *k) const final;
    void setup(Kit *k) final;
    void fix(Kit *k) final;
    ItemList toUserOutput(const Kit *k) const final;
    KitAspect *createKitAspect(Kit *k) const final;

    void addToMacroExpander(Kit *k, Utils::MacroExpander *expander) const final;

    QSet<Utils::Id> availableFeatures(const Kit *k) const final;
};

class XMakeGeneratorKitAspectFactory : public KitAspectFactory
{
public:
    XMakeGeneratorKitAspectFactory();

    Tasks validate(const Kit *k) const final;
    void setup(Kit *k) final;
    void fix(Kit *k) final;
    void upgrade(Kit *k) final;
    ItemList toUserOutput(const Kit *k) const final;
    KitAspect *createKitAspect(Kit *k) const final;
    void addToBuildEnvironment(const Kit *k, Utils::Environment &env) const final;

private:
    QVariant defaultValue(const Kit *k) const;
};

class XMakeConfigurationKitAspectFactory : public KitAspectFactory
{
public:
    XMakeConfigurationKitAspectFactory();

    // KitAspect interface
    Tasks validate(const Kit *k) const final;
    void setup(Kit *k) final;
    void fix(Kit *k) final;
    ItemList toUserOutput(const Kit *k) const final;
    KitAspect *createKitAspect(Kit *k) const final;

private:
    QVariant defaultValue(const Kit *k) const;
};

// Implementations

class XMakeKitAspectImpl final : public KitAspect
{
public:
    XMakeKitAspectImpl(Kit *kit, const KitAspectFactory *factory)
        : KitAspect(kit, factory), m_comboBox(createSubWidget<QComboBox>())
    {
        setManagingPage(Constants::Settings::TOOLS_ID);
        m_comboBox->setSizePolicy(QSizePolicy::Ignored, m_comboBox->sizePolicy().verticalPolicy());
        m_comboBox->setEnabled(false);
        m_comboBox->setToolTip(factory->description());

        refresh();

        connect(m_comboBox, &QComboBox::currentIndexChanged,
                this, &XMakeKitAspectImpl::currentXMakeToolChanged);

        XMakeToolManager *xmakeMgr = XMakeToolManager::instance();
        connect(xmakeMgr, &XMakeToolManager::xmakeAdded, this, &XMakeKitAspectImpl::refresh);
        connect(xmakeMgr, &XMakeToolManager::xmakeRemoved, this, &XMakeKitAspectImpl::refresh);
        connect(xmakeMgr, &XMakeToolManager::xmakeUpdated, this, &XMakeKitAspectImpl::refresh);
    }

    ~XMakeKitAspectImpl() override
    {
        delete m_comboBox;
    }

private:
    // KitAspectWidget interface
    void makeReadOnly() override { m_comboBox->setEnabled(false); }

    void addToLayoutImpl(Layouting::LayoutItem &builder) override
    {
        addMutableAction(m_comboBox);
        builder.addItem(m_comboBox);
    }

    void refresh() override
    {
        const GuardLocker locker(m_ignoreChanges);
        m_comboBox->clear();

        IDeviceConstPtr device = BuildDeviceKitAspect::device(kit());
        const FilePath rootPath = device->rootPath();

        const auto list = XMakeToolManager::xmakeTools();

        m_comboBox->setEnabled(!list.isEmpty());

        if (list.isEmpty()) {
            m_comboBox->addItem(Tr::tr("<No XMake Tool available>"), Id().toSetting());
            return;
        }

        const QList<XMakeTool *> same = Utils::filtered(list, [rootPath](XMakeTool *item) {
            return item->xmakeExecutable().isSameDevice(rootPath);
        });
        const QList<XMakeTool *> other = Utils::filtered(list, [rootPath](XMakeTool *item) {
            return !item->xmakeExecutable().isSameDevice(rootPath);
        });

        for (XMakeTool *item : same)
            m_comboBox->addItem(item->displayName(), item->id().toSetting());

        if (!same.isEmpty() && !other.isEmpty())
            m_comboBox->insertSeparator(m_comboBox->count());

        for (XMakeTool *item : other)
            m_comboBox->addItem(item->displayName(), item->id().toSetting());

        XMakeTool *tool = XMakeKitAspect::xmakeTool(m_kit);
        m_comboBox->setCurrentIndex(tool ? indexOf(tool->id()) : -1);
    }

    int indexOf(Id id)
    {
        for (int i = 0; i < m_comboBox->count(); ++i) {
            if (id == Id::fromSetting(m_comboBox->itemData(i)))
                return i;
        }
        return -1;
    }

    void currentXMakeToolChanged(int index)
    {
        if (m_ignoreChanges.isLocked())
            return;

        const Id id = Id::fromSetting(m_comboBox->itemData(index));
        XMakeKitAspect::setXMakeTool(m_kit, id);
    }

    Guard m_ignoreChanges;
    QComboBox *m_comboBox;
};

XMakeKitAspectFactory::XMakeKitAspectFactory()
{
    setId(Constants::TOOL_ID);
    setDisplayName(Tr::tr("XMake Tool"));
    setDescription(Tr::tr("The XMake Tool to use when building a project with XMake.<br>"
                      "This setting is ignored when using other build systems."));
    setPriority(20000);

    auto updateKits = [this] {
        if (KitManager::isLoaded()) {
            for (Kit *k : KitManager::kits())
                fix(k);
        }
    };

    //make sure the default value is set if a selected XMake is removed
    connect(XMakeToolManager::instance(), &XMakeToolManager::xmakeRemoved, this, updateKits);

    //make sure the default value is set if a new default XMake is set
    connect(XMakeToolManager::instance(), &XMakeToolManager::defaultXMakeChanged,
            this, updateKits);
}

Id XMakeKitAspect::id()
{
    return Constants::TOOL_ID;
}

Id XMakeKitAspect::xmakeToolId(const Kit *k)
{
    if (!k)
        return {};
    return Id::fromSetting(k->value(Constants::TOOL_ID));
}

XMakeTool *XMakeKitAspect::xmakeTool(const Kit *k)
{
    return k->isAspectRelevant(id()) ? XMakeToolManager::findById(xmakeToolId(k)) : nullptr;
}

void XMakeKitAspect::setXMakeTool(Kit *k, const Id id)
{
    const Id toSet = id.isValid() ? id : defaultXMakeToolId();
    QTC_ASSERT(!id.isValid() || XMakeToolManager::findById(toSet), return);
    if (k)
        k->setValue(Constants::TOOL_ID, toSet.toSetting());
}

Tasks XMakeKitAspectFactory::validate(const Kit *k) const
{
    Tasks result;
    XMakeTool *tool = XMakeKitAspect::xmakeTool(k);
    if (tool && tool->isValid()) {
        XMakeTool::Version version = tool->version();
        if (version.major < 3 || (version.major == 3 && version.minor < 14)) {
            result << BuildSystemTask(Task::Warning,
                XMakeKitAspect::msgUnsupportedVersion(version.fullVersion));
        }
    }
    return result;
}

void XMakeKitAspectFactory::setup(Kit *k)
{
    XMakeTool *tool = XMakeKitAspect::xmakeTool(k);
    if (tool)
        return;

    // Look for a suitable auto-detected one:
    const QString kitSource = k->autoDetectionSource();
    for (XMakeTool *tool : XMakeToolManager::xmakeTools()) {
        const QString toolSource = tool->detectionSource();
        if (!toolSource.isEmpty() && toolSource == kitSource) {
            XMakeKitAspect::setXMakeTool(k, tool->id());
            return;
        }
    }

    XMakeKitAspect::setXMakeTool(k, defaultXMakeToolId());
}

void XMakeKitAspectFactory::fix(Kit *k)
{
    setup(k);
}

KitAspectFactory::ItemList XMakeKitAspectFactory::toUserOutput(const Kit *k) const
{
    const XMakeTool *const tool = XMakeKitAspect::xmakeTool(k);
    return {{Tr::tr("XMake"), tool ? tool->displayName() : Tr::tr("Unconfigured")}};
}

KitAspect *XMakeKitAspectFactory::createKitAspect(Kit *k) const
{
    QTC_ASSERT(k, return nullptr);
    return new XMakeKitAspectImpl(k, this);
}

void XMakeKitAspectFactory::addToMacroExpander(Kit *k, MacroExpander *expander) const
{
    QTC_ASSERT(k, return);
    expander->registerFileVariables("XMake:Executable", Tr::tr("Path to the xmake executable"),
        [k] {
            XMakeTool *tool = XMakeKitAspect::xmakeTool(k);
            return tool ? tool->xmakeExecutable() : FilePath();
        });
}

QSet<Id> XMakeKitAspectFactory::availableFeatures(const Kit *k) const
{
    if (XMakeKitAspect::xmakeTool(k))
        return { XMakeProjectManager::Constants::XMAKE_FEATURE_ID };
    return {};
}

QString XMakeKitAspect::msgUnsupportedVersion(const QByteArray &versionString)
{
    return Tr::tr("XMake version %1 is unsupported. Update to "
              "version 3.15 (with file-api) or later.")
        .arg(QString::fromUtf8(versionString));
}

// --------------------------------------------------------------------
// XMakeGeneratorKitAspect:
// --------------------------------------------------------------------

const char GENERATOR_ID[] = "XMake.GeneratorKitInformation";

const char GENERATOR_KEY[] = "Generator";
const char EXTRA_GENERATOR_KEY[] = "ExtraGenerator";
const char PLATFORM_KEY[] = "Platform";
const char TOOLSET_KEY[] = "Toolset";

class XMakeGeneratorKitAspectImpl final : public KitAspect
{
public:
    XMakeGeneratorKitAspectImpl(Kit *kit, const KitAspectFactory *factory)
        : KitAspect(kit, factory),
          m_label(createSubWidget<ElidingLabel>()),
          m_changeButton(createSubWidget<QPushButton>())
    {
        const XMakeTool *tool = XMakeKitAspect::xmakeTool(kit);
        connect(this, &KitAspect::labelLinkActivated, this, [=](const QString &) {
            XMakeTool::openXMakeHelpUrl(tool, "%1/manual/xmake-generators.7.html");
        });

        m_label->setToolTip(factory->description());
        m_changeButton->setText(Tr::tr("Change..."));
        refresh();
        connect(m_changeButton, &QPushButton::clicked,
                this, &XMakeGeneratorKitAspectImpl::changeGenerator);
    }

    ~XMakeGeneratorKitAspectImpl() override
    {
        delete m_label;
        delete m_changeButton;
    }

private:
    // KitAspectWidget interface
    void makeReadOnly() override { m_changeButton->setEnabled(false); }

    void addToLayoutImpl(Layouting::LayoutItem &parent) override
    {
        addMutableAction(m_label);
        parent.addItem(m_label);
        parent.addItem(m_changeButton);
    }

    void refresh() override
    {
        XMakeTool *const tool = XMakeKitAspect::xmakeTool(m_kit);
        if (tool != m_currentTool)
            m_currentTool = tool;

        m_changeButton->setEnabled(m_currentTool);
        const QString generator = XMakeGeneratorKitAspect::generator(kit());
        const QString platform = XMakeGeneratorKitAspect::platform(kit());
        const QString toolset = XMakeGeneratorKitAspect::toolset(kit());

        QStringList messageLabel;
        messageLabel << generator;

        if (!platform.isEmpty())
            messageLabel << ", " << Tr::tr("Platform") << ": " << platform;
        if (!toolset.isEmpty())
            messageLabel << ", " << Tr::tr("Toolset") << ": " << toolset;

        m_label->setText(messageLabel.join(""));
    }

    void changeGenerator()
    {
        QPointer<QDialog> changeDialog = new QDialog(m_changeButton);

        // Disable help button in titlebar on windows:
        Qt::WindowFlags flags = changeDialog->windowFlags();
        flags |= Qt::MSWindowsFixedSizeDialogHint;
        changeDialog->setWindowFlags(flags);

        changeDialog->setWindowTitle(Tr::tr("XMake Generator"));

        auto layout = new QGridLayout(changeDialog);
        layout->setSizeConstraint(QLayout::SetFixedSize);

        auto xmakeLabel = new QLabel;
        xmakeLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

        auto generatorCombo = new QComboBox;
        auto platformEdit = new QLineEdit;
        auto toolsetEdit = new QLineEdit;

        int row = 0;
        layout->addWidget(new QLabel(QLatin1String("Executable:")));
        layout->addWidget(xmakeLabel, row, 1);

        ++row;
        layout->addWidget(new QLabel(Tr::tr("Generator:")), row, 0);
        layout->addWidget(generatorCombo, row, 1);

        ++row;
        layout->addWidget(new QLabel(Tr::tr("Platform:")), row, 0);
        layout->addWidget(platformEdit, row, 1);

        ++row;
        layout->addWidget(new QLabel(Tr::tr("Toolset:")), row, 0);
        layout->addWidget(toolsetEdit, row, 1);

        ++row;
        auto bb = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
        layout->addWidget(bb, row, 0, 1, 2);

        connect(bb, &QDialogButtonBox::accepted, changeDialog.data(), &QDialog::accept);
        connect(bb, &QDialogButtonBox::rejected, changeDialog.data(), &QDialog::reject);

        xmakeLabel->setText(m_currentTool->xmakeExecutable().toUserOutput());

        const QList<XMakeTool::Generator> generatorList = Utils::sorted(
                    m_currentTool->supportedGenerators(), &XMakeTool::Generator::name);

        for (auto it = generatorList.constBegin(); it != generatorList.constEnd(); ++it)
            generatorCombo->addItem(it->name);

        auto updateDialog = [&generatorList, generatorCombo,
                platformEdit, toolsetEdit](const QString &name) {
            const auto it = std::find_if(generatorList.constBegin(), generatorList.constEnd(),
                                   [name](const XMakeTool::Generator &g) { return g.name == name; });
            QTC_ASSERT(it != generatorList.constEnd(), return);
            generatorCombo->setCurrentText(name);

            platformEdit->setEnabled(it->supportsPlatform);
            toolsetEdit->setEnabled(it->supportsToolset);
        };

        updateDialog(XMakeGeneratorKitAspect::generator(kit()));

        generatorCombo->setCurrentText(XMakeGeneratorKitAspect::generator(kit()));
        platformEdit->setText(platformEdit->isEnabled() ? XMakeGeneratorKitAspect::platform(kit()) : QString());
        toolsetEdit->setText(toolsetEdit->isEnabled() ? XMakeGeneratorKitAspect::toolset(kit()) : QString());

        connect(generatorCombo, &QComboBox::currentTextChanged, updateDialog);

        if (changeDialog->exec() == QDialog::Accepted) {
            if (!changeDialog)
                return;

            XMakeGeneratorKitAspect::set(kit(), generatorCombo->currentText(),
                                         platformEdit->isEnabled() ? platformEdit->text() : QString(),
                                         toolsetEdit->isEnabled() ? toolsetEdit->text() : QString());

            refresh();
        }
    }

    ElidingLabel *m_label;
    QPushButton *m_changeButton;
    XMakeTool *m_currentTool = nullptr;
};

namespace {

class GeneratorInfo
{
public:
    GeneratorInfo() = default;
    GeneratorInfo(const QString &generator_,
                  const QString &platform_ = QString(),
                  const QString &toolset_ = QString())
        : generator(generator_)
        , platform(platform_)
        , toolset(toolset_)
    {}

    QVariant toVariant() const {
        QVariantMap result;
        result.insert(GENERATOR_KEY, generator);
        result.insert(EXTRA_GENERATOR_KEY, extraGenerator);
        result.insert(PLATFORM_KEY, platform);
        result.insert(TOOLSET_KEY, toolset);
        return result;
    }
    void fromVariant(const QVariant &v) {
        const QVariantMap value = v.toMap();

        generator = value.value(GENERATOR_KEY).toString();
        extraGenerator = value.value(EXTRA_GENERATOR_KEY).toString();
        platform = value.value(PLATFORM_KEY).toString();
        toolset = value.value(TOOLSET_KEY).toString();
    }

    QString generator;
    QString extraGenerator;
    QString platform;
    QString toolset;
};

} // namespace

static GeneratorInfo generatorInfo(const Kit *k)
{
    GeneratorInfo info;
    if (!k)
        return info;

    info.fromVariant(k->value(GENERATOR_ID));
    return info;
}

static void setGeneratorInfo(Kit *k, const GeneratorInfo &info)
{
    if (!k)
        return;
    k->setValue(GENERATOR_ID, info.toVariant());
}

XMakeGeneratorKitAspectFactory::XMakeGeneratorKitAspectFactory()
{
    setId(GENERATOR_ID);
    setDisplayName(Tr::tr("XMake <a href=\"generator\">generator</a>"));
    setDescription(Tr::tr("XMake generator defines how a project is built when using XMake.<br>"
                      "This setting is ignored when using other build systems."));
    setPriority(19000);
}

QString XMakeGeneratorKitAspect::generator(const Kit *k)
{
    return generatorInfo(k).generator;
}

QString XMakeGeneratorKitAspect::platform(const Kit *k)
{
    return generatorInfo(k).platform;
}

QString XMakeGeneratorKitAspect::toolset(const Kit *k)
{
    return generatorInfo(k).toolset;
}

void XMakeGeneratorKitAspect::setGenerator(Kit *k, const QString &generator)
{
    GeneratorInfo info = generatorInfo(k);
    info.generator = generator;
    setGeneratorInfo(k, info);
}

void XMakeGeneratorKitAspect::setPlatform(Kit *k, const QString &platform)
{
    GeneratorInfo info = generatorInfo(k);
    info.platform = platform;
    setGeneratorInfo(k, info);
}

void XMakeGeneratorKitAspect::setToolset(Kit *k, const QString &toolset)
{
    GeneratorInfo info = generatorInfo(k);
    info.toolset = toolset;
    setGeneratorInfo(k, info);
}

void XMakeGeneratorKitAspect::set(Kit *k,
                                  const QString &generator,
                                  const QString &platform,
                                  const QString &toolset)
{
    GeneratorInfo info(generator, platform, toolset);
    setGeneratorInfo(k, info);
}

QStringList XMakeGeneratorKitAspect::generatorArguments(const Kit *k)
{
    QStringList result;
    GeneratorInfo info = generatorInfo(k);
    if (info.generator.isEmpty())
        return result;

    result.append("-G" + info.generator);

    if (!info.platform.isEmpty())
        result.append("-A" + info.platform);

    if (!info.toolset.isEmpty())
        result.append("-T" + info.toolset);

    return result;
}

XMakeConfig XMakeGeneratorKitAspect::generatorXMakeConfig(const Kit *k)
{
    XMakeConfig config;

    GeneratorInfo info = generatorInfo(k);
    if (info.generator.isEmpty())
        return config;

    config << XMakeConfigItem("XMAKE_GENERATOR", info.generator.toUtf8());

    if (!info.platform.isEmpty())
        config << XMakeConfigItem("XMAKE_GENERATOR_PLATFORM", info.platform.toUtf8());

    if (!info.toolset.isEmpty())
        config << XMakeConfigItem("XMAKE_GENERATOR_TOOLSET", info.toolset.toUtf8());

    return config;
}

bool XMakeGeneratorKitAspect::isMultiConfigGenerator(const Kit *k)
{
    const QString generator = XMakeGeneratorKitAspect::generator(k);
    return generator.indexOf("Visual Studio") != -1 ||
           generator == "Xcode" ||
           generator == "Ninja Multi-Config";
}

QVariant XMakeGeneratorKitAspectFactory::defaultValue(const Kit *k) const
{
    QTC_ASSERT(k, return QVariant());

    XMakeTool *tool = XMakeKitAspect::xmakeTool(k);
    if (!tool)
        return QVariant();

    if (isIos(k))
        return GeneratorInfo("Xcode").toVariant();

    const QList<XMakeTool::Generator> known = tool->supportedGenerators();
    auto it = std::find_if(known.constBegin(), known.constEnd(), [](const XMakeTool::Generator &g) {
        return g.matches("Ninja");
    });
    if (it != known.constEnd()) {
        const bool hasNinja = [k, tool] {
            if (Internal::settings().ninjaPath().isEmpty()) {
                auto findNinja = [](const Environment &env) -> bool {
                    return !env.searchInPath("ninja").isEmpty();
                };
                if (!findNinja(tool->filePath().deviceEnvironment()))
                    return findNinja(k->buildEnvironment());
            }
            return true;
        }();

        if (hasNinja)
            return GeneratorInfo("Ninja").toVariant();
    }

    if (tool->filePath().osType() == OsTypeWindows) {
        // *sigh* Windows with its zoo of incompatible stuff again...
        Toolchain *tc = ToolchainKitAspect::cxxToolchain(k);
        if (tc && tc->typeId() == ProjectExplorer::Constants::MINGW_TOOLCHAIN_TYPEID) {
            it = std::find_if(known.constBegin(),
                              known.constEnd(),
                              [](const XMakeTool::Generator &g) {
                                  return g.matches("MinGW Makefiles");
                              });
        } else {
            it = std::find_if(known.constBegin(),
                              known.constEnd(),
                              [](const XMakeTool::Generator &g) {
                                  return g.matches("NMake Makefiles")
                                         || g.matches("NMake Makefiles JOM");
                              });
            if (ProjectExplorerPlugin::projectExplorerSettings().useJom) {
                it = std::find_if(known.constBegin(),
                                  known.constEnd(),
                                  [](const XMakeTool::Generator &g) {
                                      return g.matches("NMake Makefiles JOM");
                                  });
            }

            if (it == known.constEnd()) {
                it = std::find_if(known.constBegin(),
                                  known.constEnd(),
                                  [](const XMakeTool::Generator &g) {
                                      return g.matches("NMake Makefiles");
                                  });
            }
        }
    } else {
        // Unix-oid OSes:
        it = std::find_if(known.constBegin(), known.constEnd(), [](const XMakeTool::Generator &g) {
            return g.matches("Unix Makefiles");
        });
    }
    if (it == known.constEnd())
        it = known.constBegin(); // Fallback to the first generator...
    if (it == known.constEnd())
        return QVariant();

    return GeneratorInfo(it->name).toVariant();
}

Tasks XMakeGeneratorKitAspectFactory::validate(const Kit *k) const
{
    XMakeTool *tool = XMakeKitAspect::xmakeTool(k);
    if (!tool)
        return {};

    Tasks result;
    const auto addWarning = [&result](const QString &desc) {
        result << BuildSystemTask(Task::Warning, desc);
    };

    if (!tool->isValid()) {
        addWarning(Tr::tr("XMake Tool is unconfigured, XMake generator will be ignored."));
    } else {
        const GeneratorInfo info = generatorInfo(k);
        QList<XMakeTool::Generator> known = tool->supportedGenerators();
        auto it = std::find_if(known.constBegin(), known.constEnd(), [info](const XMakeTool::Generator &g) {
            return g.matches(info.generator);
        });
        if (it == known.constEnd()) {
            addWarning(Tr::tr("XMake Tool does not support the configured generator."));
        } else {
            if (!it->supportsPlatform && !info.platform.isEmpty())
                addWarning(Tr::tr("Platform is not supported by the selected XMake generator."));
            if (!it->supportsToolset && !info.toolset.isEmpty())
                addWarning(Tr::tr("Toolset is not supported by the selected XMake generator."));
        }
        if (!tool->hasFileApi()) {
            addWarning(Tr::tr("The selected XMake binary does not support file-api. "
                              "%1 will not be able to parse XMake projects.")
                           .arg(QGuiApplication::applicationDisplayName()));
        }
    }

    return result;
}

void XMakeGeneratorKitAspectFactory::setup(Kit *k)
{
    if (!k || k->hasValue(id()))
        return;
    GeneratorInfo info;
    info.fromVariant(defaultValue(k));
    setGeneratorInfo(k, info);
}

void XMakeGeneratorKitAspectFactory::fix(Kit *k)
{
    const XMakeTool *tool = XMakeKitAspect::xmakeTool(k);
    const GeneratorInfo info = generatorInfo(k);

    if (!tool)
        return;
    QList<XMakeTool::Generator> known = tool->supportedGenerators();
    auto it = std::find_if(known.constBegin(), known.constEnd(),
                           [info](const XMakeTool::Generator &g) {
        return g.matches(info.generator);
    });
    if (it == known.constEnd()) {
        GeneratorInfo dv;
        dv.fromVariant(defaultValue(k));
        setGeneratorInfo(k, dv);
    } else {
        const GeneratorInfo dv(info.generator,
                               it->supportsPlatform ? info.platform : QString(),
                               it->supportsToolset ? info.toolset : QString());
        setGeneratorInfo(k, dv);
    }
}

void XMakeGeneratorKitAspectFactory::upgrade(Kit *k)
{
    QTC_ASSERT(k, return);

    const QVariant value = k->value(GENERATOR_ID);
    if (value.type() != QVariant::Map) {
        GeneratorInfo info;
        const QString fullName = value.toString();
        const int pos = fullName.indexOf(" - ");
        if (pos >= 0) {
            info.generator = fullName.mid(pos + 3);
            info.extraGenerator = fullName.mid(0, pos);
        } else {
            info.generator = fullName;
        }
        setGeneratorInfo(k, info);
    }
}

KitAspectFactory::ItemList XMakeGeneratorKitAspectFactory::toUserOutput(const Kit *k) const
{
    const GeneratorInfo info = generatorInfo(k);
    QString message;
    if (info.generator.isEmpty()) {
        message = Tr::tr("<Use Default Generator>");
    } else {
        message = Tr::tr("Generator: %1<br>Extra generator: %2").arg(info.generator).arg(info.extraGenerator);
        if (!info.platform.isEmpty())
            message += "<br/>" + Tr::tr("Platform: %1").arg(info.platform);
        if (!info.toolset.isEmpty())
            message += "<br/>" + Tr::tr("Toolset: %1").arg(info.toolset);
    }
    return {{Tr::tr("XMake Generator"), message}};
}

KitAspect *XMakeGeneratorKitAspectFactory::createKitAspect(Kit *k) const
{
    return new XMakeGeneratorKitAspectImpl(k, this);
}

void XMakeGeneratorKitAspectFactory::addToBuildEnvironment(const Kit *k, Environment &env) const
{
    GeneratorInfo info = generatorInfo(k);
    if (info.generator == "NMake Makefiles JOM") {
        if (env.searchInPath("jom.exe").exists())
            return;
        env.appendOrSetPath(Core::ICore::libexecPath());
        env.appendOrSetPath(Core::ICore::libexecPath("jom"));
    }
}

// --------------------------------------------------------------------
// XMakeConfigurationKitAspect:
// --------------------------------------------------------------------

const char CONFIGURATION_ID[] = "XMake.ConfigurationKitInformation";
const char ADDITIONAL_CONFIGURATION_ID[] = "XMake.AdditionalConfigurationParameters";

const char XMAKE_C_TOOLCHAIN_KEY[] = "XMAKE_C_COMPILER";
const char XMAKE_CXX_TOOLCHAIN_KEY[] = "XMAKE_CXX_COMPILER";
const char XMAKE_QMAKE_KEY[] = "QT_QMAKE_EXECUTABLE";
const char XMAKE_PREFIX_PATH_KEY[] = "XMAKE_PREFIX_PATH";
const char QTC_XMAKE_PRESET_KEY[] = "QTC_XMAKE_PRESET";

class XMakeConfigurationKitAspectWidget final : public KitAspect
{
public:
    XMakeConfigurationKitAspectWidget(Kit *kit, const KitAspectFactory *factory)
        : KitAspect(kit, factory),
          m_summaryLabel(createSubWidget<ElidingLabel>()),
          m_manageButton(createSubWidget<QPushButton>())
    {
        refresh();
        m_manageButton->setText(Tr::tr("Change..."));
        connect(m_manageButton, &QAbstractButton::clicked,
                this, &XMakeConfigurationKitAspectWidget::editConfigurationChanges);
    }

private:
    // KitAspectWidget interface
    void addToLayoutImpl(Layouting::LayoutItem &parent) override
    {
        addMutableAction(m_summaryLabel);
        parent.addItem(m_summaryLabel);
        parent.addItem(m_manageButton);
    }

    void makeReadOnly() override
    {
        m_manageButton->setEnabled(false);
        if (m_dialog)
            m_dialog->reject();
    }

    void refresh() override
    {
        const QStringList current = XMakeConfigurationKitAspect::toArgumentsList(kit());
        const QString additionalText = XMakeConfigurationKitAspect::additionalConfiguration(kit());
        const QString labelText = additionalText.isEmpty()
                                      ? current.join(' ')
                                      : current.join(' ') + " " + additionalText;

        m_summaryLabel->setText(labelText);

        if (m_editor)
            m_editor->setPlainText(current.join('\n'));

        if (m_additionalEditor)
            m_additionalEditor->setText(additionalText);
    }

    void editConfigurationChanges()
    {
        if (m_dialog) {
            m_dialog->activateWindow();
            m_dialog->raise();
            return;
        }

        QTC_ASSERT(!m_editor, return);

        const XMakeTool *tool = XMakeKitAspect::xmakeTool(kit());

        m_dialog = new QDialog(m_summaryLabel->window());
        m_dialog->setWindowTitle(Tr::tr("Edit XMake Configuration"));
        auto layout = new QVBoxLayout(m_dialog);
        m_editor = new QPlainTextEdit;
        auto editorLabel = new QLabel(m_dialog);
        editorLabel->setText(Tr::tr("Enter one XMake <a href=\"variable\">variable</a> per line.<br/>"
                                "To set a variable, use -D&lt;variable&gt;:&lt;type&gt;=&lt;value&gt;.<br/>"
                                "&lt;type&gt; can have one of the following values: FILEPATH, PATH, "
                                "BOOL, INTERNAL, or STRING."));
        connect(editorLabel, &QLabel::linkActivated, this, [=](const QString &) {
            XMakeTool::openXMakeHelpUrl(tool, "%1/manual/xmake-variables.7.html");
        });
        m_editor->setMinimumSize(800, 200);

        auto chooser = new VariableChooser(m_dialog);
        chooser->addSupportedWidget(m_editor);
        chooser->addMacroExpanderProvider([this] { return kit()->macroExpander(); });

        m_additionalEditor = new QLineEdit;
        auto additionalLabel = new QLabel(m_dialog);
        additionalLabel->setText(Tr::tr("Additional XMake <a href=\"options\">options</a>:"));
        connect(additionalLabel, &QLabel::linkActivated, this, [=](const QString &) {
            XMakeTool::openXMakeHelpUrl(tool, "%1/manual/xmake.1.html#options");
        });

        auto additionalChooser = new VariableChooser(m_dialog);
        additionalChooser->addSupportedWidget(m_additionalEditor);
        additionalChooser->addMacroExpanderProvider([this] { return kit()->macroExpander(); });

        auto additionalLayout = new QHBoxLayout();
        additionalLayout->addWidget(additionalLabel);
        additionalLayout->addWidget(m_additionalEditor);

        auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Apply
                                            |QDialogButtonBox::Reset|QDialogButtonBox::Cancel);

        layout->addWidget(m_editor);
        layout->addWidget(editorLabel);
        layout->addLayout(additionalLayout);
        layout->addWidget(buttons);

        connect(buttons, &QDialogButtonBox::accepted, m_dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, m_dialog, &QDialog::reject);
        connect(buttons, &QDialogButtonBox::clicked, m_dialog, [buttons, this](QAbstractButton *button) {
            if (button != buttons->button(QDialogButtonBox::Reset))
                return;
            KitGuard guard(kit());
            XMakeConfigurationKitAspect::setConfiguration(kit(),
                                                          XMakeConfigurationKitAspect::defaultConfiguration(kit()));
            XMakeConfigurationKitAspect::setAdditionalConfiguration(kit(), QString());
        });
        connect(m_dialog, &QDialog::accepted, this, &XMakeConfigurationKitAspectWidget::acceptChangesDialog);
        connect(m_dialog, &QDialog::rejected, this, &XMakeConfigurationKitAspectWidget::closeChangesDialog);
        connect(buttons->button(QDialogButtonBox::Apply), &QAbstractButton::clicked,
                this, &XMakeConfigurationKitAspectWidget::applyChanges);

        refresh();
        m_dialog->show();
    }

    void applyChanges()
    {
        QTC_ASSERT(m_editor, return);
        KitGuard guard(kit());

        QStringList unknownOptions;
        const XMakeConfig config = XMakeConfig::fromArguments(m_editor->toPlainText().split('\n'),
                                                              unknownOptions);
        XMakeConfigurationKitAspect::setConfiguration(kit(), config);

        QString additionalConfiguration = m_additionalEditor->text();
        if (!unknownOptions.isEmpty()) {
            if (!additionalConfiguration.isEmpty())
                additionalConfiguration += " ";
            additionalConfiguration += ProcessArgs::joinArgs(unknownOptions);
        }
        XMakeConfigurationKitAspect::setAdditionalConfiguration(kit(), additionalConfiguration);
    }
    void closeChangesDialog()
    {
        m_dialog->deleteLater();
        m_dialog = nullptr;
        m_editor = nullptr;
        m_additionalEditor = nullptr;
    }
    void acceptChangesDialog()
    {
        applyChanges();
        closeChangesDialog();
    }

    QLabel *m_summaryLabel;
    QPushButton *m_manageButton;
    QDialog *m_dialog = nullptr;
    QPlainTextEdit *m_editor = nullptr;
    QLineEdit *m_additionalEditor = nullptr;
};


XMakeConfigurationKitAspectFactory::XMakeConfigurationKitAspectFactory()
{
    setId(CONFIGURATION_ID);
    setDisplayName(Tr::tr("XMake Configuration"));
    setDescription(Tr::tr("Default configuration passed to XMake when setting up a project."));
    setPriority(18000);
}

XMakeConfig XMakeConfigurationKitAspect::configuration(const Kit *k)
{
    if (!k)
        return XMakeConfig();
    const QStringList tmp = k->value(CONFIGURATION_ID).toStringList();
    return Utils::transform(tmp, &XMakeConfigItem::fromString);
}

void XMakeConfigurationKitAspect::setConfiguration(Kit *k, const XMakeConfig &config)
{
    if (!k)
        return;
    const QStringList tmp = Utils::transform(config.toList(),
                                             [](const XMakeConfigItem &i) { return i.toString(); });
    k->setValue(CONFIGURATION_ID, tmp);
}

QString XMakeConfigurationKitAspect::additionalConfiguration(const Kit *k)
{
    if (!k)
        return QString();
    return k->value(ADDITIONAL_CONFIGURATION_ID).toString();
}

void XMakeConfigurationKitAspect::setAdditionalConfiguration(Kit *k, const QString &config)
{
    if (!k)
        return;
    k->setValue(ADDITIONAL_CONFIGURATION_ID, config);
}

QStringList XMakeConfigurationKitAspect::toStringList(const Kit *k)
{
    QStringList current = Utils::transform(XMakeConfigurationKitAspect::configuration(k).toList(),
                                           [](const XMakeConfigItem &i) { return i.toString(); });
    current = Utils::filtered(current, [](const QString &s) { return !s.isEmpty(); });
    return current;
}

void XMakeConfigurationKitAspect::fromStringList(Kit *k, const QStringList &in)
{
    XMakeConfig result;
    for (const QString &s : in) {
        const XMakeConfigItem item = XMakeConfigItem::fromString(s);
        if (!item.key.isEmpty())
            result << item;
    }
    setConfiguration(k, result);
}

QStringList XMakeConfigurationKitAspect::toArgumentsList(const Kit *k)
{
    QStringList current = Utils::transform(XMakeConfigurationKitAspect::configuration(k).toList(),
                                           [](const XMakeConfigItem &i) {
                                               return i.toArgument(nullptr);
                                           });
    current = Utils::filtered(current, [](const QString &s) { return s != "-D" && s != "-U"; });
    return current;
}

XMakeConfig XMakeConfigurationKitAspect::defaultConfiguration(const Kit *k)
{
    Q_UNUSED(k)
    XMakeConfig config;
    // Qt4:
    config << XMakeConfigItem(XMAKE_QMAKE_KEY, XMakeConfigItem::FILEPATH, "%{Qt:qmakeExecutable}");
    // Qt5:
    config << XMakeConfigItem(XMAKE_PREFIX_PATH_KEY, XMakeConfigItem::PATH, "%{Qt:QT_INSTALL_PREFIX}");

    config << XMakeConfigItem(XMAKE_C_TOOLCHAIN_KEY, XMakeConfigItem::FILEPATH, "%{Compiler:Executable:C}");
    config << XMakeConfigItem(XMAKE_CXX_TOOLCHAIN_KEY, XMakeConfigItem::FILEPATH, "%{Compiler:Executable:Cxx}");

    return config;
}

void XMakeConfigurationKitAspect::setXMakePreset(Kit *k, const QString &presetName)
{
    XMakeConfig config = configuration(k);
    config.prepend(
        XMakeConfigItem(QTC_XMAKE_PRESET_KEY, XMakeConfigItem::INTERNAL, presetName.toUtf8()));

    setConfiguration(k, config);
}

XMakeConfigItem XMakeConfigurationKitAspect::xmakePresetConfigItem(const Kit *k)
{
    const XMakeConfig config = configuration(k);
    return Utils::findOrDefault(config, [](const XMakeConfigItem &item) {
        return item.key == QTC_XMAKE_PRESET_KEY;
    });
}

QVariant XMakeConfigurationKitAspectFactory::defaultValue(const Kit *k) const
{
    // FIXME: Convert preload scripts
    XMakeConfig config = XMakeConfigurationKitAspect::defaultConfiguration(k);
    const QStringList tmp = Utils::transform(config.toList(),
                                             [](const XMakeConfigItem &i) { return i.toString(); });
    return tmp;
}

Tasks XMakeConfigurationKitAspectFactory::validate(const Kit *k) const
{
    QTC_ASSERT(k, return Tasks());

    const XMakeTool *const xmake = XMakeKitAspect::xmakeTool(k);
    if (!xmake)
        return Tasks();

    const QtSupport::QtVersion *const version = QtSupport::QtKitAspect::qtVersion(k);
    const Toolchain *const tcC = ToolchainKitAspect::cToolchain(k);
    const Toolchain *const tcCxx = ToolchainKitAspect::cxxToolchain(k);
    const XMakeConfig config = XMakeConfigurationKitAspect::configuration(k);

    const bool isQt4 = version && version->qtVersion() < QVersionNumber(5, 0, 0);
    FilePath qmakePath; // This is relative to the xmake used for building.
    QStringList qtInstallDirs; // This is relativ to the xmake used for building.
    FilePath tcCPath;
    FilePath tcCxxPath;
    for (const XMakeConfigItem &i : config) {
        // Do not use expand(QByteArray) as we cannot be sure the input is latin1
        const QString expandedValue = k->macroExpander()->expand(QString::fromUtf8(i.value));
        if (i.key == XMAKE_QMAKE_KEY)
            qmakePath = xmake->xmakeExecutable().withNewPath(expandedValue);
        else if (i.key == XMAKE_C_TOOLCHAIN_KEY)
            tcCPath = xmake->xmakeExecutable().withNewPath(expandedValue);
        else if (i.key == XMAKE_CXX_TOOLCHAIN_KEY)
            tcCxxPath = xmake->xmakeExecutable().withNewPath(expandedValue);
        else if (i.key == XMAKE_PREFIX_PATH_KEY)
            qtInstallDirs = XMakeConfigItem::xmakeSplitValue(expandedValue);
    }

    Tasks result;
    const auto addWarning = [&result](const QString &desc) {
        result << BuildSystemTask(Task::Warning, desc);
    };

    // Validate Qt:
    if (qmakePath.isEmpty()) {
        if (version && version->isValid() && isQt4) {
            addWarning(Tr::tr("XMake configuration has no path to qmake binary set, "
                          "even though the kit has a valid Qt version."));
        }
    } else {
        if (!version || !version->isValid()) {
            addWarning(Tr::tr("XMake configuration has a path to a qmake binary set, "
                          "even though the kit has no valid Qt version."));
        } else if (qmakePath != version->qmakeFilePath() && isQt4) {
            addWarning(Tr::tr("XMake configuration has a path to a qmake binary set "
                          "that does not match the qmake binary path "
                          "configured in the Qt version."));
        }
    }
    if (version && !qtInstallDirs.contains(version->prefix().path()) && !isQt4) {
        if (version->isValid()) {
            addWarning(Tr::tr("XMake configuration has no XMAKE_PREFIX_PATH set "
                          "that points to the kit Qt version."));
        }
    }

    // Validate Toolchains:
    if (tcCPath.isEmpty()) {
        if (tcC && tcC->isValid()) {
            addWarning(Tr::tr("XMake configuration has no path to a C compiler set, "
                           "even though the kit has a valid tool chain."));
        }
    } else {
        if (!tcC || !tcC->isValid()) {
            addWarning(Tr::tr("XMake configuration has a path to a C compiler set, "
                          "even though the kit has no valid tool chain."));
        } else if (tcCPath != tcC->compilerCommand() && tcCPath != tcCPath.withNewMappedPath(tcC->compilerCommand())) {
            addWarning(Tr::tr("XMake configuration has a path to a C compiler set "
                          "that does not match the compiler path "
                          "configured in the tool chain of the kit."));
        }
    }

    if (tcCxxPath.isEmpty()) {
        if (tcCxx && tcCxx->isValid()) {
            addWarning(Tr::tr("XMake configuration has no path to a C++ compiler set, "
                          "even though the kit has a valid tool chain."));
        }
    } else {
        if (!tcCxx || !tcCxx->isValid()) {
            addWarning(Tr::tr("XMake configuration has a path to a C++ compiler set, "
                          "even though the kit has no valid tool chain."));
        } else if (tcCxxPath != tcCxx->compilerCommand() && tcCxxPath != tcCxxPath.withNewMappedPath(tcCxx->compilerCommand())) {
            addWarning(Tr::tr("XMake configuration has a path to a C++ compiler set "
                          "that does not match the compiler path "
                          "configured in the tool chain of the kit."));
        }
    }

    return result;
}

void XMakeConfigurationKitAspectFactory::setup(Kit *k)
{
    if (k && !k->hasValue(CONFIGURATION_ID))
        k->setValue(CONFIGURATION_ID, defaultValue(k));
}

void XMakeConfigurationKitAspectFactory::fix(Kit *k)
{
    Q_UNUSED(k)
}

KitAspectFactory::ItemList XMakeConfigurationKitAspectFactory::toUserOutput(const Kit *k) const
{
    return {{Tr::tr("XMake Configuration"), XMakeConfigurationKitAspect::toStringList(k).join("<br>")}};
}

KitAspect *XMakeConfigurationKitAspectFactory::createKitAspect(Kit *k) const
{
    if (!k)
        return nullptr;
    return new XMakeConfigurationKitAspectWidget(k, this);
}

// Factory instances;

XMakeKitAspectFactory &xmakeKitAspectFactory()
{
    static XMakeKitAspectFactory theXMakeKitAspectFactory;
    return theXMakeKitAspectFactory;
}

XMakeGeneratorKitAspectFactory &xmakeGeneratorKitAspectFactory()
{
    static XMakeGeneratorKitAspectFactory theXMakeGeneratorKitAspectFactory;
    return theXMakeGeneratorKitAspectFactory;
}

static XMakeConfigurationKitAspectFactory &xmakeConfigurationKitAspectFactory()
{
    static XMakeConfigurationKitAspectFactory theXMakeConfigurationKitAspectFactory;
    return theXMakeConfigurationKitAspectFactory;
}

KitAspect *XMakeKitAspect::createKitAspect(Kit *k)
{
    return xmakeKitAspectFactory().createKitAspect(k);
}

KitAspect *XMakeGeneratorKitAspect::createKitAspect(Kit *k)
{
    return xmakeGeneratorKitAspectFactory().createKitAspect(k);
}

KitAspect *XMakeConfigurationKitAspect::createKitAspect(Kit *k)
{
    return xmakeConfigurationKitAspectFactory().createKitAspect(k);
}

void Internal::setupXMakeKitAspects()
{
    xmakeKitAspectFactory();
    xmakeGeneratorKitAspectFactory();
    xmakeConfigurationKitAspectFactory();
}

} // namespace XMakeProjectManager
