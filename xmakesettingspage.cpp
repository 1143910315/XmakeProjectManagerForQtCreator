// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "xmakesettingspage.h"

#include "xmakeprojectconstants.h"
#include "xmakeprojectmanagertr.h"
#include "xmaketool.h"
#include "xmaketoolmanager.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/detailswidget.h>
#include <utils/fileutils.h>
#include <utils/headerviewstretcher.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/treemodel.h>
#include <utils/utilsicons.h>

#include <QCheckBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeView>
#include <QUuid>

using namespace Utils;

namespace XMakeProjectManager::Internal {

class XMakeToolTreeItem;

//
// XMakeToolItemModel
//

class XMakeToolItemModel : public TreeModel<TreeItem, TreeItem, XMakeToolTreeItem>
{
public:
    XMakeToolItemModel();

    XMakeToolTreeItem *xmakeToolItem(const Utils::Id &id) const;
    XMakeToolTreeItem *xmakeToolItem(const QModelIndex &index) const;
    QModelIndex addXMakeTool(const QString &name,
                             const FilePath &executable,
                             const FilePath &qchFile,
                             const bool autoRun,
                             const bool isAutoDetected);
    void addXMakeTool(const XMakeTool *item, bool changed);
    TreeItem *autoGroupItem() const;
    TreeItem *manualGroupItem() const;
    void reevaluateChangedFlag(XMakeToolTreeItem *item) const;
    void updateXMakeTool(const Utils::Id &id,
                         const QString &displayName,
                         const FilePath &executable,
                         const FilePath &qchFile);
    void removeXMakeTool(const Utils::Id &id);
    void apply();

    Utils::Id defaultItemId() const;
    void setDefaultItemId(const Utils::Id &id);

    QString uniqueDisplayName(const QString &base) const;
private:
    Utils::Id m_defaultItemId;
    QList<Utils::Id> m_removedItems;
};

class XMakeToolTreeItem : public TreeItem
{
public:
    XMakeToolTreeItem(const XMakeTool *item, bool changed)
        : m_id(item->id())
        , m_name(item->displayName())
        , m_executable(item->filePath())
        , m_qchFile(item->qchFilePath())
        , m_versionDisplay(item->versionDisplay())
        , m_detectionSource(item->detectionSource())
        , m_autodetected(item->isAutoDetected())
        , m_isSupported(item->hasFileApi())
        , m_changed(changed)
    {
        updateErrorFlags();
    }

    XMakeToolTreeItem(const QString &name,
                      const FilePath &executable,
                      const FilePath &qchFile,
                      bool autoRun,
                      bool autodetected)
        : m_id(Id::fromString(QUuid::createUuid().toString()))
        , m_name(name)
        , m_executable(executable)
        , m_qchFile(qchFile)
        , m_isAutoRun(autoRun)
        , m_autodetected(autodetected)
    {
        updateErrorFlags();
    }

    void updateErrorFlags()
    {
        const FilePath filePath = XMakeTool::xmakeExecutable(m_executable);
        m_pathExists = filePath.exists();
        m_pathIsFile = filePath.isFile();
        m_pathIsExecutable = filePath.isExecutableFile();

        XMakeTool xmake(m_autodetected ? XMakeTool::AutoDetection
                                       : XMakeTool::ManualDetection, m_id);
        xmake.setFilePath(m_executable);
        m_isSupported = xmake.hasFileApi();

        m_tooltip = Tr::tr("Version: %1").arg(xmake.versionDisplay());
        m_tooltip += "<br>" + Tr::tr("Supports fileApi: %1").arg(m_isSupported ? Tr::tr("yes") : Tr::tr("no"));
        m_tooltip += "<br>" + Tr::tr("Detection source: \"%1\"").arg(m_detectionSource);

        m_versionDisplay = xmake.versionDisplay();

        // Make sure to always have the right version in the name for Qt SDK XMake installations
        if (m_autodetected && m_name.startsWith("XMake") && m_name.endsWith("(Qt)"))
            m_name = QString("XMake %1 (Qt)").arg(m_versionDisplay);
    }

