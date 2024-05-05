// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "xmakeeditor.h"
#include "xmaketoolmanager.h"

#include "xmakeautocompleter.h"
#include "xmakebuildsystem.h"
#include "xmakefilecompletionassist.h"
#include "xmakeindenter.h"
#include "xmakeprojectconstants.h"

#include "3rdparty/xmake/cmListFileCache.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreplugintr.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/target.h>

#include <texteditor/basehoverhandler.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditoractionhandler.h>

#include <utils/mimeconstants.h>
#include <utils/textutils.h>
#include <utils/tooltip/tooltip.h>

#include <functional>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;
using namespace TextEditor;

namespace XMakeProjectManager::Internal {

//
// XMakeEditor
//

class XMakeEditor final : public BaseTextEditor
{
public:
    XMakeEditor();
    void contextHelp(const HelpCallback &callback) const final;

private:
    XMakeKeywords m_keywords;
};

XMakeEditor::XMakeEditor()
{
    if (auto tool = XMakeToolManager::defaultProjectOrDefaultXMakeTool())
        m_keywords = tool->keywords();
}

void XMakeEditor::contextHelp(const HelpCallback &callback) const
{
    auto helpPrefix = [this](const QString &word) {
        if (m_keywords.includeStandardModules.contains(word))
            return "module/";
        if (m_keywords.functions.contains(word))
            return "command/";
        if (m_keywords.variables.contains(word))
            return "variable/";
        if (m_keywords.directoryProperties.contains(word))
            return "prop_dir/";
        if (m_keywords.targetProperties.contains(word))
            return "prop_tgt/";
        if (m_keywords.sourceProperties.contains(word))
            return "prop_sf/";
        if (m_keywords.testProperties.contains(word))
            return "prop_test/";
        if (m_keywords.properties.contains(word))
            return "prop_gbl/";
        if (m_keywords.policies.contains(word))
            return "policy/";
        if (m_keywords.environmentVariables.contains(word))
            return "envvar/";

        return "unknown/";
    };

    const QString word = Text::wordUnderCursor(editorWidget()->textCursor());
    const QString id = helpPrefix(word) + word;
    if (id.startsWith("unknown/")) {
        BaseTextEditor::contextHelp(callback);
        return;
    }

    callback({{id, word}, {}, {}, HelpItem::Unknown});
}

//
// XMakeEditorWidget
//

class XMakeEditorWidget final : public TextEditorWidget
{
public:
    ~XMakeEditorWidget() final = default;

private:
    void findLinkAt(const QTextCursor &cursor,
                    const LinkHandler &processLinkCallback,
                    bool resolveTarget = true,
                    bool inNextSplit = false) final;
    void contextMenuEvent(QContextMenuEvent *e) final;
};

void XMakeEditorWidget::contextMenuEvent(QContextMenuEvent *e)
{
    showDefaultContextMenu(e, Constants::M_CONTEXT);
}

static bool mustBeQuotedInFileName(const QChar &c)
{
    return c.isSpace() || c == '"' || c == '(' || c == ')';
}

static bool isValidFileNameChar(const QString &block, int pos)
{
    const QChar c = block.at(pos);
    return !mustBeQuotedInFileName(c) || (pos > 0 && block.at(pos - 1) == '\\');
}

static QString unescape(const QString &s)
{
    QString result;
    int i = 0;
    const qsizetype size = s.size();
    while (i < size) {
        const QChar c = s.at(i);
        if (c == '\\' && i < size - 1) {
            const QChar nc = s.at(i + 1);
            if (mustBeQuotedInFileName(nc)) {
                result += nc;
                i += 2;
                continue;
            }
        }
        result += c;
        ++i;
    }
    return result;
}

static bool isValidUrlChar(const QChar &c)
{
    static QSet<QChar> urlChars{'-', '.', '_',  '~', ':', '/', '?', '#', '[', ']', '@', '!',
                                '$', '&', '\'', '(', ')', '*', '+', ',', ';', '%', '='};

    return (c.isLetterOrNumber() || urlChars.contains(c)) && !c.isSpace();
}

static bool isValidIdentifierChar(const QChar &chr)
{
    return chr.isLetterOrNumber() || chr == '_' || chr == '-';
}

static QHash<QString, Link> getLocalSymbolsHash(const QByteArray &content,
                                                const FilePath &filePath, QString &projectName)
{
    cmListFile xmakeListFile;
    if (!content.isEmpty()) {
        std::string errorString;
        const std::string fileName = "buffer";
        if (!xmakeListFile.ParseString(content.toStdString(), fileName, errorString))
            return {};
    }

    QHash<QString, Link> hash;
    for (const auto &func : xmakeListFile.Functions) {
        if (func.LowerCaseName() == "project" && func.Arguments().size() > 0) {
            projectName = QString::fromUtf8(func.Arguments()[0].Value);
            continue;
        }

        if (func.LowerCaseName() != "function" && func.LowerCaseName() != "macro"
            && func.LowerCaseName() != "set" && func.LowerCaseName() != "option")
            continue;

        if (func.Arguments().size() == 0)
            continue;
        auto arg = func.Arguments()[0];

        Link link;
        link.targetFilePath = filePath;
        link.targetLine = arg.Line;
        link.targetColumn = arg.Column - 1;
        hash.insert(QString::fromUtf8(arg.Value), link);
    }
    return hash;
}

void XMakeEditorWidget::findLinkAt(const QTextCursor &cursor,
                                   const LinkHandler &processLinkCallback,
                                   bool/* resolveTarget*/,
                                   bool /*inNextSplit*/)
{
    Link link;

    int line = 0;
    int column = 0;
    convertPosition(cursor.position(), &line, &column);

    const QString block = cursor.block().text();

    int beginPos = 0;
    int endPos = 0;
    auto addTextStartEndToLink = [&](Link &link) {
        link.linkTextStart = cursor.position() - column + beginPos + 1;
        link.linkTextEnd = cursor.position() - column + endPos;
        return link;
    };

    // check if the current position is commented out
    const qsizetype hashPos = block.indexOf(QLatin1Char('#'));
    if (hashPos >= 0 && hashPos < column) {
        // Check to see if we have a https:// link
        QString buffer;
        beginPos = column - 1;
        while (beginPos > hashPos) {
            if (isValidUrlChar(block[beginPos])) {
                buffer.prepend(block.at(beginPos));
                beginPos--;
            } else {
                break;
            }
        }
        // find the end of the url
        endPos = column;
        while (endPos < block.size()) {
            if (isValidUrlChar(block[endPos])) {
                buffer.append(block.at(endPos));
                endPos++;
            } else {
                break;
            }
        }
        if (buffer.startsWith("http")) {
            link.targetFilePath = FilePath::fromPathPart(buffer);
            addTextStartEndToLink(link);
            return processLinkCallback(link);
        }

        return processLinkCallback(link);
    }

    // find the beginning of a filename
    QString buffer;
    beginPos = column - 1;
    while (beginPos >= 0) {
        if (isValidFileNameChar(block, beginPos)) {
            buffer.prepend(block.at(beginPos));
            beginPos--;
        } else {
            break;
        }
    }

    // find the end of a filename
    endPos = column;
    while (endPos < block.size()) {
        if (isValidFileNameChar(block, endPos)) {
            buffer.append(block.at(endPos));
            endPos++;
        } else {
            break;
        }
    }

    if (buffer.isEmpty())
        return processLinkCallback(link);

    const FilePath dir = textDocument()->filePath().absolutePath();
    buffer.replace("${CMAKE_CURRENT_SOURCE_DIR}", dir.path());
    buffer.replace("${CMAKE_CURRENT_LIST_DIR}", dir.path());

    // Lambdas to find the XMake function name
    auto findFunctionStart = [cursor, this]() -> int {
        int pos = cursor.position();
        QChar chr;
        do {
            chr = textDocument()->characterAt(--pos);
        } while (pos > 0 && chr != '(');

        if (pos > 0 && chr == '(') {
            // allow space between function name and (
            do {
                chr = textDocument()->characterAt(--pos);
            } while (pos > 0 && chr.isSpace());
            ++pos;
        }
        return pos;
    };
    auto findFunctionEnd = [cursor, this]() -> int {
        int pos = cursor.position();
        QChar chr;
        do {
            chr = textDocument()->characterAt(--pos);
        } while (pos > 0 && chr != ')');
        return pos;
    };
    auto findWordStart = [cursor, this](int pos) -> int {
        // Find start position
        QChar chr;
        do {
            chr = textDocument()->characterAt(--pos);
        } while (pos > 0 && isValidIdentifierChar(chr));

        return ++pos;
    };
    const int funcStart = findFunctionStart();
    const int funcEnd = findFunctionEnd();

    // Resolve local variables and functions
    QString projectName;
    auto hash = getLocalSymbolsHash(textDocument()->textAt(0, funcEnd + 1).toUtf8(),
                                    textDocument()->filePath(),
                                    projectName);
    if (!projectName.isEmpty())
        buffer.replace("${PROJECT_NAME}", projectName);

    if (auto project = ProjectTree::currentProject()) {
        buffer.replace("${CMAKE_SOURCE_DIR}", project->projectDirectory().path());
        if (auto bs = ProjectTree::currentBuildSystem(); bs->buildConfiguration()) {
            buffer.replace("${CMAKE_BINARY_DIR}", bs->buildConfiguration()->buildDirectory().path());

            // Get the path suffix from current source dir to project source dir and apply it
            // for the binary dir
            const QString relativePathSuffix = textDocument()
                                                   ->filePath()
                                                   .parentDir()
                                                   .relativePathFrom(project->projectDirectory())
                                                   .path();
            buffer.replace("${CMAKE_CURRENT_BINARY_DIR}",
                           bs->buildConfiguration()
                               ->buildDirectory()
                               .pathAppended(relativePathSuffix)
                               .path());

            // Check if the symbols is a user defined function or macro
            if (const auto cbs = qobject_cast<const XMakeBuildSystem *>(bs)) {
                // Strip variable coating
                if (buffer.startsWith("${") && buffer.endsWith("}"))
                    buffer = buffer.mid(2, buffer.size() - 3);

                QString functionName;
                if (funcStart > funcEnd) {
                    int funcStartPos = findWordStart(funcStart);
                    functionName = textDocument()->textAt(funcStartPos, funcStart - funcStartPos);
                }

                bool skipTarget = false;
                if (functionName.toLower() == "add_subdirectory") {
                    skipTarget = cbs->projectImportedTargets().contains(buffer)
                                 || cbs->buildTargetTitles().contains(buffer);
                }
                if (!skipTarget && cbs->xmakeSymbolsHash().contains(buffer)) {
                    link = cbs->xmakeSymbolsHash().value(buffer);
                    addTextStartEndToLink(link);
                    return processLinkCallback(link);
                }

                // Handle include(XMakeFileWithoutSuffix) and find_package(Package)
                if (!functionName.isEmpty()) {
                    struct FunctionToHash
                    {
                        QString functionName;
                        const QHash<QString, Link> &hash;
                    } functionToHashes[] = {{"include", cbs->dotXMakeFilesHash()},
                                            {"find_package", cbs->findPackagesFilesHash()}};

                    for (const auto &pair : functionToHashes) {
                        if (functionName == pair.functionName && pair.hash.contains(buffer)) {
                            link = pair.hash.value(buffer);
                            addTextStartEndToLink(link);
                            return processLinkCallback(link);
                        }
                    }
                }
            }
        }
    }
    // TODO: Resolve more variables

    // Strip variable coating
    if (buffer.startsWith("${") && buffer.endsWith("}"))
        buffer = buffer.mid(2, buffer.size() - 3);

    if (hash.contains(buffer)) {
        link = hash.value(buffer);
        addTextStartEndToLink(link);
        return processLinkCallback(link);
    }

    FilePath fileName = dir.withNewPath(unescape(buffer));
    if (fileName.isRelativePath())
        fileName = dir.pathAppended(fileName.path());
    if (fileName.exists()) {
        if (fileName.isDir()) {
            FilePath subProject = fileName.pathAppended(Constants::CMAKE_LISTS_TXT);
            if (subProject.exists())
                fileName = subProject;
            else
                return processLinkCallback(link);
        }
        link.targetFilePath = fileName;
        addTextStartEndToLink(link);
    }

    processLinkCallback(link);
}

static TextDocument *createXMakeDocument()
{
    auto doc = new TextDocument;
    doc->setId(Constants::CMAKE_EDITOR_ID);
    doc->setMimeType(Utils::Constants::CMAKE_MIMETYPE);
    return doc;
}

//
// XMakeHoverHandler
//

class XMakeHoverHandler final : public TextEditor::BaseHoverHandler
{
    mutable XMakeKeywords m_keywords;
    QString m_helpToolTip;
    QVariant m_contextHelp;

public:
    const XMakeKeywords &keywords() const;

