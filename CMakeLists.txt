cmake_minimum_required(VERSION 3.16)
project(QCAD_more VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5 5.12 REQUIRED COMPONENTS Core Gui Widgets)

add_library(qcad_more_core STATIC
    src/qcad/ENTITY.CPP src/qcad/LINE.CPP src/qcad/RECTANGLE.cpp
    src/qcad/Circle.cpp src/qcad/Arc.cpp src/qcad/ELLIPSE.cpp
    src/qcad/MPolygon.cpp src/qcad/TEXT.cpp src/qcad/MCommand.cpp
    src/qcad/QCADView.cpp src/qcad/MZoomPan.cpp src/qcad/MZoomWindow.cpp
    src/qcad/mainwindow.cpp src/qcad/mainwindow.h
    src/selection/SelectionService.cpp src/selection/SelectCommand.cpp
    src/blocks/BlockDefinitionService.cpp src/blocks/BlockEntity.cpp
    src/blocks/BlockInsertionService.cpp
    src/text/TextPropertyService.cpp src/text/TextPropertiesDialog.cpp
    src/storage/DocumentStorage.cpp src/storage/EntityCodec.cpp
    src/integration/QcadBlockActions.cpp src/integration/DrawEntityCommand.cpp
    src/integration/PointCommand.cpp
    src/editing/DocumentHistory.cpp src/editing/EditingActions.cpp
)
target_include_directories(qcad_more_core PUBLIC src src/qcad)
target_compile_definitions(qcad_more_core PUBLIC QT_PAINTER NOMINMAX)
target_link_libraries(qcad_more_core PUBLIC Qt5::Core Qt5::Gui Qt5::Widgets)
if(MSVC)
    target_compile_options(qcad_more_core PUBLIC /utf-8)
    target_compile_options(qcad_more_core PRIVATE /W3)
else()
    target_compile_options(qcad_more_core PRIVATE -Wall -Wextra)
endif()
add_executable(QCAD_more WIN32 src/qcad/main.cpp src/qcad/resources.qrc)
target_link_libraries(QCAD_more PRIVATE qcad_more_core)
if(MSVC)
    target_compile_options(QCAD_more PRIVATE /utf-8)
endif()

include(CTest)
if(BUILD_TESTING)
    find_package(Qt5 REQUIRED COMPONENTS Test)
    add_executable(block_workflow_tests tests/block_workflow_tests.cpp src/qcad/resources.qrc)
    target_link_libraries(block_workflow_tests PRIVATE qcad_more_core Qt5::Test)
    if(MSVC)
        target_compile_options(block_workflow_tests PRIVATE /utf-8)
    endif()
    add_test(NAME block_workflow COMMAND block_workflow_tests -o test-results.txt,txt)
    set_tests_properties(block_workflow PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    add_executable(editor_workflow_tests tests/editor_workflow_tests.cpp src/qcad/resources.qrc)
    target_link_libraries(editor_workflow_tests PRIVATE qcad_more_core Qt5::Test)
    target_compile_definitions(editor_workflow_tests PRIVATE QCAD_MORE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    if(MSVC)
        target_compile_options(editor_workflow_tests PRIVATE /utf-8)
    endif()
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/editor-offscreen")
    add_test(NAME editor_workflow COMMAND editor_workflow_tests -o test-results-editor.txt,txt)
    set_tests_properties(editor_workflow PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/editor-offscreen")
    if(WIN32)
        file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/native-tests")
        add_test(NAME block_workflow_windows COMMAND block_workflow_tests -o test-results-windows.txt,txt)
        set_tests_properties(block_workflow_windows PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=windows"
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/native-tests")
        file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/editor-windows")
        add_test(NAME editor_workflow_windows COMMAND editor_workflow_tests -o test-results-editor-windows.txt,txt)
        set_tests_properties(editor_workflow_windows PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=windows" WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/editor-windows")
    endif()
endif()