    XMakeToolTreeItem() = default;

    XMakeToolItemModel *model() const { return static_cast<XMakeToolItemModel *>(TreeItem::model()); }

    QVariant data(int column, int role) const override
    {
        switch (role) {
        case Qt::DisplayRole: {
            switch (column) {
            case 0: {
                QString name = m_name;
                if (model()->defaultItemId() == m_id)
                    name += Tr::tr(" (Default)");
                return name;
            }
            case 1: {
                return m_executable.toUserOutput();
            }
            } // switch (column)
            return QVariant();
        }
        case Qt::FontRole: {
            QFont font;
            font.setBold(m_changed);
            font.setItalic(model()->defaultItemId() == m_id);
            return font;
        }
        case Qt::ToolTipRole: {
            QString result = m_tooltip;
            QString error;
            if (!m_pathExists) {
                error = Tr::tr("XMake executable path does not exist.");
            } else if (!m_pathIsFile) {
                error = Tr::tr("XMake executable path is not a file.");
            } else if (!m_pathIsExecutable) {
                error = Tr::tr("XMake executable path is not executable.");
            } else if (!m_isSupported) {
                error = Tr::tr(
                    "XMake executable does not provide required IDE integration features.");
            }
            if (result.isEmpty() || error.isEmpty())
                return QString("%1%2").arg(result).arg(error);
            else
                return QString("%1<br><br><b>%2</b>").arg(result).arg(error);
        }
        case Qt::DecorationRole: {
            if (column != 0)
                return QVariant();

            const bool hasError = !m_isSupported || !m_pathExists || !m_pathIsFile
                                  || !m_pathIsExecutable;
            if (hasError)
                return Icons::CRITICAL.icon();
            return QVariant();
        }
        }
        return QVariant();
    }

