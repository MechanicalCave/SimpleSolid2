add_executable(ph01_project_workspace_metadata_test
    project_workspace_metadata_test.cpp
)

target_link_libraries(ph01_project_workspace_metadata_test
    PRIVATE
        simplesolid2_application
)

add_test(
    NAME ph01.project_workspace_metadata
    COMMAND ph01_project_workspace_metadata_test
)

add_executable(ph01_project_session_test
    project_session_test.cpp
)

target_link_libraries(ph01_project_session_test
    PRIVATE
        simplesolid2_application
)

add_test(
    NAME ph01.project_session
    COMMAND ph01_project_session_test
)

add_executable(ph01_recent_project_store_test
    recent_project_store_test.cpp
)

target_link_libraries(ph01_recent_project_store_test
    PRIVATE
        simplesolid2_application
)

add_test(
    NAME ph01.recent_project_store
    COMMAND ph01_recent_project_store_test
)

add_executable(ph01_project_hub_lifecycle_test
    project_hub_lifecycle_test.cpp
)

target_include_directories(ph01_project_hub_lifecycle_test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/application
)

target_link_libraries(ph01_project_hub_lifecycle_test
    PRIVATE
        simplesolid2_application
)

add_test(
    NAME ph01.project_hub_lifecycle
    COMMAND ph01_project_hub_lifecycle_test
)

add_test(
    NAME ph01.app_smoke
    COMMAND simplesolid2 --smoke-test
)

