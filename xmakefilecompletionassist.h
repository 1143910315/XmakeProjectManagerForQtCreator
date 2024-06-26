// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/codeassist/completionassistprovider.h>

namespace XMakeProjectManager::Internal {
    class XMakeFileCompletionAssistProvider : public TextEditor::CompletionAssistProvider {
        Q_OBJECT
public:
        TextEditor::IAssistProcessor *createProcessor(const TextEditor::AssistInterface *) const final;
        int activationCharSequenceLength() const final;
        bool isActivationCharSequence(const QString &sequence) const final;
    };
} // XMakeProjectManager::Internal