    Id m_id;
    QString m_name;
    QString m_tooltip;
    FilePath m_executable;
    FilePath m_qchFile;
    QString m_versionDisplay;
    QString m_detectionSource;
    bool m_isAutoRun = true;
    bool m_pathExists = false;
    bool m_pathIsFile = false;
    bool m_pathIsExecutable = false;
    bool m_autodetected = false;
    bool m_isSupported = false;
    bool m_changed = true;
};

XMakeToolItemModel::XMakeToolItemModel()
{
    setHeader({Tr::tr("Name"), Tr::tr("Path")});
    rootItem()->appendChild(
        new StaticTreeItem({ProjectExplorer::Constants::msgAutoDetected()},
                           {ProjectExplorer::Constants::msgAutoDetectedToolTip()}));
    rootItem()->appendChild(new StaticTreeItem(Tr::tr("Manual")));

    const QList<XMakeTool *> items = XMakeToolManager::xmakeTools();
    for (const XMakeTool *item : items)
        addXMakeTool(item, false);

    XMakeTool *defTool = XMakeToolManager::defaultXMakeTool();
    m_defaultItemId = defTool ? defTool->id() : Id();
    connect(XMakeToolManager::instance(), &XMakeToolManager::xmakeRemoved,
            this, &XMakeToolItemModel::removeXMakeTool);
    connect(XMakeToolManager::instance(), &XMakeToolManager::xmakeAdded,
            this, [this](const Id &id) { addXMakeTool(XMakeToolManager::findById(id), false); });

}

QModelIndex XMakeToolItemModel::addXMakeTool(const QString &name,
                                             const FilePath &executable,
                                             const FilePath &qchFile,
                                             const bool autoRun,
                                             const bool isAutoDetected)
{
    auto item = new XMakeToolTreeItem(name, executable, qchFile, autoRun, isAutoDetected);
    if (isAutoDetected)
        autoGroupItem()->appendChild(item);
    else
        manualGroupItem()->appendChild(item);

    return item->index();
}

void XMakeToolItemModel::addXMakeTool(const XMakeTool *item, bool changed)
{
    QTC_ASSERT(item, return);

    if (xmakeToolItem(item->id()))
        return;

    auto treeItem = new XMakeToolTreeItem(item, changed);
    if (item->isAutoDetected())
        autoGroupItem()->appendChild(treeItem);
    else
        manualGroupItem()->appendChild(treeItem);
}

TreeItem *XMakeToolItemModel::autoGroupItem() const
{
    return rootItem()->childAt(0);
}

TreeItem *XMakeToolItemModel::manualGroupItem() const
{
    return rootItem()->childAt(1);
}

void XMakeToolItemModel::reevaluateChangedFlag(XMakeToolTreeItem *item) const
{
    XMakeTool *orig = XMakeToolManager::findById(item->m_id);
    item->m_changed = !orig || orig->displayName() != item->m_name
                      || orig->filePath() != item->m_executable
                      || orig->qchFilePath() != item->m_qchFile;

    //make sure the item is marked as changed when the default xmake was changed
    XMakeTool *origDefTool = XMakeToolManager::defaultXMakeTool();
    Id origDefault = origDefTool ? origDefTool->id() : Id();
    if (origDefault != m_defaultItemId) {
        if (item->m_id == origDefault || item->m_id == m_defaultItemId)
            item->m_changed = true;
    }

    item->update(); // Notify views.
}

void XMakeToolItemModel::updateXMakeTool(const Id &id,
                                         const QString &displayName,
                                         const FilePath &executable,
                                         const FilePath &qchFile)
{
    XMakeToolTreeItem *treeItem = xmakeToolItem(id);
    QTC_ASSERT(treeItem, return );

    treeItem->m_name = displayName;
    treeItem->m_executable = executable;
    treeItem->m_qchFile = qchFile;

    treeItem->updateErrorFlags();

    reevaluateChangedFlag(treeItem);
}

XMakeToolTreeItem *XMakeToolItemModel::xmakeToolItem(const Id &id) const
{
    return findItemAtLevel<2>([id](XMakeToolTreeItem *n) { return n->m_id == id; });
}

XMakeToolTreeItem *XMakeToolItemModel::xmakeToolItem(const QModelIndex &index) const
{
    return itemForIndexAtLevel<2>(index);
}

void XMakeToolItemModel::removeXMakeTool(const Id &id)
{
    if (m_removedItems.contains(id))
        return; // Item has already been removed in the model!

    XMakeToolTreeItem *treeItem = xmakeToolItem(id);
    QTC_ASSERT(treeItem, return);

    m_removedItems.append(id);
    destroyItem(treeItem);
}

void XMakeToolItemModel::apply()
{
    for (const Id &id : std::as_const(m_removedItems))
        XMakeToolManager::deregisterXMakeTool(id);

    QList<XMakeToolTreeItem *> toRegister;
    forItemsAtLevel<2>([&toRegister](XMakeToolTreeItem *item) {
        item->m_changed = false;
        if (XMakeTool *xmake = XMakeToolManager::findById(item->m_id)) {
            xmake->setDisplayName(item->m_name);
            xmake->setFilePath(item->m_executable);
            xmake->setQchFilePath(item->m_qchFile);
            xmake->setDetectionSource(item->m_detectionSource);
        } else {
            toRegister.append(item);
        }
    });

    for (XMakeToolTreeItem *item : std::as_const(toRegister)) {
        XMakeTool::Detection detection = item->m_autodetected ? XMakeTool::AutoDetection
                                                              : XMakeTool::ManualDetection;
        auto xmake = std::make_unique<XMakeTool>(detection, item->m_id);
        xmake->setDisplayName(item->m_name);
        xmake->setFilePath(item->m_executable);
        xmake->setQchFilePath(item->m_qchFile);
        xmake->setDetectionSource(item->m_detectionSource);
        if (!XMakeToolManager::registerXMakeTool(std::move(xmake)))
            item->m_changed = true;
    }

    XMakeToolManager::setDefaultXMakeTool(defaultItemId());
}

Id XMakeToolItemModel::defaultItemId() const
{
    return m_defaultItemId;
}

void XMakeToolItemModel::setDefaultItemId(const Id &id)
{
    if (m_defaultItemId == id)
        return;

    Id oldDefaultId = m_defaultItemId;
    m_defaultItemId = id;

    XMakeToolTreeItem *newDefault = xmakeToolItem(id);
    if (newDefault)
        reevaluateChangedFlag(newDefault);

    XMakeToolTreeItem *oldDefault = xmakeToolItem(oldDefaultId);
    if (oldDefault)
        reevaluateChangedFlag(oldDefault);
}


QString XMakeToolItemModel::uniqueDisplayName(const QString &base) const
{
    QStringList names;
    forItemsAtLevel<2>([&names](XMakeToolTreeItem *item) { names << item->m_name; });
    return Utils::makeUniquelyNumbered(base, names);
}

//
// XMakeToolItemConfigWidget
//

class XMakeToolItemConfigWidget : public QWidget
{
public:
    explicit XMakeToolItemConfigWidget(XMakeToolItemModel *model);
    void load(const XMakeToolTreeItem *item);
    void store() const;

private:
    void onBinaryPathEditingFinished();
    void updateQchFilePath();

