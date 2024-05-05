// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "xmakebuildconfiguration.h"

#include "xmakebuildstep.h"
#include "xmakebuildsystem.h"
#include "xmakeconfigitem.h"
#include "xmakekitaspect.h"
#include "xmakeproject.h"
#include "xmakeprojectconstants.h"
#include "xmakeprojectmanagertr.h"
#include "xmakespecificsettings.h"
#include "configmodel.h"
#include "configmodelitemdelegate.h"
#include "fileapiparser.h"
#include "presetsmacros.h"
#include "presetsparser.h"

#include <android/androidconstants.h>
#include <docker/dockerconstants.h>
#include <ios/iosconstants.h>
#include <qnx/qnxconstants.h>
#include <webassembly/webassemblyconstants.h>

#include <coreplugin/fileutils.h>
#include <coreplugin/find/itemviewfind.h>
#include <coreplugin/icore.h>

#include <projectexplorer/buildaspects.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/environmentaspectwidget.h>
#include <projectexplorer/environmentwidget.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/namedwidget.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorertr.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtbuildaspects.h>
#include <qtsupport/qtkitaspect.h>

#include <utils/algorithm.h>
#include <utils/categorysortfiltermodel.h>
#include <utils/checkablemessagebox.h>
#include <utils/commandline.h>
#include <utils/detailswidget.h>
#include <utils/infolabel.h>
#include <utils/itemviews.h>
#include <utils/layoutbuilder.h>
#include <utils/mimeconstants.h>
#include <utils/progressindicator.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/variablechooser.h>

#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QHeaderView>
#include <QLoggingCategory>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTimer>

using namespace ProjectExplorer;
using namespace Utils;

using namespace XMakeProjectManager::Internal;
namespace XMakeProjectManager {
    static Q_LOGGING_CATEGORY(xmakeBuildConfigurationLog, "qtc.xmake.bc", QtWarningMsg);

    const char DEVELOPMENT_TEAM_FLAG[] = "Ios:DevelopmentTeam:Flag";
    const char PROVISIONING_PROFILE_FLAG[] = "Ios:ProvisioningProfile:Flag";
    const char XMAKE_OSX_ARCHITECTURES_FLAG[] = "XMAKE_OSX_ARCHITECTURES:DefaultFlag";
    const char QT_QML_DEBUG_FLAG[] = "Qt:QML_DEBUG_FLAG";
    const char QT_QML_DEBUG_PARAM[] = "-DQT_QML_DEBUG";
    const char XMAKE_QT6_TOOLCHAIN_FILE_ARG[]
        = "-DXMAKE_TOOLCHAIN_FILE:FILEPATH=%{Qt:QT_INSTALL_PREFIX}/lib/xmake/Qt6/qt.toolchain.xmake";
    const char XMAKE_BUILD_TYPE[] = "XMake.Build.Type";
    const char CLEAR_SYSTEM_ENVIRONMENT_KEY[] = "XMake.Configure.ClearSystemEnvironment";
    const char USER_ENVIRONMENT_CHANGES_KEY[] = "XMake.Configure.UserEnvironmentChanges";
    const char BASE_ENVIRONMENT_KEY[] = "XMake.Configure.BaseEnvironment";

    namespace Internal {
        class XMakeBuildSettingsWidget : public NamedWidget {
public:
            explicit XMakeBuildSettingsWidget(XMakeBuildConfiguration *bc);

            void setError(const QString &message);
            void setWarning(const QString &message);

private:
            void updateButtonState();
            void updateAdvancedCheckBox();
            void updateFromKit();
            void updateConfigurationStateIndex(int index);
            XMakeConfig getQmlDebugCxxFlags();
            XMakeConfig getSigningFlagsChanges();

            void updateSelection();
            void updateConfigurationStateSelection();
            bool isInitialConfiguration() const;
            void setVariableUnsetFlag(bool unsetFlag);
            QAction *createForceAction(int type, const QModelIndex &idx);

            bool eventFilter(QObject *target, QEvent *event) override;

            void batchEditConfiguration();
            void reconfigureWithInitialParameters();
            void updateInitialXMakeArguments();
            void kitXMakeConfiguration();
            void updateConfigureDetailsWidgetsSummary(
                const QStringList &configurationArguments = QStringList());

            XMakeBuildConfiguration *m_buildConfig;
            QTreeView *m_configView;
            ConfigModel *m_configModel;
            CategorySortFilterModel *m_configFilterModel;
            CategorySortFilterModel *m_configTextFilterModel;
            ProgressIndicator *m_progressIndicator;
            QPushButton *m_addButton;
            QPushButton *m_editButton;
            QPushButton *m_setButton;
            QPushButton *m_unsetButton;
            QPushButton *m_resetButton;
            QCheckBox *m_showAdvancedCheckBox;
            QTabBar *m_configurationStates;
            QPushButton *m_reconfigureButton;
            QTimer m_showProgressTimer;
            FancyLineEdit *m_filterEdit;
            InfoLabel *m_warningMessageLabel;
            DetailsWidget *m_configureDetailsWidget;

            QPushButton *m_batchEditButton = nullptr;
            QPushButton *m_kitConfiguration = nullptr;
        };

        static QModelIndex mapToSource(const QAbstractItemView *view, const QModelIndex &idx) {
            if (!idx.isValid()) {
                return idx;
            }

            QAbstractItemModel *model = view->model();
            QModelIndex result = idx;
            while (auto proxy = qobject_cast<const QSortFilterProxyModel *>(model)) {
                result = proxy->mapToSource(result);
                model = proxy->sourceModel();
            }
            return result;
        }

