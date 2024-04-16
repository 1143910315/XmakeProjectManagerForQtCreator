cmake_minimum_required(VERSION 3.18)

project(XMakeProjectManager VERSION 12.0.2
                            DESCRIPTION "QtCreator XMake project support"
                            LANGUAGES CXX)

list(APPEND CMAKE_PREFIX_PATH "${QTC_BUILD_PATH}")

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

file(GLOB_RECURSE
    source_files
    src/*.cpp
    src/*.hpp
    src/*.inl
)

file(GLOB_RECURSE
    ui_files
    src/*.ui
)

file(GLOB_RECURSE
    assets_files
    assets/*.lua
)


add_qtc_plugin(
    ${PROJECT_NAME}
    PLUGIN_DEPENDS
        QtCreator::Core
        QtCreator::CppEditor
        QtCreator::ProjectExplorer
        QtCreator::QtSupport
    PLUGIN_RECOMMENDS
        QtCreator::Designer
        QtCreator::LangageClient
    DEPENDS
        Qt6::Core
        Qt6::Widgets
        QtCreator::Core
        QtCreator::ExtensionSystem
        QtCreator::Utils
        QtCreator::QmlJS
    SOURCES
        ${source_files}

        ${ui_files}

        ${assets_files}

        resources.qrc
)

#target_compile_options(${PROJECT_NAME} PUBLIC "/fsanitize=address")
target_include_directories(${PROJECT_NAME} PRIVATE src/)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
