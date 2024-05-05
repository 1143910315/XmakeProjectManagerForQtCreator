// Copyright (C) 2016 Jan Dalheimer <jan@dalheimer.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/textindenter.h>

namespace XMakeProjectManager::Internal {

TextEditor::Indenter *createXMakeIndenter(QTextDocument *doc);

} // XMakeProjectManager::Internal