        XMakeBuildSettingsWidget::XMakeBuildSettingsWidget(XMakeBuildConfiguration *bc) :
            NamedWidget(Tr::tr("XMake")),
            m_buildConfig(bc),
            m_configModel(new ConfigModel(this)),
            m_configFilterModel(new CategorySortFilterModel(this)),
            m_configTextFilterModel(new CategorySortFilterModel(this)) {
            m_configureDetailsWidget = new DetailsWidget;

            updateConfigureDetailsWidgetsSummary();

            auto details = new QWidget(m_configureDetailsWidget);
            m_configureDetailsWidget->setWidget(details);

            auto buildDirAspect = bc->buildDirectoryAspect();
            buildDirAspect->setAutoApplyOnEditingFinished(true);
            connect(buildDirAspect, &BaseAspect::changed, this, [this] {
                        m_configModel->flush(); // clear out config cache...;
                    });

            connect(&m_buildConfig->buildTypeAspect, &BaseAspect::changed, this, [this] {
                        if (!m_buildConfig->xmakeBuildSystem()->isMultiConfig()) {
                            XMakeConfig config;
                            config << XMakeConfigItem("XMAKE_BUILD_TYPE",
                                                      m_buildConfig->buildTypeAspect().toUtf8());

                            m_configModel->setBatchEditConfiguration(config);
                        }
                    });

            auto qmlDebugAspect = bc->aspect<QtSupport::QmlDebuggingAspect>();
            connect(qmlDebugAspect, &QtSupport::QmlDebuggingAspect::changed, this, [this] {
                        updateButtonState();
                    });

            m_warningMessageLabel = new InfoLabel({}, InfoLabel::Warning);
            m_warningMessageLabel->setVisible(false);

            m_configurationStates = new QTabBar(this);
            m_configurationStates->addTab(Tr::tr("Initial Configuration"));
            m_configurationStates->addTab(Tr::tr("Current Configuration"));
            connect(m_configurationStates, &QTabBar::currentChanged, this, [this](int index) {
                        updateConfigurationStateIndex(index);
                    });

            m_kitConfiguration = new QPushButton(Tr::tr("Kit Configuration"));
            m_kitConfiguration->setToolTip(Tr::tr("Edit the current kit's XMake configuration."));
            m_kitConfiguration->setFixedWidth(m_kitConfiguration->sizeHint().width());
            connect(m_kitConfiguration, &QPushButton::clicked,
                    this, &XMakeBuildSettingsWidget::kitXMakeConfiguration,
                    Qt::QueuedConnection);

            m_filterEdit = new FancyLineEdit;
            m_filterEdit->setPlaceholderText(Tr::tr("Filter"));
            m_filterEdit->setFiltering(true);
            auto tree = new TreeView;
            connect(tree, &TreeView::activated,
                    tree, [tree](const QModelIndex &idx) {
                        tree->edit(idx);
                    });
            m_configView = tree;

            m_configView->viewport()->installEventFilter(this);

            m_configFilterModel->setSourceModel(m_configModel);
            m_configFilterModel->setFilterKeyColumn(0);
            m_configFilterModel->setFilterRole(ConfigModel::ItemIsAdvancedRole);
            m_configFilterModel->setFilterFixedString("0");

            m_configTextFilterModel->setSourceModel(m_configFilterModel);
            m_configTextFilterModel->setSortRole(Qt::DisplayRole);
            m_configTextFilterModel->setFilterKeyColumn(-1);
            m_configTextFilterModel->setNewItemRole(ConfigModel::ItemIsUserNew);

            connect(m_configTextFilterModel, &QAbstractItemModel::layoutChanged, this, [this] {
                        QModelIndex selectedIdx = m_configView->currentIndex();
                        if (selectedIdx.isValid()) {
                            m_configView->scrollTo(selectedIdx);
                        }
                    });

            m_configView->setModel(m_configTextFilterModel);
            m_configView->setMinimumHeight(300);
            m_configView->setSortingEnabled(true);
            m_configView->sortByColumn(0, Qt::AscendingOrder);
            m_configView->header()->setSectionResizeMode(QHeaderView::Stretch);
            m_configView->setSelectionMode(QAbstractItemView::ExtendedSelection);
            m_configView->setSelectionBehavior(QAbstractItemView::SelectItems);
            m_configView->setAlternatingRowColors(true);
            m_configView->setFrameShape(QFrame::NoFrame);
            m_configView->setItemDelegate(new ConfigModelItemDelegate(bc->project()->projectDirectory(),
                                                                      m_configView));
            m_configView->setRootIsDecorated(false);
            QFrame *findWrapper = Core::ItemViewFind::createSearchableWrapper(m_configView, Core::ItemViewFind::LightColored);
            findWrapper->setFrameStyle(QFrame::StyledPanel);

            m_progressIndicator = new ProgressIndicator(ProgressIndicatorSize::Large, findWrapper);
            m_progressIndicator->attachToWidget(findWrapper);
            m_progressIndicator->raise();
            m_progressIndicator->hide();
            m_showProgressTimer.setSingleShot(true);
            m_showProgressTimer.setInterval(50); // don't show progress for < 50ms tasks
            connect(&m_showProgressTimer, &QTimer::timeout, this, [this] {
                        m_progressIndicator->show();
                    });

            m_addButton = new QPushButton(Tr::tr("&Add"));
            m_addButton->setToolTip(Tr::tr("Add a new configuration value."));
            auto addButtonMenu = new QMenu(this);
            addButtonMenu->addAction(Tr::tr("&Boolean"))->setData(
                QVariant::fromValue(static_cast<int>(ConfigModel::DataItem::BOOLEAN)));
            addButtonMenu->addAction(Tr::tr("&String"))->setData(
                QVariant::fromValue(static_cast<int>(ConfigModel::DataItem::STRING)));
            addButtonMenu->addAction(Tr::tr("&Directory"))->setData(
                QVariant::fromValue(static_cast<int>(ConfigModel::DataItem::DIRECTORY)));
            addButtonMenu->addAction(Tr::tr("&File"))->setData(
                QVariant::fromValue(static_cast<int>(ConfigModel::DataItem::FILE)));
            m_addButton->setMenu(addButtonMenu);

            m_editButton = new QPushButton(Tr::tr("&Edit"));
            m_editButton->setToolTip(Tr::tr("Edit the current XMake configuration value."));

            m_setButton = new QPushButton(Tr::tr("&Set"));
            m_setButton->setToolTip(Tr::tr("Set a value in the XMake configuration."));

            m_unsetButton = new QPushButton(Tr::tr("&Unset"));
            m_unsetButton->setToolTip(Tr::tr("Unset a value in the XMake configuration."));

            m_resetButton = new QPushButton(Tr::tr("&Reset"));
            m_resetButton->setToolTip(Tr::tr("Reset all unapplied changes."));
            m_resetButton->setEnabled(false);

            m_batchEditButton = new QPushButton(Tr::tr("Batch Edit..."));
            m_batchEditButton->setToolTip(Tr::tr("Set or reset multiple values in the XMake configuration."));

            m_showAdvancedCheckBox = new QCheckBox(Tr::tr("Advanced"));
            m_showAdvancedCheckBox->setChecked(settings().showAdvancedOptionsByDefault());

            connect(m_configView->selectionModel(), &QItemSelectionModel::selectionChanged,
                    this, [this](const QItemSelection &, const QItemSelection &) {
                        updateSelection();
                    });

            m_reconfigureButton = new QPushButton(Tr::tr("Run XMake"));
            m_reconfigureButton->setEnabled(false);

            using namespace Layouting;
            Grid xmakeConfiguration {
                m_filterEdit, br,
                findWrapper,
                Column {
                    m_addButton,
                    m_editButton,
                    m_setButton,
                    m_unsetButton,
                    m_resetButton,
                    m_batchEditButton,
                    Space(10),
                    m_showAdvancedCheckBox,
                    st
                }
            };

            auto configureEnvironmentAspectWidget = bc->configureEnv.createConfigWidget();
            configureEnvironmentAspectWidget->setContentsMargins(0, 0, 0, 0);
            configureEnvironmentAspectWidget->layout()->setContentsMargins(0, 0, 0, 0);

            Column {
                Form {
                    buildDirAspect, br,
                    bc->buildTypeAspect, br,
                    qmlDebugAspect
                },
                m_warningMessageLabel,
                m_kitConfiguration,
                Column {
                    m_configurationStates,
                    Group {
                        Column {
                            xmakeConfiguration,
                            Row {
                                bc->initialXMakeArguments, br,
                                bc->additionalXMakeOptions
                            },
                            m_reconfigureButton,
                        }
                    },
                    configureEnvironmentAspectWidget
                },
                noMargin
            }.attachTo(details);

            Column {
                m_configureDetailsWidget,
                noMargin
            }.attachTo(this);

            updateAdvancedCheckBox();

            XMakeBuildSystem *bs = m_buildConfig->xmakeBuildSystem();
            setError(bs->error());
            setWarning(bs->warning());

            connect(bs, &BuildSystem::parsingStarted, this, [this] {
                        updateButtonState();
                        m_configView->setEnabled(false);
                        m_showProgressTimer.start();
                    });

            m_configModel->setMacroExpander(bc->macroExpander());

            if (bs->isParsing()) {
                m_showProgressTimer.start();
            } else {
                m_configModel->setConfiguration(bs->configurationFromXMake());
                m_configModel->setInitialParametersConfiguration(
                    m_buildConfig->initialXMakeArguments.xmakeConfiguration());
            }

            connect(bs, &BuildSystem::parsingFinished, this, [this, bs] {
                        const XMakeConfig config = bs->configurationFromXMake();
                        const TriState qmlDebugSetting = m_buildConfig->qmlDebugging();
                        bool qmlDebugConfig = XMakeBuildConfiguration::hasQmlDebugging(config);
                        if ((qmlDebugSetting == TriState::Enabled && !qmlDebugConfig)
                            || (qmlDebugSetting == TriState::Disabled && qmlDebugConfig)) {
                            m_buildConfig->qmlDebugging.setValue(TriState::Default);
                        }
                        m_configModel->setConfiguration(config);
                        m_configModel->setInitialParametersConfiguration(
                            m_buildConfig->initialXMakeArguments.xmakeConfiguration());
                        m_buildConfig->filterConfigArgumentsFromAdditionalXMakeArguments();
                        updateFromKit();
                        m_configView->setEnabled(true);
                        updateButtonState();
                        m_showProgressTimer.stop();
                        m_progressIndicator->hide();
                        updateConfigurationStateSelection();
                    });

            connect(bs, &XMakeBuildSystem::configurationCleared, this, [this] {
                        updateConfigurationStateSelection();
                    });

            connect(bs, &XMakeBuildSystem::errorOccurred, this, [this] {
                        m_showProgressTimer.stop();
                        m_progressIndicator->hide();
                        updateConfigurationStateSelection();
                    });

            connect(m_configModel, &QAbstractItemModel::dataChanged,
                    this, &XMakeBuildSettingsWidget::updateButtonState);
            connect(m_configModel, &QAbstractItemModel::modelReset,
                    this, &XMakeBuildSettingsWidget::updateButtonState);

            connect(m_buildConfig, &XMakeBuildConfiguration::signingFlagsChanged,
                    this, &XMakeBuildSettingsWidget::updateButtonState);

            connect(m_showAdvancedCheckBox, &QCheckBox::stateChanged,
                    this, &XMakeBuildSettingsWidget::updateAdvancedCheckBox);

            connect(m_filterEdit,
                    &QLineEdit::textChanged,
                    m_configTextFilterModel,
                    [this](const QString &txt) {
                        m_configTextFilterModel->setFilterRegularExpression(
                            QRegularExpression(QRegularExpression::escape(txt),
                                               QRegularExpression::CaseInsensitiveOption));
                    });

            connect(m_resetButton, &QPushButton::clicked, this, [this] {
                        m_configModel->resetAllChanges(isInitialConfiguration());
                    });
            connect(m_reconfigureButton, &QPushButton::clicked, this, [this, bs] {
                        if (!bs->isParsing()) {
                            if (isInitialConfiguration()) {
                                reconfigureWithInitialParameters();
                            } else {
                                bs->runXMakeWithExtraArguments();
                            }
                        } else {
                            bs->stopXMakeRun();
                            m_reconfigureButton->setEnabled(false);
                        }
                    });
            connect(m_setButton, &QPushButton::clicked, this, [this] {
                        setVariableUnsetFlag(false);
                    });
            connect(m_unsetButton, &QPushButton::clicked, this, [this] {
                        setVariableUnsetFlag(true);
                    });
            connect(m_editButton, &QPushButton::clicked, this, [this] {
                        QModelIndex idx = m_configView->currentIndex();
                        if (idx.column() != 1) {
                            idx = idx.sibling(idx.row(), 1);
                        }
                        m_configView->setCurrentIndex(idx);
                        m_configView->edit(idx);
                    });
            connect(addButtonMenu, &QMenu::triggered, this, [this](QAction *action) {
                        ConfigModel::DataItem::Type type =
                            static_cast<ConfigModel::DataItem::Type>(action->data().value<int>());
                        QString value = Tr::tr("<UNSET>");
                        if (type == ConfigModel::DataItem::BOOLEAN) {
                            value = QString::fromLatin1("OFF");
                        }

                        m_configModel->appendConfiguration(Tr::tr("<UNSET>"), value, type, isInitialConfiguration());
                        const TreeItem *item = m_configModel->findNonRootItem([&value, type](TreeItem *item) {
                                                                                  ConfigModel::DataItem dataItem = ConfigModel::dataItemFromIndex(item->index());
                                                                                  return dataItem.key == Tr::tr("<UNSET>") && dataItem.type == type && dataItem.value == value;
                                                                              });
                        QModelIndex idx = m_configModel->indexForItem(item);
                        idx = m_configTextFilterModel->mapFromSource(m_configFilterModel->mapFromSource(idx));
                        m_configView->setFocus();
                        m_configView->scrollTo(idx);
                        m_configView->setCurrentIndex(idx);
                        m_configView->edit(idx);
                    });
            connect(m_batchEditButton, &QAbstractButton::clicked,
                    this, &XMakeBuildSettingsWidget::batchEditConfiguration);

            connect(bs, &XMakeBuildSystem::errorOccurred,
                    this, &XMakeBuildSettingsWidget::setError);
            connect(bs, &XMakeBuildSystem::warningOccurred,
                    this, &XMakeBuildSettingsWidget::setWarning);

            connect(bs, &XMakeBuildSystem::configurationChanged,
                    m_configModel, &ConfigModel::setBatchEditConfiguration);

            updateFromKit();
            connect(m_buildConfig->target(), &Target::kitChanged,
                    this, &XMakeBuildSettingsWidget::updateFromKit);
            connect(bc, &XMakeBuildConfiguration::enabledChanged, this, [this, bc] {
                        if (bc->isEnabled()) {
                            setError(QString());
                        }
                    });
            connect(this, &QObject::destroyed, this, [this] {
                        updateInitialXMakeArguments();
                    });

            connect(m_buildConfig->target()->project(), &Project::aboutToSaveSettings, this, [this] {
                        updateInitialXMakeArguments();
                    });

            connect(&bc->initialXMakeArguments,
                    &Utils::BaseAspect::labelLinkActivated,
                    this,
                    [this](const QString &) {
                        const XMakeTool *tool = XMakeKitAspect::xmakeTool(m_buildConfig->kit());
                        XMakeTool::openXMakeHelpUrl(tool, "%1/manual/xmake.1.html#options");
                    });
            connect(&bc->additionalXMakeOptions,
                    &Utils::BaseAspect::labelLinkActivated, this, [this](const QString &) {
                        const XMakeTool *tool = XMakeKitAspect::xmakeTool(m_buildConfig->kit());
                        XMakeTool::openXMakeHelpUrl(tool, "%1/manual/xmake.1.html#options");
                    });

            if (HostOsInfo::isMacHost()) {
                m_configurationStates->setDrawBase(false);
            }
            m_configurationStates->setExpanding(false);
            m_reconfigureButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

            updateSelection();
            updateConfigurationStateSelection();
        }