set_tests_properties(ph01.app_smoke PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

find_package(Qt6 6 REQUIRED COMPONENTS Widgets Test)

add_executable(ph02a_project_hub_selection_ux_test
    project_hub_selection_ux_test.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/cad_workbench.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/cad_workbench_shell.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/part_document_tree_controller.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/project_creation_dialog.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/project_hub_window.cpp
)

target_include_directories(ph02a_project_hub_selection_ux_test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/application
        ${PROJECT_SOURCE_DIR}/src/ui
)

target_link_libraries(ph02a_project_hub_selection_ux_test
    PRIVATE
        simplesolid2_application
        Qt6::Widgets
)

target_compile_features(ph02a_project_hub_selection_ux_test PRIVATE cxx_std_20)

add_test(
    NAME ph02a.project_hub_selection_ux
    COMMAND ph02a_project_hub_selection_ux_test
)

set_tests_properties(ph02a.project_hub_selection_ux PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

add_executable(ph02b_project_hub_recent_availability_test
    project_hub_recent_availability_test.cpp
)

target_include_directories(ph02b_project_hub_recent_availability_test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/application
)

target_link_libraries(ph02b_project_hub_recent_availability_test
    PRIVATE
        simplesolid2_application
)

target_compile_features(ph02b_project_hub_recent_availability_test PRIVATE cxx_std_20)

add_test(
    NAME ph02b.project_hub_recent_availability
    COMMAND ph02b_project_hub_recent_availability_test
)

add_executable(ph02b_project_hub_availability_ux_test
    project_hub_availability_ux_test.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/cad_workbench.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/cad_workbench_shell.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/part_document_tree_controller.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/project_creation_dialog.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/project_hub_window.cpp
)

target_include_directories(ph02b_project_hub_availability_ux_test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/application
        ${PROJECT_SOURCE_DIR}/src/ui
)

target_link_libraries(ph02b_project_hub_availability_ux_test
    PRIVATE
        simplesolid2_application
        Qt6::Widgets
)

target_compile_features(ph02b_project_hub_availability_ux_test PRIVATE cxx_std_20)

add_test(
    NAME ph02b.project_hub_availability_ux
    COMMAND ph02b_project_hub_availability_ux_test
)

set_tests_properties(ph02b.project_hub_availability_ux PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

add_executable(ph02c_project_creation_workflow_test
    project_creation_workflow_test.cpp
)

target_include_directories(ph02c_project_creation_workflow_test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/application
)

target_link_libraries(ph02c_project_creation_workflow_test
    PRIVATE
        simplesolid2_application
)

target_compile_features(ph02c_project_creation_workflow_test PRIVATE cxx_std_20)

add_test(
    NAME ph02c.project_creation_workflow
    COMMAND ph02c_project_creation_workflow_test
)

add_executable(ph02c_project_creation_dialog_test
    project_creation_dialog_test.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/project_creation_dialog.cpp
)

target_include_directories(ph02c_project_creation_dialog_test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/ui
)

target_link_libraries(ph02c_project_creation_dialog_test
    PRIVATE
        Qt6::Widgets
        Qt6::Test
)

target_compile_features(ph02c_project_creation_dialog_test PRIVATE cxx_std_20)

add_test(
    NAME ph02c.project_creation_dialog
    COMMAND ph02c_project_creation_dialog_test
)

set_tests_properties(ph02c.project_creation_dialog PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)


add_executable(part01_document_primitives_test
    document_primitives_test.cpp
)

target_link_libraries(part01_document_primitives_test
    PRIVATE
        simplesolid2_core
)

add_test(
    NAME part01.document_primitives
    COMMAND part01_document_primitives_test
)

add_executable(part01_part_document_transaction_test
    part_document_transaction_test.cpp
)

target_link_libraries(part01_part_document_transaction_test
    PRIVATE
        simplesolid2_part
)

add_test(
    NAME part01.part_document_transaction
    COMMAND part01_part_document_transaction_test
)

add_executable(part01_part_document_store_test
    part_document_store_test.cpp
)

target_link_libraries(part01_part_document_store_test
    PRIVATE
        simplesolid2_part
)

add_test(
    NAME part01.part_document_store
    COMMAND part01_part_document_store_test
)

add_executable(part01_document_session_test
    document_session_test.cpp
)

target_link_libraries(part01_document_session_test
    PRIVATE
        simplesolid2_application
)

add_test(
    NAME part01.document_session
    COMMAND part01_document_session_test
)

add_executable(part01_document_workspace_test
    document_workspace_test.cpp
)

target_link_libraries(part01_document_workspace_test
    PRIVATE
        simplesolid2_application
)

add_test(
    NAME part01.document_workspace
    COMMAND part01_document_workspace_test
)

add_executable(part01_project_document_lifecycle_test
    project_document_lifecycle_test.cpp
)

target_link_libraries(part01_project_document_lifecycle_test
    PRIVATE
        simplesolid2_application
)

add_test(
    NAME part01.project_document_lifecycle
    COMMAND part01_project_document_lifecycle_test
)


add_executable(part01_project_dirty_close_test
    project_dirty_close_test.cpp
)

target_include_directories(part01_project_dirty_close_test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/application
)

target_link_libraries(part01_project_dirty_close_test
    PRIVATE
        simplesolid2_application
)

target_compile_features(part01_project_dirty_close_test PRIVATE cxx_std_20)

add_test(
    NAME part01.project_dirty_close
    COMMAND part01_project_dirty_close_test
)

add_executable(wb01_cad_workbench_test
    cad_workbench_test.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/cad_workbench.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/cad_workbench_shell.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/part_document_tree_controller.cpp
)

target_include_directories(wb01_cad_workbench_test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/ui
)

target_link_libraries(wb01_cad_workbench_test
    PRIVATE
        simplesolid2_application
        Qt6::Widgets
)

target_compile_features(wb01_cad_workbench_test PRIVATE cxx_std_20)

add_test(
    NAME wb01.cad_workbench
    COMMAND wb01_cad_workbench_test
)

set_tests_properties(wb01.cad_workbench PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)


add_executable(wb01_viewer_camera_state_test
    viewer_camera_state_test.cpp
)
target_link_libraries(wb01_viewer_camera_state_test PRIVATE simplesolid2_viewer)
add_test(NAME wb01.viewer_camera_state COMMAND wb01_viewer_camera_state_test)

add_executable(wb01_viewer_navigation_test
    viewer_navigation_test.cpp
)
target_link_libraries(wb01_viewer_navigation_test PRIVATE simplesolid2_viewer)
add_test(NAME wb01.viewer_navigation COMMAND wb01_viewer_navigation_test)

add_executable(wb01_viewer_reference_presentation_test
    viewer_reference_presentation_test.cpp
)
target_link_libraries(wb01_viewer_reference_presentation_test PRIVATE simplesolid2_viewer)
add_test(NAME wb01.viewer_reference_presentation COMMAND wb01_viewer_reference_presentation_test)

add_executable(wb01_viewer_navigation_mapping_test
    viewer_navigation_mapping_test.cpp
    ${PROJECT_SOURCE_DIR}/src/viewer_qt_occt/navigation_mapping.cpp
)
target_include_directories(wb01_viewer_navigation_mapping_test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/viewer_qt_occt
)
target_compile_features(wb01_viewer_navigation_mapping_test PRIVATE cxx_std_20)
add_test(NAME wb01.viewer_navigation_mapping COMMAND wb01_viewer_navigation_mapping_test)

add_test(
    NAME wb01.viewer_boundaries
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_ROOT=${PROJECT_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_viewer_boundaries.cmake
)


add_executable(wb01_builtin_reference_test
    builtin_reference_test.cpp
)
target_link_libraries(wb01_builtin_reference_test PRIVATE simplesolid2_core)
add_test(NAME wb01.builtin_reference COMMAND wb01_builtin_reference_test)


add_test(
    NAME wb01.workbench_shell_boundaries
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_ROOT=${PROJECT_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_workbench_shell_boundaries.cmake
)