    void identifyMatch(TextEditorWidget *editorWidget,
                       int pos,
                       ReportPriority report) final;
    void operateTooltip(TextEditorWidget *editorWidget, const QPoint &point) final;
};

const XMakeKeywords &XMakeHoverHandler::keywords() const
{
    if (m_keywords.functions.isEmpty())
        if (auto tool = XMakeToolManager::defaultProjectOrDefaultXMakeTool())
            m_keywords = tool->keywords();

    return m_keywords;
}

void XMakeHoverHandler::identifyMatch(TextEditorWidget *editorWidget,
                                      int pos,
                                      ReportPriority report)
{
    const QScopeGuard cleanup([this, report] { report(priority()); });

    QTextCursor cursor = editorWidget->textCursor();
    cursor.setPosition(pos);
    const QString word = Text::wordUnderCursor(cursor);

    FilePath helpFile;
    QString helpCategory;
    struct
    {
        const QMap<QString, FilePath> &map;
        QString helpCategory;
    } keywordsListMaps[] = {{keywords().functions, "command"},
                            {keywords().variables, "variable"},
                            {keywords().directoryProperties, "prop_dir"},
                            {keywords().sourceProperties, "prop_sf"},
                            {keywords().targetProperties, "prop_tgt"},
                            {keywords().testProperties, "prop_test"},
                            {keywords().properties, "prop_gbl"},
                            {keywords().includeStandardModules, "module"},
                            {keywords().findModules, "module"},
                            {keywords().policies, "policy"},
                            {keywords().environmentVariables, "envvar"}};

    for (const auto &pair : keywordsListMaps) {
        if (pair.map.contains(word)) {
            helpFile = pair.map.value(word);
            helpCategory = pair.helpCategory;
            break;
        }
    }
    m_helpToolTip.clear();
    if (!helpFile.isEmpty())
        m_helpToolTip = XMakeToolManager::toolTipForRstHelpFile(helpFile);

    m_contextHelp = QVariant::fromValue(
        HelpItem({QString("%1/%2").arg(helpCategory, word), word}, {}, {}, HelpItem::Unknown));

    setPriority(!m_helpToolTip.isEmpty() ? Priority_Tooltip : Priority_None);
}

void XMakeHoverHandler::operateTooltip(TextEditorWidget *editorWidget, const QPoint &point)
{
    if (!m_helpToolTip.isEmpty() && toolTip() != m_helpToolTip)
        ToolTip::show(point, m_helpToolTip, Qt::MarkdownText, editorWidget, m_contextHelp);
    else if (m_helpToolTip.isEmpty())
        ToolTip::hide();
    setToolTip(m_helpToolTip);
}

// XMakeEditorFactory

class XMakeEditorFactory final : public TextEditorFactory
{
public:
    XMakeEditorFactory()
    {
        setId(Constants::CMAKE_EDITOR_ID);
        setDisplayName(::Core::Tr::tr("XMake Editor"));
        addMimeType(Utils::Constants::CMAKE_MIMETYPE);
        addMimeType(Utils::Constants::CMAKE_PROJECT_MIMETYPE);

        setEditorCreator([] { return new XMakeEditor; });
        setEditorWidgetCreator([] { return new XMakeEditorWidget; });
        setDocumentCreator(createXMakeDocument);
        setIndenterCreator(createXMakeIndenter);
        setUseGenericHighlighter(true);
        setCommentDefinition(CommentDefinition::HashStyle);
        setCodeFoldingSupported(true);

        setCompletionAssistProvider(new XMakeFileCompletionAssistProvider);
        setAutoCompleterCreator([] { return new XMakeAutoCompleter; });

        setEditorActionHandlers(TextEditorActionHandler::UnCommentSelection
                                | TextEditorActionHandler::FollowSymbolUnderCursor
                                | TextEditorActionHandler::Format);

        addHoverHandler(new XMakeHoverHandler);

        ActionContainer *contextMenu = ActionManager::createMenu(Constants::M_CONTEXT);
        contextMenu->addAction(ActionManager::command(TextEditor::Constants::FOLLOW_SYMBOL_UNDER_CURSOR));
        contextMenu->addSeparator(Context(Constants::CMAKE_EDITOR_ID));
        contextMenu->addAction(ActionManager::command(TextEditor::Constants::UN_COMMENT_SELECTION));
    }
};

void setupXMakeEditor()
{
    static XMakeEditorFactory theXMakeEditorFactory;
}

} // XMakeProjectManager::Internal