        void XMakeBuildSettingsWidget::batchEditConfiguration() {
            auto dialog = new QDialog(this);
            dialog->setWindowTitle(Tr::tr("Edit XMake Configuration"));
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->setModal(true);
            auto layout = new QVBoxLayout(dialog);
            auto editor = new QPlainTextEdit(dialog);

            auto label = new QLabel(dialog);
            label->setText(Tr::tr("Enter one XMake <a href=\"variable\">variable</a> per line.<br/>"
                                  "To set or change a variable, use -D&lt;variable&gt;:&lt;type&gt;=&lt;value&gt;.<br/>"
                                  "&lt;type&gt; can have one of the following values: FILEPATH, PATH, BOOL, INTERNAL, or STRING.<br/>"
                                  "To unset a variable, use -U&lt;variable&gt;.<br/>"));
            connect(label, &QLabel::linkActivated, this, [this](const QString &) {
                        const XMakeTool *tool = XMakeKitAspect::xmakeTool(m_buildConfig->target()->kit());
                        XMakeTool::openXMakeHelpUrl(tool, "%1/manual/xmake-variables.7.html");
                    });
            editor->setMinimumSize(800, 200);

            auto chooser = new Utils::VariableChooser(dialog);
            chooser->addSupportedWidget(editor);
            chooser->addMacroExpanderProvider([this] {
                                                  return m_buildConfig->macroExpander();
                                              });

            auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

            layout->addWidget(editor);
            layout->addWidget(label);
            layout->addWidget(buttons);

            connect(buttons, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
            connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
            connect(dialog, &QDialog::accepted, this, [this, editor] {
                        const auto expander = m_buildConfig->macroExpander();

                        const QStringList lines = editor->toPlainText().split('\n', Qt::SkipEmptyParts);
                        const QStringList expandedLines = Utils::transform(lines,
                                                                           [expander](const QString &s) {
                                                                               return expander->expand(s);
                                                                           });
                        const bool isInitial = isInitialConfiguration();
                        QStringList unknownOptions;
                        XMakeConfig config = XMakeConfig::fromArguments(isInitial ? lines : expandedLines,
                                                                        unknownOptions);
                        for (auto &ci : config) {
                            ci.isInitial = isInitial;
                        }

                        m_configModel->setBatchEditConfiguration(config);
                    });

            editor->setPlainText(
                m_buildConfig->xmakeBuildSystem()->configurationChangesArguments(isInitialConfiguration())
                .join('\n'));

            dialog->show();
        }

        void XMakeBuildSettingsWidget::reconfigureWithInitialParameters() {
            QMessageBox::StandardButton reply = CheckableMessageBox::question(
                Core::ICore::dialogParent(),
                Tr::tr("Re-configure with Initial Parameters"),
                Tr::tr("Clear XMake configuration and configure with initial parameters?"),
                settings().askBeforeReConfigureInitialParams.askAgainCheckableDecider(),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::Yes);

            settings().writeSettings();

            if (reply != QMessageBox::Yes) {
                return;
            }

            m_buildConfig->xmakeBuildSystem()->clearXMakeCache();

            updateInitialXMakeArguments();

            if (ProjectExplorerPlugin::saveModifiedFiles()) {
                m_buildConfig->xmakeBuildSystem()->runXMake();
            }
        }

        void XMakeBuildSettingsWidget::updateInitialXMakeArguments() {
            XMakeConfig initialList = m_buildConfig->initialXMakeArguments.xmakeConfiguration();

            for (const XMakeConfigItem &ci : m_buildConfig->xmakeBuildSystem()->configurationChanges()) {
                if (!ci.isInitial) {
                    continue;
                }
                auto it = std::find_if(initialList.begin(),
                                       initialList.end(),
                                       [ci](const XMakeConfigItem &item) {
                                           return item.key == ci.key;
                                       });
                if (it != initialList.end()) {
                    *it = ci;
                    if (ci.isUnset) {
                        initialList.erase(it);
                    }
                } else if (!ci.key.isEmpty()) {
                    initialList.push_back(ci);
                }
            }

            m_buildConfig->initialXMakeArguments.setXMakeConfiguration(initialList);

            // value() will contain only the unknown arguments (the non -D/-U arguments)
            // As the user would expect to have e.g. "--preset" from "Initial Configuration"
            // to "Current Configuration" as additional parameters
            m_buildConfig->setAdditionalXMakeArguments(ProcessArgs::splitArgs(
                m_buildConfig->initialXMakeArguments(), HostOsInfo::hostOs()));
        }

        void XMakeBuildSettingsWidget::kitXMakeConfiguration() {
            m_buildConfig->kit()->blockNotification();

            auto dialog = new QDialog(this);
            dialog->setWindowTitle(Tr::tr("Kit XMake Configuration"));
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->setModal(true);
            dialog->setSizeGripEnabled(true);
            connect(dialog, &QDialog::finished, this, [this] {
                        m_buildConfig->kit()->unblockNotification();
                    });

            Layouting::Grid grid;
            KitAspect *widget = XMakeKitAspect::createKitAspect(m_buildConfig->kit());
            widget->setParent(dialog);
            widget->addToLayout(grid);
            widget = XMakeGeneratorKitAspect::createKitAspect(m_buildConfig->kit());
            widget->setParent(dialog);
            widget->addToLayout(grid);
            widget = XMakeConfigurationKitAspect::createKitAspect(m_buildConfig->kit());
            widget->setParent(dialog);
            widget->addToLayout(grid);
            grid.attachTo(dialog);

            auto layout = qobject_cast<QGridLayout *>(dialog->layout());

            layout->setColumnStretch(1, 1);

            auto buttons = new QDialogButtonBox(QDialogButtonBox::Close);
            connect(buttons, &QDialogButtonBox::clicked, dialog, &QDialog::close);
            layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Maximum, QSizePolicy::MinimumExpanding),
                            4, 0);
            layout->addWidget(buttons, 5, 0, 1, -1);

            dialog->setMinimumWidth(400);
            dialog->resize(800, 1);
            dialog->show();
        }

        void XMakeBuildSettingsWidget::updateConfigureDetailsWidgetsSummary(
            const QStringList &configurationArguments) {
            ProjectExplorer::ProcessParameters params;

            CommandLine cmd;
            const XMakeTool *tool = XMakeKitAspect::xmakeTool(m_buildConfig->kit());
            cmd.setExecutable(tool ? tool->xmakeExecutable() : "xmake");

            const FilePath buildDirectory = m_buildConfig->buildDirectory();

            params.setCommandLine(cmd);
            m_configureDetailsWidget->setSummaryText(params.summary(Tr::tr("Configure")));
            m_configureDetailsWidget->setState(DetailsWidget::Expanded);
        }

        void XMakeBuildSettingsWidget::setError(const QString &message) {
            m_buildConfig->buildDirectoryAspect()->setProblem(message);
        }

        void XMakeBuildSettingsWidget::setWarning(const QString &message) {
            bool showWarning = !message.isEmpty();
            m_warningMessageLabel->setVisible(showWarning);
            m_warningMessageLabel->setText(message);
        }

        void XMakeBuildSettingsWidget::updateButtonState() {
            const bool isParsing = m_buildConfig->xmakeBuildSystem()->isParsing();

            // Update extra data in buildconfiguration
            const QList<ConfigModel::DataItem> changes = m_configModel->configurationForXMake();

            const XMakeConfig configChanges
                = getQmlDebugCxxFlags() + getSigningFlagsChanges()
                    + Utils::transform(changes, [](const ConfigModel::DataItem &i) {
                                           XMakeConfigItem ni;
                                           ni.key = i.key.toUtf8();
                                           ni.value = i.value.toUtf8();
                                           ni.documentation = i.description.toUtf8();
                                           ni.isAdvanced = i.isAdvanced;
                                           ni.isInitial = i.isInitial;
                                           ni.isUnset = i.isUnset;
                                           ni.inXMakeCache = i.inXMakeCache;
                                           ni.values = i.values;
                                           switch (i.type) {
                                               case ConfigModel::DataItem::BOOLEAN: {
                                                   ni.type = XMakeConfigItem::BOOL;
                                                   break;
                                               }
                                               case ConfigModel::DataItem::FILE: {
                                                   ni.type = XMakeConfigItem::FILEPATH;
                                                   break;
                                               }
                                               case ConfigModel::DataItem::DIRECTORY: {
                                                   ni.type = XMakeConfigItem::PATH;
                                                   break;
                                               }
                                               case ConfigModel::DataItem::STRING: {
                                                   ni.type = XMakeConfigItem::STRING;
                                                   break;
                                               }
                                               case ConfigModel::DataItem::UNKNOWN:
                                               default: {
                                                   ni.type = XMakeConfigItem::UNINITIALIZED;
                                                   break;
                                               }
                                           }
                                           return ni;
                                       });

            const bool isInitial = isInitialConfiguration();
            m_resetButton->setEnabled(m_configModel->hasChanges(isInitial) && !isParsing);

            m_buildConfig->initialXMakeArguments.setVisible(isInitialConfiguration());
            m_buildConfig->additionalXMakeOptions.setVisible(!isInitialConfiguration());

            m_buildConfig->initialXMakeArguments.setEnabled(!isParsing);
            m_buildConfig->additionalXMakeOptions.setEnabled(!isParsing);

            // Update label and text boldness of the reconfigure button
            QFont reconfigureButtonFont = m_reconfigureButton->font();
            if (isParsing) {
                m_reconfigureButton->setText(Tr::tr("Stop XMake"));
                reconfigureButtonFont.setBold(false);
            } else {
                m_reconfigureButton->setEnabled(true);
                if (isInitial) {
                    m_reconfigureButton->setText(Tr::tr("Re-configure with Initial Parameters"));
                } else {
                    m_reconfigureButton->setText(Tr::tr("Run XMake"));
                }
                reconfigureButtonFont.setBold(isInitial ? m_configModel->hasChanges(isInitial)
                                                : !configChanges.isEmpty());
            }
            m_reconfigureButton->setFont(reconfigureButtonFont);

            m_buildConfig->xmakeBuildSystem()->setConfigurationChanges(configChanges);

            // Update the tooltip with the changes
            const QStringList configurationArguments =
                m_buildConfig->xmakeBuildSystem()->configurationChangesArguments(isInitialConfiguration());
            m_reconfigureButton->setToolTip(configurationArguments.join('\n'));
            updateConfigureDetailsWidgetsSummary(configurationArguments);
        }

        void XMakeBuildSettingsWidget::updateAdvancedCheckBox() {
            if (m_showAdvancedCheckBox->isChecked()) {
                m_configFilterModel->setFilterRole(ConfigModel::ItemIsAdvancedRole);
                m_configFilterModel->setFilterRegularExpression("[01]");
            } else {
                m_configFilterModel->setFilterRole(ConfigModel::ItemIsAdvancedRole);
                m_configFilterModel->setFilterFixedString("0");
            }
            updateButtonState();
        }

        void XMakeBuildSettingsWidget::updateFromKit() {
            const Kit *k = m_buildConfig->kit();
            XMakeConfig config = XMakeConfigurationKitAspect::configuration(k);

            config.append(XMakeGeneratorKitAspect::generatorXMakeConfig(k));

            // First the key value parameters
            ConfigModel::KitConfiguration configHash;
            for (const XMakeConfigItem &i : config) {
                configHash.insert(QString::fromUtf8(i.key), i);
            }

            m_configModel->setConfigurationFromKit(configHash);

            // Then the additional parameters
            const QStringList additionalKitXMake = ProcessArgs::splitArgs(
                XMakeConfigurationKitAspect::additionalConfiguration(k), HostOsInfo::hostOs());
            const QStringList additionalInitialXMake =
                ProcessArgs::splitArgs(m_buildConfig->initialXMakeArguments(), HostOsInfo::hostOs());

            QStringList mergedArgumentList;
            std::set_union(additionalInitialXMake.begin(),
                           additionalInitialXMake.end(),
                           additionalKitXMake.begin(),
                           additionalKitXMake.end(),
                           std::back_inserter(mergedArgumentList));
            m_buildConfig->initialXMakeArguments.setValue(ProcessArgs::joinArgs(mergedArgumentList));
        }

        void XMakeBuildSettingsWidget::updateConfigurationStateIndex(int index) {
            if (index == 0) {
                m_configFilterModel->setFilterRole(ConfigModel::ItemIsInitialRole);
                m_configFilterModel->setFilterFixedString("1");
            } else {
                updateAdvancedCheckBox();
            }

            m_showAdvancedCheckBox->setEnabled(index != 0);

            updateButtonState();
        }

