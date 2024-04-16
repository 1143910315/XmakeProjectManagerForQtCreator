cmake_minimum_required(VERSION 3.10)

list(APPEND CMAKE_PREFIX_PATH "./dev")

project(XmakeProjectManagerForQtCreator)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 17)

find_package(QtCreator REQUIRED COMPONENTS Core)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
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
    PLUGIN_CLASS MesonProjectPlugin
  DEPENDS QmlJS
  PLUGIN_DEPENDS QtCreator::Core QtCreator::CppEditor QtCreator::ProjectExplorer QtCreator::TextEditor QtCreator::QtSupport
  SOURCES
  arrayoptionlineedit.cpp
  arrayoptionlineedit.h
  buildoptions.h
  buildoptionsmodel.cpp
  buildoptionsmodel.h
  buildoptionsparser.h
  buildsystemfilesparser.h
  common.h
  infoparser.h
  kitdata.h
  mesonactionsmanager.cpp
  mesonactionsmanager.h
  mesonbuildconfiguration.cpp
  mesonbuildconfiguration.h
  mesonbuildsystem.cpp
  mesonbuildsystem.h
  mesoninfo.h
  mesoninfoparser.h
  mesonoutputparser.cpp
  mesonoutputparser.h
  mesonproject.cpp
  mesonproject.h
  mesonprojectimporter.cpp
  mesonprojectimporter.h
  mesonprojectmanagertr.h
  mesonprojectnodes.cpp
  mesonprojectnodes.h
  mesonprojectparser.cpp
  mesonprojectparser.h
  mesonprojectplugin.cpp
  mesonrunconfiguration.cpp
  mesonrunconfiguration.h
  mesontoolkitaspect.cpp
  mesontoolkitaspect.h
  mesontools.cpp
  mesontools.h
  mesonwrapper.cpp
  mesonwrapper.h
  ninjabuildstep.cpp
  ninjabuildstep.h
  ninjaparser.cpp
  ninjaparser.h
  ninjatoolkitaspect.cpp
  ninjatoolkitaspect.h
  ninjawrapper.h
  projecttree.cpp
  projecttree.h
  resources_meson.qrc
  settings.cpp
  settings.h
  target.h
  targetparser.h
  toolitemsettings.cpp
  toolitemsettings.h
  toolkitaspectwidget.cpp
  toolkitaspectwidget.h
  toolsmodel.cpp
  toolsmodel.h
  toolssettingsaccessor.cpp
  toolssettingsaccessor.h
  toolssettingspage.cpp
  toolssettingspage.h
  tooltreeitem.cpp
  tooltreeitem.h
  toolwrapper.cpp
  toolwrapper.h
  versionhelper.h
)
