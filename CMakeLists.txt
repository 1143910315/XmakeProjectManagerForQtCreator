cmake_minimum_required(VERSION 3.10)

list(APPEND CMAKE_PREFIX_PATH "./dev")

project(XmakeProjectManagerForQtCreator)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 17)


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

# Add a CMake option that enables building your plugin with tests.
# You don't want your released plugin binaries to contain tests,
# so make that default to 'NO'.
# Enable tests by passing -DWITH_TESTS=ON to CMake.
option(WITH_TESTS "Builds with tests" NO)

if(WITH_TESTS)
  # Look for QtTest
  find_package(${QtX} REQUIRED COMPONENTS Test)

  # Tell CMake functions like add_qtc_plugin about the QtTest component.
  set(IMPLICIT_DEPENDS Qt::Test)

  # Enable ctest for auto tests.
  enable_testing()
endif()

add_qtc_plugin(XmakeProjectManagerForQtCreator
  PLUGIN_DEPENDS
    QtCreator::Core
    QtCreator::CppEditor
    QtCreator::ProjectExplorer
    QtCreator::TextEditor
    QtCreator::QtSupport
  DEPENDS
    ${QtX}::Widgets
    QtCreator::ExtensionSystem
    QtCreator::Utils
  SOURCES
    .github/workflows/build_cmake.yml
    .github/workflows/README.md
    README.md
    builddirparameters.cpp builddirparameters.h
    cmake_global.h
    cmakeabstractprocessstep.cpp cmakeabstractprocessstep.h
    cmakeautocompleter.cpp cmakeautocompleter.h
    cmakebuildconfiguration.cpp cmakebuildconfiguration.h
    cmakebuildsystem.cpp cmakebuildsystem.h
    cmakebuildstep.cpp cmakebuildstep.h
    cmakebuildtarget.h
    cmakeconfigitem.cpp cmakeconfigitem.h
    cmakeeditor.cpp cmakeeditor.h
    cmakefilecompletionassist.cpp cmakefilecompletionassist.h
    cmakeformatter.cpp cmakeformatter.h
    cmakeindenter.cpp cmakeindenter.h
    cmakeinstallstep.cpp cmakeinstallstep.h
    cmakekitaspect.cpp cmakekitaspect.h
    cmakelocatorfilter.cpp cmakelocatorfilter.h
    cmakeparser.cpp cmakeparser.h
    cmakeprocess.cpp cmakeprocess.h
    cmakeproject.cpp cmakeproject.h
    cmakeproject.qrc
    cmakeprojectconstants.h
    cmakeprojectimporter.cpp cmakeprojectimporter.h
    cmakeprojectmanager.cpp cmakeprojectmanager.h
    cmakeprojectmanagertr.h
    cmakeprojectnodes.cpp cmakeprojectnodes.h
    cmakeprojectplugin.cpp
    cmakesettingspage.cpp cmakesettingspage.h
    cmakespecificsettings.cpp cmakespecificsettings.h
    cmaketool.cpp cmaketool.h
    cmaketoolmanager.cpp cmaketoolmanager.h
    cmaketoolsettingsaccessor.cpp cmaketoolsettingsaccessor.h
    configmodel.cpp configmodel.h
    configmodelitemdelegate.cpp configmodelitemdelegate.h
    fileapidataextractor.cpp fileapidataextractor.h
    fileapiparser.cpp fileapiparser.h
    fileapireader.cpp fileapireader.h
    presetsparser.cpp presetsparser.h
    presetsmacros.cpp presetsmacros.h
    projecttreehelper.cpp projecttreehelper.h
    3rdparty/cmake/cmListFileCache.cxx
    3rdparty/cmake/cmListFileLexer.cxx
    3rdparty/cmake/cmListFileCache.h
    3rdparty/rstparser/rstparser.cc 3rdparty/rstparser/rstparser.h
)

target_sources(XmakeProjectManagerForQtCreator
  PRIVATE
    cmakeproject.qrc
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

