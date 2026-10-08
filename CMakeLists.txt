cmake_minimum_required(VERSION 3.16)
project(QCAD_more VERSION 0.1.0 LANGUAGES CXX)

# The course project uses Qt 5.12. This target builds only the new modules.
find_package(Qt5 5.12 REQUIRED COMPONENTS Core Gui)

add_library(qcad_more STATIC
    src/selection/SelectionService.cpp
    src/blocks/BlockDefinitionService.cpp
    src/blocks/BlockInsertionService.cpp
    src/text/TextPropertyService.cpp
    src/storage/DocumentStorage.cpp
    src/integration/QcadBlockActions.cpp
)
target_include_directories(qcad_more PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_features(qcad_more PUBLIC cxx_std_14)
target_link_libraries(qcad_more PUBLIC Qt5::Core Qt5::Gui)
if(MSVC)
    target_compile_options(qcad_more PRIVATE /W4 /utf-8)
else()
    target_compile_options(qcad_more PRIVATE -Wall -Wextra -Wpedantic)
endif()
