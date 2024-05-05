import qbs 1.0

QtcPlugin {
    name: "XMakeProjectManager"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "CppEditor" }
    Depends { name: "QmlJS" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }
    Depends { name: "QtSupport" }

    files: [
        "builddirparameters.cpp",
        "builddirparameters.h",
        "xmake_global.h",
        "xmakeabstractprocessstep.cpp",
        "xmakeabstractprocessstep.h",
        "xmakebuildconfiguration.cpp",
        "xmakebuildconfiguration.h",
        "xmakebuildstep.cpp",
        "xmakebuildstep.h",
        "xmakebuildsystem.cpp",
        "xmakebuildsystem.h",
        "xmakebuildtarget.h",
        "xmakeconfigitem.cpp",
        "xmakeconfigitem.h",
        "xmakeeditor.cpp",
        "xmakeeditor.h",
        "xmakefilecompletionassist.cpp",
        "xmakefilecompletionassist.h",
        "xmakeformatter.cpp",
        "xmakeformatter.h",
        "xmakeinstallstep.cpp",
        "xmakeinstallstep.h",
        "xmakekitaspect.h",
        "xmakekitaspect.cpp",
        "xmakelocatorfilter.cpp",
        "xmakelocatorfilter.h",
        "xmakeparser.cpp",
        "xmakeparser.h",
        "xmakeprocess.cpp",
        "xmakeprocess.h",
        "xmakeproject.cpp",
        "xmakeproject.h",
        "xmakeproject.qrc",
        "xmakeprojectimporter.cpp",
        "xmakeprojectimporter.h",
        "xmakeprojectconstants.h",
        "xmakeprojectmanager.cpp",
        "xmakeprojectmanager.h",
        "xmakeprojectmanagertr.h",
        "xmakeprojectnodes.cpp",
        "xmakeprojectnodes.h",
        "xmakeprojectplugin.cpp",
        "xmaketool.cpp",
        "xmaketool.h",
        "xmaketoolmanager.cpp",
        "xmaketoolmanager.h",
        "xmaketoolsettingsaccessor.cpp",
        "xmaketoolsettingsaccessor.h",
        "xmakesettingspage.h",
        "xmakesettingspage.cpp",
        "xmakeindenter.h",
        "xmakeindenter.cpp",
        "xmakeautocompleter.h",
        "xmakeautocompleter.cpp",
        "xmakespecificsettings.h",
        "xmakespecificsettings.cpp",
        "configmodel.cpp",
        "configmodel.h",
        "configmodelitemdelegate.cpp",
        "configmodelitemdelegate.h",
        "fileapidataextractor.cpp",
        "fileapidataextractor.h",
        "fileapiparser.cpp",
        "fileapiparser.h",
        "fileapireader.cpp",
        "fileapireader.h",
        "presetsparser.cpp",
        "presetsparser.h",
        "presetsmacros.cpp",
        "presetsmacros.h",
        "projecttreehelper.cpp",
        "projecttreehelper.h"
    ]

    Group {
        name: "3rdparty"
        cpp.includePaths: base.concat("3rdparty/xmake")

        prefix: "3rdparty/"
        files: [
            "xmake/cmListFileCache.cxx",
            "xmake/cmListFileCache.h",
            "xmake/cmListFileLexer.cxx",
            "xmake/cmListFileLexer.h",
            "xmake/cmStandardLexer.h",
            "rstparser/rstparser.cc",
            "rstparser/rstparser.h"
        ]
    }
}
