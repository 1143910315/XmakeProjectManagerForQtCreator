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

set(TS_FILES XmakeProjectManagerForQtCreator_zh_CN.ts)



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
  DEPENDS
    ${QtX}::Widgets
    QtCreator::ExtensionSystem
    QtCreator::Utils
  SOURCES
    .github/workflows/build_cmake.yml
    .github/workflows/README.md
    README.md
    XmakeProjectManagerForQtCreator.cpp
    XmakeProjectManagerForQtCreator.h
    XmakeProjectManagerForQtCreator_global.h
    XmakeProjectManagerForQtCreatorconstants.h
    XmakeProjectManagerForQtCreatortr.h
    ${TS_FILES}
)

target_sources(XmakeProjectManagerForQtCreator
  PRIVATE
    settingPage/settings.cpp settingPage/settings.h
    XmakeProjectManagerForQtCreator.qrc
)

#find_package(Qt6 REQUIRED COMPONENTS LinguistTools)
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