    XMakeToolItemModel *m_model;
    QLineEdit *m_displayNameLineEdit;
    PathChooser *m_binaryChooser;
    PathChooser *m_qchFileChooser;
    QLabel *m_versionLabel;
    Id m_id;
    bool m_loadingItem;
};

XMakeToolItemConfigWidget::XMakeToolItemConfigWidget(XMakeToolItemModel *model)
    : m_model(model), m_loadingItem(false)
{
    m_displayNameLineEdit = new QLineEdit(this);

    m_binaryChooser = new PathChooser(this);
    m_binaryChooser->setExpectedKind(PathChooser::ExistingCommand);
    m_binaryChooser->setMinimumWidth(400);
    m_binaryChooser->setHistoryCompleter("Cmake.Command.History");
    m_binaryChooser->setCommandVersionArguments({"--version"});
    m_binaryChooser->setAllowPathFromDevice(true);

    m_qchFileChooser = new PathChooser(this);
    m_qchFileChooser->setExpectedKind(PathChooser::File);
    m_qchFileChooser->setMinimumWidth(400);
    m_qchFileChooser->setHistoryCompleter("Cmake.qchFile.History");
    m_qchFileChooser->setPromptDialogFilter("*.qch");
    m_qchFileChooser->setPromptDialogTitle(Tr::tr("XMake .qch File"));

    m_versionLabel = new QLabel(this);

    auto formLayout = new QFormLayout(this);
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    formLayout->addRow(new QLabel(Tr::tr("Name:")), m_displayNameLineEdit);
    formLayout->addRow(new QLabel(Tr::tr("Path:")), m_binaryChooser);
    formLayout->addRow(new QLabel(Tr::tr("Version:")), m_versionLabel);
    formLayout->addRow(new QLabel(Tr::tr("Help file:")), m_qchFileChooser);

    connect(m_binaryChooser, &PathChooser::browsingFinished, this, &XMakeToolItemConfigWidget::onBinaryPathEditingFinished);
    connect(m_binaryChooser, &PathChooser::editingFinished, this, &XMakeToolItemConfigWidget::onBinaryPathEditingFinished);
    connect(m_qchFileChooser, &PathChooser::rawPathChanged, this, &XMakeToolItemConfigWidget::store);
    connect(m_displayNameLineEdit, &QLineEdit::textChanged, this, &XMakeToolItemConfigWidget::store);
}

void XMakeToolItemConfigWidget::store() const
{
    if (!m_loadingItem && m_id.isValid())
        m_model->updateXMakeTool(m_id,
                                 m_displayNameLineEdit->text(),
                                 m_binaryChooser->filePath(),
                                 m_qchFileChooser->filePath());
}

void XMakeToolItemConfigWidget::onBinaryPathEditingFinished()
{
    updateQchFilePath();
    store();
    load(m_model->xmakeToolItem(m_id));
}

void XMakeToolItemConfigWidget::updateQchFilePath()
{
    // QDS does not want automatic detection of xmake help file
    if (Core::ICore::isQtDesignStudio())
        return;
    if (m_qchFileChooser->filePath().isEmpty())
        m_qchFileChooser->setFilePath(XMakeTool::searchQchFile(m_binaryChooser->filePath()));
}

void XMakeToolItemConfigWidget::load(const XMakeToolTreeItem *item)
{
    m_loadingItem = true; // avoid intermediate signal handling
    m_id = Id();
    if (!item) {
        m_loadingItem = false;
        return;
    }

    // Set values:
    m_displayNameLineEdit->setEnabled(!item->m_autodetected);
    m_displayNameLineEdit->setText(item->m_name);

    m_binaryChooser->setReadOnly(item->m_autodetected);
    m_binaryChooser->setFilePath(item->m_executable);

    m_qchFileChooser->setReadOnly(item->m_autodetected);
    m_qchFileChooser->setBaseDirectory(item->m_executable.parentDir());
    m_qchFileChooser->setFilePath(item->m_qchFile);

    m_versionLabel->setText(item->m_versionDisplay);

    m_id = item->m_id;
    m_loadingItem = false;
}

//
// XMakeToolConfigWidget
//

class XMakeToolConfigWidget : public Core::IOptionsPageWidget
{
public:
    XMakeToolConfigWidget()
    {
        m_addButton = new QPushButton(Tr::tr("Add"), this);

        m_cloneButton = new QPushButton(Tr::tr("Clone"), this);
        m_cloneButton->setEnabled(false);

        m_delButton = new QPushButton(Tr::tr("Remove"), this);
        m_delButton->setEnabled(false);

        m_makeDefButton = new QPushButton(Tr::tr("Make Default"), this);
        m_makeDefButton->setEnabled(false);
        m_makeDefButton->setToolTip(Tr::tr("Set as the default XMake Tool to use when creating a new kit or when no value is set."));

        m_container = new DetailsWidget(this);
        m_container->setState(DetailsWidget::NoSummary);
        m_container->setVisible(false);

        m_xmakeToolsView = new QTreeView(this);
        m_xmakeToolsView->setModel(&m_model);
        m_xmakeToolsView->setUniformRowHeights(true);
        m_xmakeToolsView->setSelectionMode(QAbstractItemView::SingleSelection);
        m_xmakeToolsView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_xmakeToolsView->expandAll();

        QHeaderView *header = m_xmakeToolsView->header();
        header->setStretchLastSection(false);
        header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        header->setSectionResizeMode(1, QHeaderView::Stretch);
        (void) new HeaderViewStretcher(header, 0);

        auto buttonLayout = new QVBoxLayout();
        buttonLayout->setContentsMargins(0, 0, 0, 0);
        buttonLayout->addWidget(m_addButton);
        buttonLayout->addWidget(m_cloneButton);
        buttonLayout->addWidget(m_delButton);
        buttonLayout->addWidget(m_makeDefButton);
        buttonLayout->addItem(new QSpacerItem(10, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

        auto verticalLayout = new QVBoxLayout();
        verticalLayout->addWidget(m_xmakeToolsView);
        verticalLayout->addWidget(m_container);

        auto horizontalLayout = new QHBoxLayout(this);
        horizontalLayout->addLayout(verticalLayout);
        horizontalLayout->addLayout(buttonLayout);

        connect(m_xmakeToolsView->selectionModel(), &QItemSelectionModel::currentChanged,
                this, &XMakeToolConfigWidget::currentXMakeToolChanged, Qt::QueuedConnection);

        connect(m_addButton, &QAbstractButton::clicked,
                this, &XMakeToolConfigWidget::addXMakeTool);
        connect(m_cloneButton, &QAbstractButton::clicked,
                this, &XMakeToolConfigWidget::cloneXMakeTool);
        connect(m_delButton, &QAbstractButton::clicked,
                this, &XMakeToolConfigWidget::removeXMakeTool);
        connect(m_makeDefButton, &QAbstractButton::clicked,
                this, &XMakeToolConfigWidget::setDefaultXMakeTool);

        m_itemConfigWidget = new XMakeToolItemConfigWidget(&m_model);
        m_container->setWidget(m_itemConfigWidget);
    }

    void apply() final;

    void cloneXMakeTool();
    void addXMakeTool();
    void removeXMakeTool();
    void setDefaultXMakeTool();
    void currentXMakeToolChanged(const QModelIndex &newCurrent);

    XMakeToolItemModel m_model;
    QTreeView *m_xmakeToolsView;
    QPushButton *m_addButton;
    QPushButton *m_cloneButton;
    QPushButton *m_delButton;
    QPushButton *m_makeDefButton;
    DetailsWidget *m_container;
    XMakeToolItemConfigWidget *m_itemConfigWidget;
    XMakeToolTreeItem *m_currentItem = nullptr;
};

void XMakeToolConfigWidget::apply()
{
    m_itemConfigWidget->store();
    m_model.apply();
}

void XMakeToolConfigWidget::cloneXMakeTool()
{
    if (!m_currentItem)
        return;

    QModelIndex newItem = m_model.addXMakeTool(Tr::tr("Clone of %1").arg(m_currentItem->m_name),
                                               m_currentItem->m_executable,
                                               m_currentItem->m_qchFile,
                                               m_currentItem->m_isAutoRun,
                                               false);

    m_xmakeToolsView->setCurrentIndex(newItem);
}

void XMakeToolConfigWidget::addXMakeTool()
{
    QModelIndex newItem = m_model.addXMakeTool(m_model.uniqueDisplayName(Tr::tr("New XMake")),
                                               FilePath(),
                                               FilePath(),
                                               true,
                                               false);

    m_xmakeToolsView->setCurrentIndex(newItem);
}

void XMakeToolConfigWidget::removeXMakeTool()
{
    bool delDef = m_model.defaultItemId() == m_currentItem->m_id;
    m_model.removeXMakeTool(m_currentItem->m_id);
    m_currentItem = nullptr;

    if (delDef) {
        auto it = static_cast<XMakeToolTreeItem *>(m_model.autoGroupItem()->firstChild());
        if (!it)
            it = static_cast<XMakeToolTreeItem *>(m_model.manualGroupItem()->firstChild());
        if (it)
            m_model.setDefaultItemId(it->m_id);
    }

    TreeItem *newCurrent = m_model.manualGroupItem()->lastChild();
    if (!newCurrent)
        newCurrent = m_model.autoGroupItem()->lastChild();

    if (newCurrent)
        m_xmakeToolsView->setCurrentIndex(newCurrent->index());
}

void XMakeToolConfigWidget::setDefaultXMakeTool()
{
    if (!m_currentItem)
        return;

    m_model.setDefaultItemId(m_currentItem->m_id);
    m_makeDefButton->setEnabled(false);
}

void XMakeToolConfigWidget::currentXMakeToolChanged(const QModelIndex &newCurrent)
{
    m_currentItem = m_model.xmakeToolItem(newCurrent);
    m_itemConfigWidget->load(m_currentItem);
    m_container->setVisible(m_currentItem);
    m_cloneButton->setEnabled(m_currentItem);
    m_delButton->setEnabled(m_currentItem && !m_currentItem->m_autodetected);
    m_makeDefButton->setEnabled(m_currentItem && (!m_model.defaultItemId().isValid() || m_currentItem->m_id != m_model.defaultItemId()));
}

// XMakeSettingsPage

class XMakeSettingsPage final : public Core::IOptionsPage
{
public:
    XMakeSettingsPage()
    {
        setId(Constants::Settings::TOOLS_ID);
        setDisplayName(Tr::tr("Tools"));
        setDisplayCategory("XMake");
        setCategory(Constants::Settings::CATEGORY);
        setWidgetCreator([] { return new XMakeToolConfigWidget; });
    }
};

void setupXMakeSettingsPage()
{
    static XMakeSettingsPage theXMakeSettingsPage;
}

} // XMakeProjectManager::Internal