        XMakeConfig XMakeBuildSettingsWidget::getQmlDebugCxxFlags() {
            const TriState qmlDebuggingState = m_buildConfig->qmlDebugging();
            if (qmlDebuggingState == TriState::Default) { // don't touch anything
                return {};
            }

            const bool enable = m_buildConfig->qmlDebugging() == TriState::Enabled;

            const XMakeConfig configList = m_buildConfig->xmakeBuildSystem()->configurationFromXMake();
            const QByteArrayList cxxFlagsPrev { "XMAKE_CXX_FLAGS",
                                                "XMAKE_CXX_FLAGS_DEBUG",
                                                "XMAKE_CXX_FLAGS_RELWITHDEBINFO",
                                                "XMAKE_CXX_FLAGS_INIT" };
            const QByteArrayList cxxFlags { "XMAKE_CXX_FLAGS_INIT", "XMAKE_CXX_FLAGS" };
            const QByteArray qmlDebug(QT_QML_DEBUG_PARAM);

            XMakeConfig changedConfig;

            if (enable) {
                const FilePath xmakeCache = m_buildConfig->buildDirectory().pathAppended(
                    Constants::XMAKE_CACHE_TXT);

                // Only modify the XMAKE_CXX_FLAGS variable if the project was previously configured
                // otherwise XMAKE_CXX_FLAGS_INIT will take care of setting the qmlDebug define
                if (xmakeCache.exists()) {
                    for (const XMakeConfigItem &item : configList) {
                        if (!cxxFlags.contains(item.key)) {
                            continue;
                        }

                        XMakeConfigItem it(item);
                        if (!it.value.contains(qmlDebug)) {
                            it.value = it.value.append(' ').append(qmlDebug).trimmed();
                            changedConfig.append(it);
                        }
                    }
                }
            } else {
                // Remove -DQT_QML_DEBUG from all configurations, potentially set by previous Qt Creator versions
                for (const XMakeConfigItem &item : configList) {
                    if (!cxxFlagsPrev.contains(item.key)) {
                        continue;
                    }

                    XMakeConfigItem it(item);
                    int index = it.value.indexOf(qmlDebug);
                    if (index != -1) {
                        it.value.remove(index, qmlDebug.length());
                        it.value = it.value.trimmed();
                        changedConfig.append(it);
                    }
                }
            }
            return changedConfig;
        }

        XMakeConfig XMakeBuildSettingsWidget::getSigningFlagsChanges() {
            const XMakeConfig flags = m_buildConfig->signingFlags();
            if (flags.isEmpty()) {
                return {};
            }

            const XMakeConfig configList = m_buildConfig->xmakeBuildSystem()->configurationFromXMake();
            if (configList.isEmpty()) {
                // we don't have any configuration --> initial configuration takes care of this itself
                return {};
            }
            XMakeConfig changedConfig;
            for (const XMakeConfigItem &signingFlag : flags) {
                const XMakeConfigItem existingFlag = Utils::findOrDefault(configList,
                                                                          Utils::equal(&XMakeConfigItem::key,
                                                                                       signingFlag.key));
                const bool notInConfig = existingFlag.key.isEmpty();
                if (notInConfig != signingFlag.isUnset || existingFlag.value != signingFlag.value) {
                    changedConfig.append(signingFlag);
                }
            }
            return changedConfig;
        }

        void XMakeBuildSettingsWidget::updateSelection() {
            const QModelIndexList selectedIndexes = m_configView->selectionModel()->selectedIndexes();
            unsigned int setableCount = 0;
            unsigned int unsetableCount = 0;
            unsigned int editableCount = 0;

            for (const QModelIndex &index : selectedIndexes) {
                if (index.isValid() && index.flags().testFlag(Qt::ItemIsSelectable)) {
                    const ConfigModel::DataItem di = ConfigModel::dataItemFromIndex(index);
                    if (di.isUnset) {
                        setableCount++;
                    } else {
                        unsetableCount++;
                    }
                }
                if (index.isValid() && index.flags().testFlag(Qt::ItemIsEditable)) {
                    editableCount++;
                }
            }

            m_setButton->setEnabled(setableCount > 0);
            m_unsetButton->setEnabled(unsetableCount > 0);
            m_editButton->setEnabled(editableCount == 1);
        }

        void XMakeBuildSettingsWidget::updateConfigurationStateSelection() {
            const bool hasReplyFile
                = FileApiParser::scanForXMakeReplyFile(m_buildConfig->buildDirectory()).exists();

            const int switchToIndex = hasReplyFile ? 1 : 0;
            if (m_configurationStates->currentIndex() != switchToIndex) {
                m_configurationStates->setCurrentIndex(switchToIndex);
            } else {
                emit m_configurationStates->currentChanged(switchToIndex);
            }
        }

        bool XMakeBuildSettingsWidget::isInitialConfiguration() const {
            return m_configurationStates->currentIndex() == 0;
        }

        void XMakeBuildSettingsWidget::setVariableUnsetFlag(bool unsetFlag) {
            const QModelIndexList selectedIndexes = m_configView->selectionModel()->selectedIndexes();
            bool unsetFlagToggled = false;
            for (const QModelIndex &index : selectedIndexes) {
                if (index.isValid()) {
                    const ConfigModel::DataItem di = ConfigModel::dataItemFromIndex(index);
                    if (di.isUnset != unsetFlag) {
                        m_configModel->toggleUnsetFlag(mapToSource(m_configView, index));
                        unsetFlagToggled = true;
                    }
                }
            }

            if (unsetFlagToggled) {
                updateSelection();
            }
        }

        QAction *XMakeBuildSettingsWidget::createForceAction(int type, const QModelIndex &idx) {
            auto t = static_cast<ConfigModel::DataItem::Type>(type);
            QString typeString;
            switch (type) {
                case ConfigModel::DataItem::BOOLEAN: {
                    typeString = Tr::tr("bool", "display string for xmake type BOOLEAN");
                    break;
                }
                case ConfigModel::DataItem::FILE: {
                    typeString = Tr::tr("file", "display string for xmake type FILE");
                    break;
                }
                case ConfigModel::DataItem::DIRECTORY: {
                    typeString = Tr::tr("directory", "display string for xmake type DIRECTORY");
                    break;
                }
                case ConfigModel::DataItem::STRING: {
                    typeString = Tr::tr("string", "display string for xmake type STRING");
                    break;
                }
                case ConfigModel::DataItem::UNKNOWN: {
                    return nullptr;
                }
            }
            QAction *forceAction = new QAction(Tr::tr("Force to %1").arg(typeString), nullptr);
            forceAction->setEnabled(m_configModel->canForceTo(idx, t));
            connect(forceAction, &QAction::triggered,
                    this, [this, idx, t] {
                        m_configModel->forceTo(idx, t);
                    });
            return forceAction;
        }

        bool XMakeBuildSettingsWidget::eventFilter(QObject *target, QEvent *event) {
            // handle context menu events:
            if (target != m_configView->viewport() || event->type() != QEvent::ContextMenu) {
                return false;
            }

            auto e = static_cast<QContextMenuEvent *>(event);
            const QModelIndex idx = mapToSource(m_configView, m_configView->indexAt(e->pos()));
            if (!idx.isValid()) {
                return false;
            }

            auto menu = new QMenu(this);
            connect(menu, &QMenu::triggered, menu, &QMenu::deleteLater);

            auto help = new QAction(Tr::tr("Help"), this);
            menu->addAction(help);
            connect(help, &QAction::triggered, this, [this, idx] {
                        const XMakeConfigItem item = ConfigModel::dataItemFromIndex(idx).toXMakeConfigItem();

                        const XMakeTool *tool = XMakeKitAspect::xmakeTool(m_buildConfig->target()->kit());
                        const QString linkUrl = "%1/variable/" + QString::fromUtf8(item.key) + ".html";
                        XMakeTool::openXMakeHelpUrl(tool, linkUrl);
                    });

            menu->addSeparator();

            QAction *action = nullptr;
            if ((action = createForceAction(ConfigModel::DataItem::BOOLEAN, idx))) {
                menu->addAction(action);
            }
            if ((action = createForceAction(ConfigModel::DataItem::FILE, idx))) {
                menu->addAction(action);
            }
            if ((action = createForceAction(ConfigModel::DataItem::DIRECTORY, idx))) {
                menu->addAction(action);
            }
            if ((action = createForceAction(ConfigModel::DataItem::STRING, idx))) {
                menu->addAction(action);
            }

            menu->addSeparator();

            auto applyKitOrInitialValue = new QAction(isInitialConfiguration()
                                                  ? Tr::tr("Apply Kit Value")
                                                  : Tr::tr("Apply Initial Configuration Value"),
                                                      this);
            menu->addAction(applyKitOrInitialValue);
            connect(applyKitOrInitialValue, &QAction::triggered, this, [this] {
                        const QModelIndexList selectedIndexes = m_configView->selectionModel()->selectedIndexes();

                        const QModelIndexList validIndexes = Utils::filtered(selectedIndexes, [](const QModelIndex &index) {
                                                                                 return index.isValid() && index.flags().testFlag(Qt::ItemIsSelectable);
                                                                             });

                        for (const QModelIndex &index : validIndexes) {
                            if (isInitialConfiguration()) {
                                m_configModel->applyKitValue(mapToSource(m_configView, index));
                            } else {
                                m_configModel->applyInitialValue(mapToSource(m_configView, index));
                            }
                        }
                    });

            menu->addSeparator();

            auto copy = new QAction(Tr::tr("Copy"), this);
            menu->addAction(copy);
            connect(copy, &QAction::triggered, this, [this] {
                        const QModelIndexList selectedIndexes = m_configView->selectionModel()->selectedIndexes();

                        const QModelIndexList validIndexes = Utils::filtered(selectedIndexes, [](const QModelIndex &index) {
                                                                                 return index.isValid() && index.flags().testFlag(Qt::ItemIsSelectable);
                                                                             });

                        const QStringList variableList
                            = Utils::transform(validIndexes, [this](const QModelIndex &index) {
                                                   return ConfigModel::dataItemFromIndex(index).toXMakeConfigItem().toArgument(
                                                       isInitialConfiguration() ? nullptr : m_buildConfig->macroExpander());
                                               });

                        setClipboardAndSelection(variableList.join('\n'));
                    });

            menu->move(e->globalPos());
            menu->show();

            return true;
        }

        static bool isWebAssembly(const Kit *k) {
            return DeviceTypeKitAspect::deviceTypeId(k) == WebAssembly::Constants::WEBASSEMBLY_DEVICE_TYPE;
        }

        static bool isQnx(const Kit *k) {
            return DeviceTypeKitAspect::deviceTypeId(k) == Qnx::Constants::QNX_QNX_OS_TYPE;
        }

        static bool isWindowsARM64(const Kit *k) {
            Toolchain *toolchain = ToolchainKitAspect::cxxToolchain(k);
            if (!toolchain) {
                return false;
            }
            const Abi targetAbi = toolchain->targetAbi();
            return targetAbi.os() == Abi::WindowsOS && targetAbi.architecture() == Abi::ArmArchitecture
                   && targetAbi.wordWidth() == 64;
        }

        static CommandLine defaultInitialXMakeCommand(const Kit *k, const QString &buildType) {
            // Generator:
            XMakeTool *tool = XMakeKitAspect::xmakeTool(k);
            QTC_ASSERT(tool, return {});

            CommandLine cmd { tool->xmakeExecutable() };
            cmd.addArgs(XMakeGeneratorKitAspect::generatorArguments(k));

            // XMAKE_BUILD_TYPE:
            if (!buildType.isEmpty() && !XMakeGeneratorKitAspect::isMultiConfigGenerator(k)) {
                cmd.addArg("-DXMAKE_BUILD_TYPE:STRING=" + buildType);
            }

            // Package manager auto setup
            if (settings().packageManagerAutoSetup()) {
                cmd.addArg(QString("-DXMAKE_PROJECT_INCLUDE_BEFORE:FILEPATH="
                                   "%{BuildConfig:BuildDirectory:NativeFilePath}/%1/auto-setup.xmake")
                           .arg(Constants::PACKAGE_MANAGER_DIR));
            }

            // Cross-compilation settings:
            if (!XMakeBuildConfiguration::isIos(k)) { // iOS handles this differently
                const QString sysRoot = SysRootKitAspect::sysRoot(k).path();
                if (!sysRoot.isEmpty()) {
                    cmd.addArg("-DXMAKE_SYSROOT:PATH=" + sysRoot);
                    if (Toolchain *tc = ToolchainKitAspect::cxxToolchain(k)) {
                        const QString targetTriple = tc->originalTargetTriple();
                        cmd.addArg("-DXMAKE_C_COMPILER_TARGET:STRING=" + targetTriple);
                        cmd.addArg("-DXMAKE_CXX_COMPILER_TARGET:STRING=" + targetTriple);
                    }
                }
            }

            cmd.addArgs(XMakeConfigurationKitAspect::toArgumentsList(k));
            cmd.addArgs(XMakeConfigurationKitAspect::additionalConfiguration(k), CommandLine::Raw);

            return cmd;
        }

