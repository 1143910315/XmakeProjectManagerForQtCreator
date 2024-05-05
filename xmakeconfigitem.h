// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "xmake_global.h"

#include <QByteArray>
#include <QObject>
#include <QStringList>

#include <optional>

namespace Utils {
class FilePath;
class MacroExpander;
} // namespace Utils

namespace ProjectExplorer {
class Kit;
}

namespace XMakeProjectManager {

class XMAKE_EXPORT XMakeConfigItem
{
public:
    enum Type { FILEPATH, PATH, BOOL, STRING, INTERNAL, STATIC, UNINITIALIZED };
    XMakeConfigItem();
    XMakeConfigItem(const QByteArray &k, Type t, const QByteArray &d, const QByteArray &v, const QStringList &s = {});
    XMakeConfigItem(const QByteArray &k, Type t, const QByteArray &v);
    XMakeConfigItem(const QByteArray &k, const QByteArray &v);

    static QStringList xmakeSplitValue(const QString &in, bool keepEmpty = false);
    static Type typeStringToType(const QByteArray &typeString);
    static QString typeToTypeString(const Type t);
    static std::optional<bool> toBool(const QString &value);
    bool isNull() const { return key.isEmpty(); }

    QString expandedValue(const ProjectExplorer::Kit *k) const;
    QString expandedValue(const Utils::MacroExpander *expander) const;

    static bool less(const XMakeConfigItem &a, const XMakeConfigItem &b);
    static XMakeConfigItem fromString(const QString &s);
    QString toString(const Utils::MacroExpander *expander = nullptr) const;
    QString toArgument() const;
    QString toArgument(const Utils::MacroExpander *expander) const;
    QString toXMakeSetLine(const Utils::MacroExpander *expander = nullptr) const;

    bool operator==(const XMakeConfigItem &o) const;
    friend size_t qHash(const XMakeConfigItem &it);  // needed for MSVC

    QByteArray key;
    Type type = STRING;
    bool isAdvanced = false;
    bool inXMakeCache = false;
    bool isUnset = false;
    bool isInitial = false;
    QByteArray value; // converted to string as needed
    QByteArray documentation;
    QStringList values;
};

class XMAKE_EXPORT XMakeConfig : public QList<XMakeConfigItem>
{
public:
    XMakeConfig() = default;
    XMakeConfig(const QList<XMakeConfigItem> &items) : QList<XMakeConfigItem>(items) {}
    XMakeConfig(std::initializer_list<XMakeConfigItem> items) : QList<XMakeConfigItem>(items) {}

    const QList<XMakeConfigItem> &toList() const { return *this; }

    static XMakeConfig fromArguments(const QStringList &list, QStringList &unknownOptions);
    static XMakeConfig fromFile(const Utils::FilePath &input, QString *errorMessage);

    QByteArray valueOf(const QByteArray &key) const;
    QString stringValueOf(const QByteArray &key) const;
    Utils::FilePath filePathValueOf(const QByteArray &key) const;
    QString expandedValueOf(const ProjectExplorer::Kit *k, const QByteArray &key) const;
};

#ifdef WITH_TESTS
namespace Internal { QObject *createXMakeConfigTest(); }
#endif

} // namespace XMakeProjectManager
