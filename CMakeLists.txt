cmake_minimum_required(VERSION 3.10)

list(APPEND CMAKE_PREFIX_PATH "./dev")

project(XmakeProjectManagerForQtCreator)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Zi")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")

## 添加库目录
#include_directories("./dev/qtcreator/lib/qtcreator/plugins")
#
## 获取目录下的所有库文件
#file(GLOB LIBRARIES "./dev/qtcreator/lib/qtcreator/plugins/*.lib")
#
## 提取文件名（不包括扩展名）
#foreach(LIB ${LIBRARIES})
#    get_filename_component(LIB_NAME ${LIB} NAME_WE)
#    message(STATUS "Library name: ${LIB_NAME}")
#    list(APPEND _arg_PRIVATE ${LIB_NAME})
#endforeach()

find_package(QtCreator REQUIRED COMPONENTS Core)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)

#find_package(Qt6 REQUIRED COMPONENTS LinguistTools)


set(QtX Qt${QT_VERSION_MAJOR})

# Add a XMake option that enables building your plugin with tests.
# You don't want your released plugin binaries to contain tests,
# so make that default to 'NO'.
# Enable tests by passing -DWITH_TESTS=ON to XMake.
option(WITH_TESTS "Builds with tests" NO)

if(WITH_TESTS)
  # Look for QtTest
  find_package(${QtX} REQUIRED COMPONENTS Test)

  # Tell XMake functions like add_qtc_plugin about the QtTest component.
  set(IMPLICIT_DEPENDS Qt::Test)

  # Enable ctest for auto tests.
  enable_testing()
endif()

add_qtc_plugin(XmakeProjectManagerForQtCreator
  PLUGIN_DEPENDS
    QtCreator::Core
    QtCreator::CppEditor
    QtCreator::ProjectExplorer
    QtCreator::QmlJSEditor
    QtCreator::QmlJSTools
    QtCreator::QtSupport
    QtCreator::TextEditor
  DEPENDS
    ${QtX}::Widgets
    QtCreator::ExtensionSystem
    QtCreator::Utils
  SOURCES
    .github/workflows/build_cmake.yml
    .github/workflows/README.md
    README.md
    builddirparameters.cpp builddirparameters.h
    xmake_global.h
    xmakeabstractprocessstep.cpp xmakeabstractprocessstep.h
    xmakeautocompleter.cpp xmakeautocompleter.h
    xmakebuildconfiguration.cpp xmakebuildconfiguration.h
    xmakebuildsystem.cpp xmakebuildsystem.h
    xmakebuildstep.cpp xmakebuildstep.h
    xmakebuildtarget.h
    xmakeconfigitem.cpp xmakeconfigitem.h
    xmakeeditor.cpp xmakeeditor.h
    xmakefilecompletionassist.cpp xmakefilecompletionassist.h
    xmakeformatter.cpp xmakeformatter.h
    xmakeindenter.cpp xmakeindenter.h
    xmakeinstallstep.cpp xmakeinstallstep.h
    xmakekitaspect.cpp xmakekitaspect.h
    xmakelocatorfilter.cpp xmakelocatorfilter.h
    xmakeparser.cpp xmakeparser.h
    xmakeprocess.cpp xmakeprocess.h
    xmakeproject.cpp xmakeproject.h
    xmakeproject.qrc
    xmakeprojectconstants.h
    xmakeprojectimporter.cpp xmakeprojectimporter.h
    xmakeprojectmanager.cpp xmakeprojectmanager.h
    xmakeprojectmanagertr.h
    xmakeprojectnodes.cpp xmakeprojectnodes.h
    xmakeprojectplugin.cpp
    xmakesettingspage.cpp xmakesettingspage.h
    xmakespecificsettings.cpp xmakespecificsettings.h
    xmaketool.cpp xmaketool.h
    xmaketoolmanager.cpp xmaketoolmanager.h
    xmaketoolsettingsaccessor.cpp xmaketoolsettingsaccessor.h
    configmodel.cpp configmodel.h
    configmodelitemdelegate.cpp configmodelitemdelegate.h
    fileapidataextractor.cpp fileapidataextractor.h
    fileapiparser.cpp fileapiparser.h
    fileapireader.cpp fileapireader.h
    presetsparser.cpp presetsparser.h
    presetsmacros.cpp presetsmacros.h
    projecttreehelper.cpp projecttreehelper.h
    3rdparty/xmake/cmListFileCache.cxx
    3rdparty/xmake/cmListFileLexer.cxx
    3rdparty/xmake/cmListFileCache.h
    3rdparty/rstparser/rstparser.cc 3rdparty/rstparser/rstparser.h
)

target_sources(XmakeProjectManagerForQtCreator
  PRIVATE
    xmakeproject.qrc
)


#if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
#
##    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
#    qt_add_lupdate(XmakeProjectManagerForQtCreator TS_FILES ${TS_FILES})
#    qt_add_lrelease(XmakeProjectManagerForQtCreator
#        TS_FILES ${TS_FILES}
#        QM_FILES_OUTPUT_VARIABLE QM_FILES)
#    qt_add_resources(XmakeProjectManagerForQtCreator "translations"
#        PREFIX "/i18n"
#        BASE "${CMAKE_CURRENT_BINARY_DIR}"
#        FILES "${qm_files}")
#else()
#
#    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
#endif()