        static void addXMakeConfigurePresetToInitialArguments(QStringList &initialArguments,
                                                              const XMakeProject *project,
                                                              const Kit *k,
                                                              const Utils::Environment &env,
                                                              const Utils::FilePath &buildDirectory) {
            const XMakeConfigItem presetItem = XMakeConfigurationKitAspect::xmakePresetConfigItem(k);
            if (presetItem.isNull()) {
                return;
            }

            // Remove the -DQTC_XMAKE_PRESET argument, which is only used as a kit marker
            const QString presetArgument = presetItem.toArgument();
            const QString presetName = presetItem.expandedValue(k);
            initialArguments.removeIf(
                [presetArgument](const QString &item) {
                    return item == presetArgument;
                });

            PresetsDetails::ConfigurePreset configurePreset
                = Utils::findOrDefault(project->presetsData().configurePresets,
                                       [presetName](const PresetsDetails::ConfigurePreset &preset) {
                                           return preset.name == presetName;
                                       });

            // Add the command line arguments
            if (configurePreset.warnings) {
                if (configurePreset.warnings.value().dev) {
                    bool value = configurePreset.warnings.value().dev.value();
                    initialArguments.append(value ? QString("-Wdev") : QString("-Wno-dev"));
                }
                if (configurePreset.warnings.value().deprecated) {
                    bool value = configurePreset.warnings.value().deprecated.value();
                    initialArguments.append(value ? QString("-Wdeprecated") : QString("-Wno-deprecated"));
                }
                if (configurePreset.warnings.value().uninitialized
                    && configurePreset.warnings.value().uninitialized.value()) {
                    initialArguments.append("--warn-uninitialized");
                }
                if (configurePreset.warnings.value().unusedCli
                    && !configurePreset.warnings.value().unusedCli.value()) {
                    initialArguments.append(" --no-warn-unused-cli");
                }
                if (configurePreset.warnings.value().systemVars
                    && configurePreset.warnings.value().systemVars.value()) {
                    initialArguments.append("--check-system-vars");
                }
            }

            if (configurePreset.errors) {
                if (configurePreset.errors.value().dev) {
                    bool value = configurePreset.errors.value().dev.value();
                    initialArguments.append(value ? QString("-Werror=dev") : QString("-Wno-error=dev"));
                }
                if (configurePreset.errors.value().deprecated) {
                    bool value = configurePreset.errors.value().deprecated.value();
                    initialArguments.append(value ? QString("-Werror=deprecated")
                                          : QString("-Wno-error=deprecated"));
                }
            }

            if (configurePreset.debug) {
                if (configurePreset.debug.value().find && configurePreset.debug.value().find.value()) {
                    initialArguments.append("--debug-find");
                }
                if (configurePreset.debug.value().tryCompile
                    && configurePreset.debug.value().tryCompile.value()) {
                    initialArguments.append("--debug-trycompile");
                }
                if (configurePreset.debug.value().output && configurePreset.debug.value().output.value()) {
                    initialArguments.append("--debug-output");
                }
            }

            XMakePresets::Macros::updateToolchainFile(configurePreset,
                                                      env,
                                                      project->projectDirectory(),
                                                      buildDirectory);
            XMakePresets::Macros::updateInstallDir(configurePreset, env, project->projectDirectory());

            // Merge the presets cache variables
            XMakeConfig cache;
            if (configurePreset.cacheVariables) {
                cache = configurePreset.cacheVariables.value();
            }

            for (const XMakeConfigItem &presetItemRaw : cache) {
                // Expand the XMakePresets Macros
                XMakeConfigItem presetItem(presetItemRaw);

                QString presetItemValue = QString::fromUtf8(presetItem.value);
                XMakePresets::Macros::expand(configurePreset, env, project->projectDirectory(), presetItemValue);
                presetItem.value = presetItemValue.toUtf8();

                const QString presetItemArg = presetItem.toArgument();
                const QString presetItemArgNoType = presetItemArg.left(presetItemArg.indexOf(":"));

                static QSet<QByteArray> defaultKitMacroValues { "XMAKE_C_COMPILER",
                                                                "XMAKE_CXX_COMPILER",
                                                                "QT_QMAKE_EXECUTABLE",
                                                                "QT_HOST_PATH",
                                                                "XMAKE_PROJECT_INCLUDE_BEFORE" };

                auto it = std::find_if(initialArguments.begin(),
                                       initialArguments.end(),
                                       [presetItemArgNoType](const QString &arg) {
                                           return arg.startsWith(presetItemArgNoType);
                                       });

                if (it != initialArguments.end()) {
                    QString &arg = *it;
                    XMakeConfigItem argItem = XMakeConfigItem::fromString(arg.mid(2)); // skip -D

                    // These values have Qt Creator macro names pointing to the Kit values
                    // which are preset expanded values used when the Kit was created
                    if (defaultKitMacroValues.contains(argItem.key) && argItem.value.startsWith("%{")) {
                        continue;
                    }

                    // For multi value path variables append the non Qt path
                    if (argItem.key == "XMAKE_PREFIX_PATH" || argItem.key == "XMAKE_FIND_ROOT_PATH") {
                        QStringList presetValueList = presetItem.expandedValue(k).split(";");

                        // Remove the expanded Qt path from the presets values
                        QString argItemExpandedValue = argItem.expandedValue(k);
                        presetValueList.removeIf([argItemExpandedValue](const QString &presetPath) {
                                                     QStringList argItemPaths = argItemExpandedValue.split(";");
                                                     for (const QString &argPath : argItemPaths) {
                                                         const FilePath argFilePath = FilePath::fromString(argPath);
                                                         const FilePath presetFilePath = FilePath::fromUserInput(presetPath);

                                                         if (argFilePath == presetFilePath) {
                                                             return true;
                                                         }
                                                     }
                                                     return false;
                                                 });

                        // Add the presets values to the final argument
                        for (const QString &presetPath : presetValueList) {
                            argItem.value.append(";");
                            argItem.value.append(presetPath.toUtf8());
                        }

                        arg = argItem.toArgument();
                    } else if (argItem.key == "XMAKE_TOOLCHAIN_FILE") {
                        const FilePath argFilePath = FilePath::fromString(argItem.expandedValue(k));
                        const FilePath presetFilePath = FilePath::fromUserInput(
                            QString::fromUtf8(presetItem.value));

                        if (argFilePath != presetFilePath) {
                            arg = presetItem.toArgument();
                        }
                    } else if (argItem.expandedValue(k) != QString::fromUtf8(presetItem.value)) {
                        arg = presetItem.toArgument();
                    }
                } else {
                    initialArguments.append(presetItem.toArgument());
                }
            }
        }

        static Utils::EnvironmentItems getEnvironmentItemsFromXMakeConfigurePreset(
            const XMakeProject *project, const Kit *k) {
            Utils::EnvironmentItems envItems;

            const XMakeConfigItem presetItem = XMakeConfigurationKitAspect::xmakePresetConfigItem(k);
            if (presetItem.isNull()) {
                return envItems;
            }

            const QString presetName = presetItem.expandedValue(k);

            PresetsDetails::ConfigurePreset configurePreset
                = Utils::findOrDefault(project->presetsData().configurePresets,
                                       [presetName](const PresetsDetails::ConfigurePreset &preset) {
                                           return preset.name == presetName;
                                       });

            XMakePresets::Macros::expand(configurePreset, envItems, project->projectDirectory());

            return envItems;
        }

        static Utils::EnvironmentItems getEnvironmentItemsFromXMakeBuildPreset(
            const XMakeProject *project, const Kit *k, const QString &buildPresetName) {
            Utils::EnvironmentItems envItems;

            const XMakeConfigItem presetItem = XMakeConfigurationKitAspect::xmakePresetConfigItem(k);
            if (presetItem.isNull()) {
                return envItems;
            }

            PresetsDetails::BuildPreset buildPreset
                = Utils::findOrDefault(project->presetsData().buildPresets,
                                       [buildPresetName](const PresetsDetails::BuildPreset &preset) {
                                           return preset.name == buildPresetName;
                                       });

            XMakePresets::Macros::expand(buildPreset, envItems, project->projectDirectory());

            return envItems;
        }
    } // namespace Internal

