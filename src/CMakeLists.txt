add_library(simplesolid2_core STATIC
    core/document.cpp
)

target_include_directories(simplesolid2_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/core/include
)

target_compile_features(simplesolid2_core PUBLIC cxx_std_20)

add_library(simplesolid2_persistence STATIC
    persistence/atomic_file.cpp
)

target_include_directories(simplesolid2_persistence
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/persistence/include
)

target_compile_features(simplesolid2_persistence PUBLIC cxx_std_20)

add_library(simplesolid2_part STATIC
    part/part_document.cpp
    part/part_document_store.cpp
)

target_include_directories(simplesolid2_part
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/part/include
)

target_link_libraries(simplesolid2_part
    PUBLIC
        simplesolid2_core
        simplesolid2_persistence
)

target_compile_features(simplesolid2_part PUBLIC cxx_std_20)

add_library(simplesolid2_application STATIC
    application/document_session.cpp
    application/document_workspace.cpp
    application/project_hub_controller.cpp
    application/recent_project_store.cpp
    application/project_session.cpp
    application/project_workspace_metadata.cpp
)

target_include_directories(simplesolid2_application
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/application/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/application
)

target_link_libraries(simplesolid2_application
    PUBLIC
        simplesolid2_part
)

target_compile_features(simplesolid2_application PUBLIC cxx_std_20)

find_package(Qt6 6 REQUIRED COMPONENTS Widgets)
find_package(OpenCASCADE CONFIG REQUIRED)

foreach(_target IN ITEMS TKernel TKMath TKG3d TKGeomBase TKService TKV3d TKOpenGl)
    if(NOT TARGET ${_target})
        message(FATAL_ERROR "OpenCASCADE package is missing required Viewer target ${_target}")
    endif()
endforeach()

add_library(simplesolid2_viewer STATIC
    viewer/camera_state.cpp
    viewer/navigation.cpp
)

target_include_directories(simplesolid2_viewer
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/viewer/include
)

target_compile_features(simplesolid2_viewer PUBLIC cxx_std_20)

if(NOT WIN32)
    message(FATAL_ERROR "WB-01 native Qt/OCCT Viewer provider is currently supported on Windows")
endif()

add_library(simplesolid2_viewer_qt_occt STATIC
    viewer_qt_occt/navigation_mapping.cpp
    viewer_qt_occt/qt_occt_viewer_widget.cpp
)

target_include_directories(simplesolid2_viewer_qt_occt
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/viewer_qt_occt/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/viewer_qt_occt
)

target_link_libraries(simplesolid2_viewer_qt_occt
    PUBLIC
        simplesolid2_viewer
        Qt6::Widgets
    PRIVATE
        TKernel
        TKMath
        TKG3d
        TKGeomBase
        TKService
        TKV3d
        TKOpenGl
)

target_compile_features(simplesolid2_viewer_qt_occt PUBLIC cxx_std_20)

add_executable(simplesolid2
    app/main.cpp
    ui/cad_workbench.cpp
    ui/project_creation_dialog.cpp
    ui/project_hub_window.cpp
)

target_include_directories(simplesolid2
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/application
        ${CMAKE_CURRENT_SOURCE_DIR}/ui
)

target_link_libraries(simplesolid2
    PRIVATE
        simplesolid2_application
        Qt6::Widgets
)

target_compile_features(simplesolid2 PRIVATE cxx_std_20)

set_target_properties(simplesolid2 PROPERTIES
    OUTPUT_NAME "SimpleSolid2"
)
