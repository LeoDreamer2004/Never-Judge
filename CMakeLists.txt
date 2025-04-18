cmake_minimum_required(VERSION 3.16)

project(NeverJudge VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
# NOTE: We use tree-sitter-c/cpp/python here
pkg_check_modules(TREE_SITTER REQUIRED tree-sitter)

find_package(Qt6 REQUIRED COMPONENTS Widgets Svg Network)
find_package(QTermWidget6 REQUIRED)
find_package(QCoro6 REQUIRED COMPONENTS Core Network)

set(PROJECT_SOURCES
        util/file.cpp
        util/script.cpp
        web/crawl.cpp
        web/parse.cpp
        res/resource.qrc
        ide/language.cpp
        ide/project.cpp
        ide/cmd.cpp
        ide/ide.cpp
        ide/highlighter.cpp
        ide/lsp.cpp
        widgets/setting.cpp
        widgets/icon.cpp
        widgets/footer.cpp
        widgets/iconNav.cpp
        widgets/preview.cpp
        widgets/code.cpp
        widgets/fileTree.cpp
        widgets/terminal.cpp
        widgets/menu.cpp
        widgets/window.cpp
        main.cpp
)
qt_add_executable(NeverJudge
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
)

target_include_directories(NeverJudge PRIVATE ${TREE_SITTER_INCLUDE_LIBRARIES})
target_link_libraries(NeverJudge PRIVATE
        qtermwidget6 Qt6::Widgets Qt6::Svg Qt6::Network QCoro6::Core QCoro6::Network
        ${TREE_SITTER_LIBRARIES})
set_target_properties(NeverJudge PROPERTIES
        ${BUNDLE_ID_OPTION}
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE TRUE
        WIN32_EXECUTABLE TRUE
)

qt_finalize_executable(NeverJudge)