// -----------------------------------------------------------------------------
// XMakeBuildConfiguration:
// -----------------------------------------------------------------------------

    XMakeBuildConfiguration::XMakeBuildConfiguration(Target *target, Id id)
        : BuildConfiguration(target, id) {
        m_buildSystem = new XMakeBuildSystem(this);

        buildDirectoryAspect()->setValueAcceptor(
            [](const QString &oldDir, const QString &newDir) -> std::optional<QString> {
                if (oldDir.isEmpty()) {
                    return newDir;
                }

                const FilePath oldDirXMakeCache = FilePath::fromUserInput(oldDir).pathAppended(
                    Constants::XMAKE_CACHE_TXT);
                const FilePath newDirXMakeCache = FilePath::fromUserInput(newDir).pathAppended(
                    Constants::XMAKE_CACHE_TXT);

                if (oldDirXMakeCache.exists() && !newDirXMakeCache.exists()) {
                    if (QMessageBox::information(
                        Core::ICore::dialogParent(),
                        Tr::tr("Changing Build Directory"),
                        Tr::tr("Change the build directory to \"%1\" and start with a "
                               "basic XMake configuration?")
                        .arg(newDir),
                        QMessageBox::Ok,
                        QMessageBox::Cancel)
                        == QMessageBox::Ok) {
                        return newDir;
                    }
                    return std::nullopt;
                }
                return newDir;
            });

        // Will not be displayed, only persisted
        sourceDirectory.setSettingsKey("XMake.Source.Directory");

        buildTypeAspect.setSettingsKey(XMAKE_BUILD_TYPE);
        buildTypeAspect.setLabelText(Tr::tr("Build type:"));
        buildTypeAspect.setDisplayStyle(StringAspect::LineEditDisplay);
        buildTypeAspect.setDefaultValue("Unknown");

        initialXMakeArguments.setMacroExpanderProvider([this] {
                                                           return macroExpander();
                                                       });

        additionalXMakeOptions.setSettingsKey("XMake.Additional.Options");
        additionalXMakeOptions.setLabelText(Tr::tr("Additional XMake <a href=\"options\">options</a>:"));
        additionalXMakeOptions.setDisplayStyle(StringAspect::LineEditDisplay);
        additionalXMakeOptions.setMacroExpanderProvider([this] {
                                                            return macroExpander();
                                                        });

        macroExpander()->registerVariable(DEVELOPMENT_TEAM_FLAG,
                                          Tr::tr("The XMake flag for the development team"),
                                          [this] {
                                              const XMakeConfig flags = signingFlags();
                                              if (!flags.isEmpty()) {
                                                  return flags.first().toArgument();
                                              }
                                              return QString();
                                          });
        macroExpander()->registerVariable(PROVISIONING_PROFILE_FLAG,
                                          Tr::tr("The XMake flag for the provisioning profile"),
                                          [this] {
                                              const XMakeConfig flags = signingFlags();
                                              if (flags.size() > 1 && !flags.at(1).isUnset) {
                                                  return flags.at(1).toArgument();
                                              }
                                              return QString();
                                          });

        macroExpander()->registerVariable(XMAKE_OSX_ARCHITECTURES_FLAG,
                                          Tr::tr("The XMake flag for the architecture on macOS"),
                                          [target] {
                                              if (HostOsInfo::isRunningUnderRosetta()) {
                                                  if (auto *qt = QtSupport::QtKitAspect::qtVersion(target->kit())) {
                                                      const Abis abis = qt->qtAbis();
                                                      for (const Abi &abi : abis) {
                                                          if (abi.architecture() == Abi::ArmArchitecture) {
                                                              return QLatin1String("-DXMAKE_OSX_ARCHITECTURES=arm64");
                                                          }
                                                      }
                                                  }
                                              }
                                              return QLatin1String();
                                          });
        macroExpander()->registerVariable(QT_QML_DEBUG_FLAG,
                                          Tr::tr("The XMake flag for QML debugging, if enabled"),
                                          [this] {
                                              if (aspect<QtSupport::QmlDebuggingAspect>()->value()
                                                  == TriState::Enabled) {
                                                  return QLatin1String(QT_QML_DEBUG_PARAM);
                                              }
                                              return QLatin1String();
                                          });

        qmlDebugging.setBuildConfiguration(this);

        setInitialBuildAndCleanSteps(target);

        setInitializer([this, target](const BuildInfo &info) {
                           const Kit *k = target->kit();
                           const QtSupport::QtVersion *qt = QtSupport::QtKitAspect::qtVersion(k);
                           const Store extraInfoMap = storeFromVariant(info.extraInfo);
                           const QString buildType = extraInfoMap.contains(XMAKE_BUILD_TYPE)
                                      ? extraInfoMap.value(XMAKE_BUILD_TYPE).toString()
                                      : info.typeName;

                           CommandLine cmd = defaultInitialXMakeCommand(k, buildType);
                           m_buildSystem->setIsMultiConfig(XMakeGeneratorKitAspect::isMultiConfigGenerator(k));

                           // Android magic:
                           if (DeviceTypeKitAspect::deviceTypeId(k) == Android::Constants::ANDROID_DEVICE_TYPE) {
                               auto addUniqueKeyToCmd = [&cmd](const QString &prefix, const QString &value) -> bool {
                                   const bool isUnique =
                                       !Utils::contains(cmd.splitArguments(), [&prefix](const QString &arg) {
                                                            return arg.startsWith(prefix);
                                                        });
                                   if (isUnique) {
                                       cmd.addArg(prefix + value);
                                   }
                                   return isUnique;
                               };
                               buildSteps()->appendStep(Android::Constants::ANDROID_BUILD_APK_ID);
                               const auto bs = buildSteps()->steps().constLast();
                               addUniqueKeyToCmd("-DANDROID_PLATFORM:STRING=",
                                                 bs->data(Android::Constants::AndroidNdkPlatform).toString());
                               auto ndkLocation = bs->data(Android::Constants::NdkLocation).value<FilePath>();
                               cmd.addArg("-DANDROID_NDK:PATH=" + ndkLocation.path());

                               cmd.addArg("-DXMAKE_TOOLCHAIN_FILE:FILEPATH="
                                          + ndkLocation.pathAppended("build/xmake/android.toolchain.xmake").path());
                               cmd.addArg("-DANDROID_USE_LEGACY_TOOLCHAIN_FILE:BOOL=OFF");

                               auto androidAbis = bs->data(Android::Constants::AndroidMkSpecAbis).toStringList();
                               QString preferredAbi;
                               if (androidAbis.contains(ProjectExplorer::Constants::ANDROID_ABI_ARMEABI_V7A)) {
                                   preferredAbi = ProjectExplorer::Constants::ANDROID_ABI_ARMEABI_V7A;
                               } else if (androidAbis.isEmpty()
                                          || androidAbis.contains(ProjectExplorer::Constants::ANDROID_ABI_ARM64_V8A)) {
                                   preferredAbi = ProjectExplorer::Constants::ANDROID_ABI_ARM64_V8A;
                               } else {
                                   preferredAbi = androidAbis.first();
                               }
                               cmd.addArg("-DANDROID_ABI:STRING=" + preferredAbi);
                               cmd.addArg("-DANDROID_STL:STRING=c++_shared");
                               cmd.addArg("-DXMAKE_FIND_ROOT_PATH:PATH=%{Qt:QT_INSTALL_PREFIX}");

                               auto sdkLocation = bs->data(Android::Constants::SdkLocation).value<FilePath>();

                               if (qt && qt->qtVersion() >= QVersionNumber(6, 0, 0)) {
                                   // Don't build apk under ALL target because Qt Creator will handle it
                                   if (qt->qtVersion() >= QVersionNumber(6, 1, 0)) {
                                       cmd.addArg("-DQT_NO_GLOBAL_APK_TARGET_PART_OF_ALL:BOOL=ON");
                                       if (qt->qtVersion() >= QVersionNumber(6, 8, 0)) {
                                           cmd.addArg("-DQT_USE_TARGET_ANDROID_BUILD_DIR:BOOL=ON");
                                       }
                                   }

                                   cmd.addArg("-DQT_HOST_PATH:PATH=%{Qt:QT_HOST_PREFIX}");
                                   cmd.addArg("-DANDROID_SDK_ROOT:PATH=" + sdkLocation.path());
                               } else {
                                   cmd.addArg("-DANDROID_SDK:PATH=" + sdkLocation.path());
                               }
                           }

                           const IDevice::ConstPtr device = DeviceKitAspect::device(k);
                           if (XMakeBuildConfiguration::isIos(k)) {
                               if (qt && qt->qtVersion().majorVersion() >= 6) {
                                   // TODO it would be better if we could set
                                   // XMAKE_SYSTEM_NAME=iOS and XMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH=YES
                                   // and build with "xmake --build . -- -arch <arch>" instead of setting the architecture
                                   // and sysroot in the XMake configuration, but that currently doesn't work with Qt/XMake
                                   // https://gitlab.kitware.com/xmake/xmake/-/issues/21276
                                   const Id deviceType = DeviceTypeKitAspect::deviceTypeId(k);
                                   // TODO the architectures are probably not correct with Apple Silicon in the mix...
                                   const QString architecture = deviceType == Ios::Constants::IOS_DEVICE_TYPE
                                                 ? QLatin1String("arm64")
                                                 : QLatin1String("x86_64");
                                   const QString sysroot = deviceType == Ios::Constants::IOS_DEVICE_TYPE
                                            ? QLatin1String("iphoneos")
                                            : QLatin1String("iphonesimulator");
                                   cmd.addArg(XMAKE_QT6_TOOLCHAIN_FILE_ARG);
                                   cmd.addArg("-DXMAKE_OSX_ARCHITECTURES:STRING=" + architecture);
                                   cmd.addArg("-DXMAKE_OSX_SYSROOT:STRING=" + sysroot);
                                   cmd.addArg("%{" + QLatin1String(DEVELOPMENT_TEAM_FLAG) + "}");
                                   cmd.addArg("%{" + QLatin1String(PROVISIONING_PROFILE_FLAG) + "}");
                               }
                           } else if (device && device->osType() == Utils::OsTypeMac) {
                               cmd.addArg("%{" + QLatin1String(XMAKE_OSX_ARCHITECTURES_FLAG) + "}");
                           }

                           if (isWebAssembly(k) || isQnx(k) || isWindowsARM64(k)) {
                               if (qt && qt->qtVersion().majorVersion() >= 6) {
                                   cmd.addArg(XMAKE_QT6_TOOLCHAIN_FILE_ARG);
                               }
                           }

                           if (info.buildDirectory.isEmpty()) {
                               setBuildDirectory(shadowBuildDirectory(target->project()->projectFilePath(),
                                                                      k,
                                                                      info.typeName,
                                                                      info.buildType));
                           }

                           if (extraInfoMap.contains(Constants::XMAKE_HOME_DIR)) {
                               sourceDirectory.setValue(FilePath::fromVariant(extraInfoMap.value(Constants::XMAKE_HOME_DIR)));
                           }

                           qmlDebugging.setValue(extraInfoMap.contains(Constants::QML_DEBUG_SETTING)
                                  ? TriState::fromVariant(extraInfoMap.value(Constants::QML_DEBUG_SETTING))
                                  : TriState::Default);

                           if (qt && qt->isQmlDebuggingSupported()) {
                               cmd.addArg("-DXMAKE_CXX_FLAGS_INIT:STRING=%{" + QLatin1String(QT_QML_DEBUG_FLAG) + "}");
                           }

                           XMakeProject *xmakeProject = static_cast<XMakeProject *>(target->project());
                           configureEnv.setUserEnvironmentChanges(
                               getEnvironmentItemsFromXMakeConfigurePreset(xmakeProject, k));

                           QStringList initialXMakeArguments = cmd.splitArguments();
                           addXMakeConfigurePresetToInitialArguments(initialXMakeArguments,
                                                                     xmakeProject,
                                                                     k,
                                                                     configureEnvironment(),
                                                                     info.buildDirectory);
                           setInitialXMakeArguments(initialXMakeArguments);
                           setXMakeBuildType(buildType);

                           setBuildPresetToBuildSteps(target);
                       });
    }

    XMakeBuildConfiguration::~XMakeBuildConfiguration() {
        delete m_buildSystem;
    }

    FilePath XMakeBuildConfiguration::shadowBuildDirectory(const FilePath &projectFilePath,
                                                           const Kit *k,
                                                           const QString &bcName,
                                                           BuildConfiguration::BuildType buildType) {
        if (projectFilePath.isEmpty()) {
            return {};
        }

        const QString projectName = projectFilePath.parentDir().fileName();
        const FilePath projectDir = Project::projectDirectory(projectFilePath);
        FilePath buildPath = buildDirectoryFromTemplate(projectDir, projectFilePath, projectName, k,
                                                        bcName, buildType, "xmake");

        if (XMakeGeneratorKitAspect::isMultiConfigGenerator(k)) {
            const QString path = buildPath.path();
            buildPath = buildPath.withNewPath(path.left(path.lastIndexOf(QString("-%1").arg(bcName))));
        }

        return buildPath;
    }

    bool XMakeBuildConfiguration::isIos(const Kit *k) {
        const Id deviceType = DeviceTypeKitAspect::deviceTypeId(k);
        return deviceType == Ios::Constants::IOS_DEVICE_TYPE
               || deviceType == Ios::Constants::IOS_SIMULATOR_TYPE;
    }

    bool XMakeBuildConfiguration::hasQmlDebugging(const XMakeConfig &config) {
        // Determine QML debugging flags. This must match what we do in
        // XMakeBuildSettingsWidget::getQmlDebugCxxFlags()
        // such that in doubt we leave the QML Debugging setting at "Leave at default"
        const QString cxxFlagsInit = config.stringValueOf("XMAKE_CXX_FLAGS_INIT");
        const QString cxxFlags = config.stringValueOf("XMAKE_CXX_FLAGS");
        return cxxFlagsInit.contains(QT_QML_DEBUG_PARAM) && cxxFlags.contains(QT_QML_DEBUG_PARAM);
    }

    void XMakeBuildConfiguration::buildTarget(const QString &buildTarget) {
        auto cmBs = qobject_cast<XMakeBuildStep *>(findOrDefault(
            buildSteps()->steps(),
            [](const BuildStep *bs) {
                return bs->id() == Constants::XMAKE_BUILD_STEP_ID;
            }));

        QStringList originalBuildTargets;
        if (cmBs) {
            originalBuildTargets = cmBs->buildTargets();
            cmBs->setBuildTargets({ buildTarget });
        }

        BuildManager::buildList(buildSteps());

        if (cmBs) {
            cmBs->setBuildTargets(originalBuildTargets);
        }
    }

    XMakeConfig XMakeBuildSystem::configurationFromXMake() const {
        return m_configurationFromXMake;
    }

    XMakeConfig XMakeBuildSystem::configurationChanges() const {
        return m_configurationChanges;
    }

    QStringList XMakeBuildSystem::configurationChangesArguments(bool initialParameters) const {
        const QList<XMakeConfigItem> filteredInitials
            = Utils::filtered(m_configurationChanges, [initialParameters](const XMakeConfigItem &ci) {
                                  return initialParameters ? ci.isInitial : !ci.isInitial;
                              });
        return Utils::transform(filteredInitials, &XMakeConfigItem::toArgument);
    }

    XMakeConfig XMakeBuildSystem::initialXMakeConfiguration() const {
        return xmakeBuildConfiguration()->initialXMakeArguments.xmakeConfiguration();
    }

    void XMakeBuildSystem::setConfigurationFromXMake(const XMakeConfig &config) {
        m_configurationFromXMake = config;
    }

    void XMakeBuildSystem::setConfigurationChanges(const XMakeConfig &config) {
        qCDebug(xmakeBuildConfigurationLog)
            << "Configuration changes before:" << configurationChangesArguments();

        m_configurationChanges = config;

        qCDebug(xmakeBuildConfigurationLog)
            << "Configuration changes after:" << configurationChangesArguments();
    }

// FIXME: Run clean steps when a setting starting with "ANDROID_BUILD_ABI_" is changed.
// FIXME: Warn when kit settings are overridden by a project.

    void XMakeBuildSystem::clearError(ForceEnabledChanged fec) {
        if (!m_error.isEmpty()) {
            m_error.clear();
            fec = ForceEnabledChanged::True;
        }
        if (fec == ForceEnabledChanged::True) {
            qCDebug(xmakeBuildConfigurationLog) << "Emitting enabledChanged signal";
            emit buildConfiguration()->enabledChanged();
        }
    }

    void XMakeBuildConfiguration::setInitialXMakeArguments(const QStringList &args) {
        QStringList additionalArguments;
        initialXMakeArguments.setAllValues(args.join('\n'), additionalArguments);

        // Set the unknown additional arguments also for the "Current Configuration"
        setAdditionalXMakeArguments(additionalArguments);
    }

    QStringList XMakeBuildConfiguration::additionalXMakeArguments() const {
        return ProcessArgs::splitArgs(additionalXMakeOptions(), HostOsInfo::hostOs());
    }

    void XMakeBuildConfiguration::setAdditionalXMakeArguments(const QStringList &args) {
        const QStringList expandedAdditionalArguments = Utils::transform(args, [this](const QString &s) {
                                                                             return macroExpander()->expand(s);
                                                                         });
        const QStringList nonEmptyAdditionalArguments = Utils::filtered(expandedAdditionalArguments,
                                                                        [](const QString &s) {
                                                                            return !s.isEmpty();
                                                                        });
        additionalXMakeOptions.setValue(ProcessArgs::joinArgs(nonEmptyAdditionalArguments));
    }

    void XMakeBuildConfiguration::filterConfigArgumentsFromAdditionalXMakeArguments() {
        // On iOS the %{Ios:DevelopmentTeam:Flag} evalues to something like
        // -DXMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM:STRING=MAGICSTRING
        // which is already part of the XMake variables and should not be also
        // in the addtional XMake options
        const QStringList arguments = ProcessArgs::splitArgs(additionalXMakeOptions(),
                                                             HostOsInfo::hostOs());
        QStringList unknownOptions;
        const XMakeConfig config = XMakeConfig::fromArguments(arguments, unknownOptions);

        additionalXMakeOptions.setValue(ProcessArgs::joinArgs(unknownOptions));
    }

    void XMakeBuildSystem::setError(const QString &message) {
        qCDebug(xmakeBuildConfigurationLog) << "Setting error to" << message;
        QTC_ASSERT(!message.isEmpty(), return );

        const QString oldMessage = m_error;
        if (m_error != message) {
            m_error = message;
        }
        if (oldMessage.isEmpty() != !message.isEmpty()) {
            qCDebug(xmakeBuildConfigurationLog) << "Emitting enabledChanged signal";
            emit buildConfiguration()->enabledChanged();
        }
        TaskHub::addTask(BuildSystemTask(Task::TaskType::Error, message));
        emit errorOccurred(m_error);
    }

    void XMakeBuildSystem::setWarning(const QString &message) {
        if (m_warning == message) {
            return;
        }
        m_warning = message;
        TaskHub::addTask(BuildSystemTask(Task::TaskType::Warning, message));
        emit warningOccurred(m_warning);
    }

    QString XMakeBuildSystem::error() const {
        return m_error;
    }

    QString XMakeBuildSystem::warning() const {
        return m_warning;
    }

    NamedWidget *XMakeBuildConfiguration::createConfigWidget() {
        return new XMakeBuildSettingsWidget(this);
    }

    XMakeConfig XMakeBuildConfiguration::signingFlags() const {
        return {};
    }

    void XMakeBuildConfiguration::setInitialBuildAndCleanSteps(const Target *target) {
        const XMakeConfigItem presetItem = XMakeConfigurationKitAspect::xmakePresetConfigItem(
            target->kit());

        int buildSteps = 1;
        if (!presetItem.isNull()) {
            const QString presetName = presetItem.expandedValue(target->kit());
            const XMakeProject *project = static_cast<const XMakeProject *>(target->project());

            const auto buildPresets = project->presetsData().buildPresets;
            const int count
                = std::count_if(buildPresets.begin(),
                                buildPresets.end(),
                                [presetName, project](const PresetsDetails::BuildPreset &preset) {
                                    bool enabled = true;
                                    if (preset.condition) {
                                        enabled = XMakePresets::Macros::evaluatePresetCondition(
                                            preset, project->projectDirectory());
                                    }

                                    return preset.configurePreset == presetName
                                           && !preset.hidden.value() && enabled;
                                });
            if (count != 0) {
                buildSteps = count;
            }
        }

        for (int i = 0; i < buildSteps; ++i) {
            appendInitialBuildStep(Constants::XMAKE_BUILD_STEP_ID);
        }

        appendInitialCleanStep(Constants::XMAKE_BUILD_STEP_ID);
    }

    void XMakeBuildConfiguration::setBuildPresetToBuildSteps(const ProjectExplorer::Target *target) {
        const XMakeConfigItem presetItem = XMakeConfigurationKitAspect::xmakePresetConfigItem(
            target->kit());

        if (presetItem.isNull()) {
            return;
        }

        const QString presetName = presetItem.expandedValue(target->kit());
        const XMakeProject *project = static_cast<const XMakeProject *>(target->project());

        const auto allBuildPresets = project->presetsData().buildPresets;
        const auto buildPresets = Utils::filtered(
            allBuildPresets, [presetName, project](const PresetsDetails::BuildPreset &preset) {
                bool enabled = true;
                if (preset.condition) {
                    enabled = XMakePresets::Macros::evaluatePresetCondition(preset,
                                                                            project->projectDirectory());
                }

                return preset.configurePreset == presetName && !preset.hidden.value() && enabled;
            });

        const QList<BuildStep *> buildStepList
            = Utils::filtered(buildSteps()->steps(), [](const BuildStep *bs) {
                                  return bs->id() == Constants::XMAKE_BUILD_STEP_ID;
                              });

        if (buildPresets.size() != buildStepList.size()) {
            return;
        }

        for (qsizetype i = 0; i < buildStepList.size(); ++i) {
            XMakeBuildStep *cbs = qobject_cast<XMakeBuildStep *>(buildStepList[i]);
            cbs->setBuildPreset(buildPresets[i].name);
            cbs->setUserEnvironmentChanges(
                getEnvironmentItemsFromXMakeBuildPreset(project, target->kit(), buildPresets[i].name));

            if (buildPresets[i].targets) {
                QString targets = buildPresets[i].targets.value().join(" ");

                XMakePresets::Macros::expand(buildPresets[i],
                                             cbs->environment(),
                                             project->projectDirectory(),
                                             targets);

                cbs->setBuildTargets(targets.split(" "));
            }

            QStringList xmakeArguments;
            if (buildPresets[i].jobs) {
                xmakeArguments.append(QString("-j %1").arg(buildPresets[i].jobs.value()));
            }
            if (buildPresets[i].verbose && buildPresets[i].verbose.value()) {
                xmakeArguments.append("--verbose");
            }
            if (buildPresets[i].cleanFirst && buildPresets[i].cleanFirst.value()) {
                xmakeArguments.append("--clean-first");
            }
            if (!xmakeArguments.isEmpty()) {
                cbs->setXMakeArguments(xmakeArguments);
            }

            if (buildPresets[i].nativeToolOptions) {
                QString nativeToolOptions = buildPresets[i].nativeToolOptions.value().join(" ");

                XMakePresets::Macros::expand(buildPresets[i],
                                             cbs->environment(),
                                             project->projectDirectory(),
                                             nativeToolOptions);

                cbs->setToolArguments(nativeToolOptions.split(" "));
            }

            if (buildPresets[i].configuration) {
                cbs->setConfiguration(buildPresets[i].configuration.value());
            }

            // Leave only the first build step enabled
            if (i > 0) {
                cbs->setEnabled(false);
            }
        }
    }

/*!
   \class XMakeBuildConfigurationFactory
 */

    XMakeBuildConfigurationFactory::XMakeBuildConfigurationFactory() {
        registerBuildConfiguration<XMakeBuildConfiguration>(Constants::XMAKE_BUILDCONFIGURATION_ID);

        setSupportedProjectType(XMakeProjectManager::Constants::XMAKE_PROJECT_ID);
        setSupportedProjectMimeTypeName(Utils::Constants::CMAKE_PROJECT_MIMETYPE);

        setBuildGenerator([](const Kit *k, const FilePath &projectPath, bool forSetup) {
                              QList<BuildInfo> result;

                              FilePath path = forSetup ? Project::projectDirectory(projectPath) : projectPath;

                              // Skip the default shadow build directories for build types if we have presets
                              const XMakeConfigItem presetItem = XMakeConfigurationKitAspect::xmakePresetConfigItem(k);
                              if (!presetItem.isNull()) {
                                  return result;
                              }

                              for (int type = BuildTypeXMake; type != BuildTypeLast; ++type) {
                                  BuildInfo info = createBuildInfo(BuildType(type));
                                  if (forSetup) {
                                      info.buildDirectory = XMakeBuildConfiguration::shadowBuildDirectory(projectPath,
                                                                                                          k,
                                                                                                          info.typeName,
                                                                                                          info.buildType);
                                  }
                                  result << info;
                              }
                              return result;
                          });
    }

    XMakeBuildConfigurationFactory::BuildType XMakeBuildConfigurationFactory::buildTypeFromByteArray(
        const QByteArray &in) {
        const QByteArray bt = in.toLower();
        if (bt == "xmake") {
            return BuildTypeXMake;
        }
        return BuildTypeNone;
    }

    BuildConfiguration::BuildType XMakeBuildConfigurationFactory::xmakeBuildTypeToBuildType(
        const XMakeBuildConfigurationFactory::BuildType &in) {
        return createBuildInfo(in).buildType;
    }

    BuildInfo XMakeBuildConfigurationFactory::createBuildInfo(BuildType buildType) {
        BuildInfo info;

        switch (buildType) {
            case BuildTypeNone: {
                info.typeName = "Build";
                info.displayName = ::ProjectExplorer::Tr::tr("Build");
                info.buildType = BuildConfiguration::Unknown;
                break;
            }
            case BuildTypeXMake: {
                info.typeName = "XMake";
                info.displayName = ::ProjectExplorer::Tr::tr("XMake");
                info.buildType = BuildConfiguration::XMake;
                Store extraInfo;
                // enable QML debugging by default
                extraInfo.insert(Constants::QML_DEBUG_SETTING, TriState::Enabled.toVariant());
                info.extraInfo = variantFromStore(extraInfo);
                break;
            }
            default: {
                QTC_CHECK(false);
                break;
            }
        }

        return info;
    }

    BuildConfiguration::BuildType XMakeBuildConfiguration::buildType() const {
        return m_buildSystem->buildType();
    }

    BuildConfiguration::BuildType XMakeBuildSystem::buildType() const {
        QByteArray xmakeBuildTypeName = m_configurationFromXMake.valueOf("XMAKE_BUILD_TYPE");
        if (xmakeBuildTypeName.isEmpty()) {
            QByteArray xmakeCfgTypes = m_configurationFromXMake.valueOf("XMAKE_CONFIGURATION_TYPES");
            if (!xmakeCfgTypes.isEmpty()) {
                xmakeBuildTypeName = xmakeBuildType().toUtf8();
            }
        }
        // Cover all common XMake build types
        const XMakeBuildConfigurationFactory::BuildType xmakeBuildType
            = XMakeBuildConfigurationFactory::buildTypeFromByteArray(xmakeBuildTypeName);
        return XMakeBuildConfigurationFactory::xmakeBuildTypeToBuildType(xmakeBuildType);
    }

    BuildSystem *XMakeBuildConfiguration::buildSystem() const {
        return m_buildSystem;
    }

    XMakeBuildSystem *XMakeBuildConfiguration::xmakeBuildSystem() const {
        return m_buildSystem;
    }

    void XMakeBuildConfiguration::addToEnvironment(Utils::Environment &env) const {
        const XMakeTool *tool = XMakeKitAspect::xmakeTool(kit());
        // The hack further down is only relevant for desktop
        if (tool && tool->xmakeExecutable().needsDevice()) {
            return;
        }

        const FilePath ninja = settings().ninjaPath();
        if (!ninja.isEmpty()) {
            env.appendOrSetPath(ninja.isFile() ? ninja.parentDir() : ninja);
        }
    }

    Environment XMakeBuildConfiguration::configureEnvironment() const {
        Environment env = configureEnv.environment();
        addToEnvironment(env);

        return env;
    }

    QString XMakeBuildSystem::xmakeBuildType() const {
        auto setBuildTypeFromConfig = [this](const XMakeConfig &config) {
            auto it = std::find_if(config.begin(), config.end(), [](const XMakeConfigItem &item) {
                                       return item.key == "XMAKE_BUILD_TYPE" && !item.isInitial;
                                   });
            if (it != config.end()) {
                xmakeBuildConfiguration()->setXMakeBuildType(QString::fromUtf8(it->value));
            }
        };

        if (!isMultiConfig()) {
            setBuildTypeFromConfig(configurationChanges());
        }

        QString xmakeBuildType = xmakeBuildConfiguration()->buildTypeAspect();

        const Utils::FilePath xmakeCacheTxt = buildConfiguration()->buildDirectory().pathAppended(
            Constants::XMAKE_CACHE_TXT);
        const bool hasXMakeCache = xmakeCacheTxt.exists();
        XMakeConfig config;

        if (xmakeBuildType == "Unknown") {
            // The "Unknown" type is the case of loading of an existing project
            // that doesn't have the "XMake.Build.Type" aspect saved
            if (hasXMakeCache) {
                QString errorMessage;
                config = XMakeConfig::fromFile(xmakeCacheTxt, &errorMessage);
            } else {
                config = initialXMakeConfiguration();
            }
        } else if (!hasXMakeCache) {
            config = initialXMakeConfiguration();
        }

        if (!config.isEmpty() && !isMultiConfig()) {
            setBuildTypeFromConfig(config);
        }

        return xmakeBuildType;
    }

    void XMakeBuildConfiguration::setXMakeBuildType(const QString &xmakeBuildType, bool quiet) {
        buildTypeAspect.setValue(xmakeBuildType, quiet ? BaseAspect::BeQuiet : BaseAspect::DoEmit);
    }

    namespace Internal {
// ----------------------------------------------------------------------
// - InitialXMakeParametersAspect:
// ----------------------------------------------------------------------

        const XMakeConfig &InitialXMakeArgumentsAspect::xmakeConfiguration() const {
            return m_xmakeConfiguration;
        }

        const QStringList InitialXMakeArgumentsAspect::allValues() const {
            QStringList initialXMakeArguments = Utils::transform(m_xmakeConfiguration.toList(),
                                                                 [](const XMakeConfigItem &ci) {
                                                                     return ci.toArgument(nullptr);
                                                                 });

            initialXMakeArguments.append(ProcessArgs::splitArgs(value(), HostOsInfo::hostOs()));

            return initialXMakeArguments;
        }

        void InitialXMakeArgumentsAspect::setAllValues(const QString &values, QStringList &additionalOptions) {
            QStringList arguments = values.split('\n', Qt::SkipEmptyParts);
            QString xmakeGenerator;
            for (QString &arg: arguments) {
                if (arg.startsWith("-G")) {
                    arg.replace("-G", "-DXMAKE_GENERATOR:STRING=");
                }
                if (arg.startsWith("-A")) {
                    arg.replace("-A", "-DXMAKE_GENERATOR_PLATFORM:STRING=");
                }
                if (arg.startsWith("-T")) {
                    arg.replace("-T", "-DXMAKE_GENERATOR_TOOLSET:STRING=");
                }
            }
            if (!xmakeGenerator.isEmpty()) {
                arguments.append(xmakeGenerator);
            }

            m_xmakeConfiguration = XMakeConfig::fromArguments(arguments, additionalOptions);
            for (XMakeConfigItem &ci : m_xmakeConfiguration) {
                ci.isInitial = true;
            }

            // Display the unknown arguments in "Additional XMake Options"
            const QString additionalOptionsValue = ProcessArgs::joinArgs(additionalOptions);
            setValue(additionalOptionsValue, BeQuiet);
        }

        void InitialXMakeArgumentsAspect::setXMakeConfiguration(const XMakeConfig &config) {
            m_xmakeConfiguration = config;
            for (XMakeConfigItem &ci : m_xmakeConfiguration) {
                ci.isInitial = true;
            }
        }

        void InitialXMakeArgumentsAspect::fromMap(const Store &map) {
            const QString value = map.value(settingsKey(), defaultValue()).toString();
            QStringList additionalArguments;
            setAllValues(value, additionalArguments);
        }

        void InitialXMakeArgumentsAspect::toMap(Store &map) const {
            saveToMap(map, allValues().join('\n'), defaultValue(), settingsKey());
        }

        InitialXMakeArgumentsAspect::InitialXMakeArgumentsAspect(AspectContainer *container)
            : StringAspect(container) {
            setSettingsKey("XMake.Initial.Parameters");
            setLabelText(Tr::tr("Additional XMake <a href=\"options\">options</a>:"));
            setDisplayStyle(LineEditDisplay);
        }

// -----------------------------------------------------------------------------
// ConfigureEnvironmentAspect:
// -----------------------------------------------------------------------------
        class ConfigureEnvironmentAspectWidget final : public EnvironmentAspectWidget {
public:
            ConfigureEnvironmentAspectWidget(ConfigureEnvironmentAspect *aspect, Target *target)
                : EnvironmentAspectWidget(aspect) {
                envWidget()->setOpenTerminalFunc([target](const Environment &env) {
                                                     if (BuildConfiguration *bc = target->activeBuildConfiguration()) {
                                                         Core::FileUtils::openTerminal(bc->buildDirectory(), env);
                                                     }
                                                 });
            }
        };

        ConfigureEnvironmentAspect::ConfigureEnvironmentAspect(AspectContainer *container,
                                                               BuildConfiguration *bc)
            : EnvironmentAspect(container) {
            Target *target = bc->target();
            setIsLocal(true);
            setAllowPrintOnRun(false);
            setConfigWidgetCreator(
                [this, target] {
                    return new ConfigureEnvironmentAspectWidget(this, target);
                });
            addSupportedBaseEnvironment(Tr::tr("Clean Environment"), {});
            setLabelText(Tr::tr("Base environment for the XMake configure step:"));

            const int systemEnvIndex = addSupportedBaseEnvironment(Tr::tr("System Environment"), [target] {
                                                                       IDevice::ConstPtr device = BuildDeviceKitAspect::device(target->kit());
                                                                       return device ? device->systemEnvironment() : Environment::systemEnvironment();
                                                                   });

            const int buildEnvIndex = addSupportedBaseEnvironment(Tr::tr("Build Environment"), [bc] {
                                                                      return bc->environment();
                                                                  });

            connect(target,
                    &Target::activeBuildConfigurationChanged,
                    this,
                    &EnvironmentAspect::environmentChanged);
            connect(target,
                    &Target::buildEnvironmentChanged,
                    this,
                    &EnvironmentAspect::environmentChanged);

            const XMakeConfigItem presetItem = XMakeConfigurationKitAspect::xmakePresetConfigItem(
                target->kit());

            setBaseEnvironmentBase(presetItem.isNull() ? buildEnvIndex : systemEnvIndex);

            connect(target->project(),
                    &Project::environmentChanged,
                    this,
                    &EnvironmentAspect::environmentChanged);

            connect(KitManager::instance(), &KitManager::kitUpdated, this, [this, target](const Kit *k) {
                        if (target->kit() == k) {
                            emit EnvironmentAspect::environmentChanged();
                        }
                    });

            addModifier([target](Utils::Environment &env) {
                            // This will add ninja to path
                            if (BuildConfiguration *bc = target->activeBuildConfiguration()) {
                                bc->addToEnvironment(env);
                            }
                            target->kit()->addToBuildEnvironment(env);
                            env.modify(target->project()->additionalEnvironment());
                        });
        }

        void ConfigureEnvironmentAspect::fromMap(const Store &map) {
            // Match the key values from Qt Creator 9.0.0/1 to the ones from EnvironmentAspect
            const bool cleanSystemEnvironment = map.value(CLEAR_SYSTEM_ENVIRONMENT_KEY).toBool();
            const QStringList userEnvironmentChanges
                = map.value(USER_ENVIRONMENT_CHANGES_KEY).toStringList();

            const int baseEnvironmentIndex = map.value(BASE_ENVIRONMENT_KEY, baseEnvironmentBase()).toInt();

            Store tmpMap;
            tmpMap.insert(BASE_KEY, cleanSystemEnvironment ? 0 : baseEnvironmentIndex);
            tmpMap.insert(CHANGES_KEY, userEnvironmentChanges);

            ProjectExplorer::EnvironmentAspect::fromMap(tmpMap);
        }

        void ConfigureEnvironmentAspect::toMap(Store &map) const {
            Store tmpMap;
            ProjectExplorer::EnvironmentAspect::toMap(tmpMap);

            const int baseKey = tmpMap.value(BASE_KEY).toInt();

            map.insert(CLEAR_SYSTEM_ENVIRONMENT_KEY, baseKey == 0);
            map.insert(BASE_ENVIRONMENT_KEY, baseKey);
            map.insert(USER_ENVIRONMENT_CHANGES_KEY, tmpMap.value(CHANGES_KEY).toStringList());
        }

        void setupXMakeBuildConfiguration() {
            static XMakeBuildConfigurationFactory theXMakeBuildConfigurationFactory;
        }
    } // namespace Internal
} // namespace XMakeProjectManager
