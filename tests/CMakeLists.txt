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
    ${PROJECT_SOURCE_DIR}/src/ui/open_document_dialog.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/workspace_location_dialog.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/cad_workbench_shell.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/part_document_tree_controller.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/part_sketch_interaction_controller.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/part_viewport_controller.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/project_creation_dialog.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/project_hub_window.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/project_workspace_shell.cpp
)

target_include_directories(ph02a_project_hub_selection_ux_test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/application
        ${PROJECT_SOURCE_DIR}/src/ui
)

target_link_libraries(ph02a_project_hub_selection_ux_test
    PRIVATE
        simplesolid2_application
        simplesolid2_viewer
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
    ${PROJECT_SOURCE_DIR}/src/ui/open_document_dialog.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/workspace_location_dialog.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/cad_workbench_shell.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/part_document_tree_controller.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/part_sketch_interaction_controller.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/part_viewport_controller.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/project_creation_dialog.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/project_hub_window.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/project_workspace_shell.cpp
)

target_include_directories(ph02b_project_hub_availability_ux_test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/application
        ${PROJECT_SOURCE_DIR}/src/ui
)

target_link_libraries(ph02b_project_hub_availability_ux_test
    PRIVATE
        simplesolid2_application
        simplesolid2_viewer
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

add_executable(persist01_native_document_container_test
    native_document_container_test.cpp
)

target_include_directories(persist01_native_document_container_test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/third_party/miniz-3.1.2
)

target_link_libraries(persist01_native_document_container_test
    PRIVATE
        simplesolid2_persistence
        simplesolid2_miniz
)

target_compile_features(persist01_native_document_container_test PRIVATE cxx_std_20)

add_test(
    NAME persist01.native_document_container
    COMMAND persist01_native_document_container_test
)

add_executable(sk01_part_sketch_host_test
    sk01_part_sketch_host_test.cpp
)

target_link_libraries(sk01_part_sketch_host_test
    PRIVATE
        simplesolid2_application
)

target_compile_features(sk01_part_sketch_host_test PRIVATE cxx_std_20)

add_test(
    NAME sk01.part_sketch_host
    COMMAND sk01_part_sketch_host_test
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
    ${PROJECT_SOURCE_DIR}/src/ui/open_document_dialog.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/workspace_location_dialog.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/cad_workbench_shell.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/part_document_tree_controller.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/part_sketch_interaction_controller.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/part_viewport_controller.cpp
)

target_include_directories(wb01_cad_workbench_test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/ui
)

target_link_libraries(wb01_cad_workbench_test
    PRIVATE
        simplesolid2_application
        simplesolid2_viewer
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


add_test(
    NAME ws01.project_workspace_boundaries
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_ROOT=${PROJECT_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_project_workspace_boundaries.cmake
)


add_executable(wb01_viewer_selection_test
    viewer_selection_test.cpp
)
target_link_libraries(wb01_viewer_selection_test PRIVATE simplesolid2_viewer)
add_test(NAME wb01.viewer_selection COMMAND wb01_viewer_selection_test)


add_executable(wb01_viewer_native_smoke_test
    viewer_native_smoke_test.cpp
)
target_link_libraries(wb01_viewer_native_smoke_test
    PRIVATE
        simplesolid2_viewer_qt_occt
        Qt6::Widgets
)
target_compile_features(wb01_viewer_native_smoke_test PRIVATE cxx_std_20)
add_test(
    NAME wb01.viewer_native_smoke
    COMMAND wb01_viewer_native_smoke_test
)
set_tests_properties(wb01.viewer_native_smoke PROPERTIES
    TIMEOUT 20
)


add_executable(wb01a_workbench_native_stress_test
    workbench_native_stress_test.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/cad_workbench.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/open_document_dialog.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/workspace_location_dialog.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/cad_workbench_shell.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/part_document_tree_controller.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/part_sketch_interaction_controller.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/part_viewport_controller.cpp
)
target_include_directories(wb01a_workbench_native_stress_test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/ui
)
target_link_libraries(wb01a_workbench_native_stress_test
    PRIVATE
        simplesolid2_application
        simplesolid2_viewer
        simplesolid2_viewer_qt_occt
        Qt6::Widgets
        Qt6::Test
)
target_compile_features(wb01a_workbench_native_stress_test PRIVATE cxx_std_20)
add_test(
    NAME wb01a.workbench_native_stress
    COMMAND wb01a_workbench_native_stress_test
)
set_tests_properties(wb01a.workbench_native_stress PROPERTIES
    TIMEOUT 120
)


add_executable(wb01a_workspace_directory_test
    workspace_directory_test.cpp
)
target_link_libraries(wb01a_workspace_directory_test
    PRIVATE
        simplesolid2_application
)
target_compile_features(wb01a_workspace_directory_test PRIVATE cxx_std_20)
add_test(
    NAME wb01a.workspace_directory
    COMMAND wb01a_workspace_directory_test
)


add_executable(wb01a_document_dialog_ux_test
    document_dialog_ux_test.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/open_document_dialog.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/workspace_location_dialog.cpp
)
target_include_directories(wb01a_document_dialog_ux_test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/ui
)
target_link_libraries(wb01a_document_dialog_ux_test
    PRIVATE
        simplesolid2_application
        Qt6::Widgets
        Qt6::Test
)
target_compile_features(wb01a_document_dialog_ux_test PRIVATE cxx_std_20)
add_test(
    NAME wb01a.document_dialog_ux
    COMMAND wb01a_document_dialog_ux_test
)
set_tests_properties(wb01a.document_dialog_ux PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)


add_executable(wb01a_workbench_document_dialog_integration_test
    workbench_document_dialog_integration_test.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/cad_workbench.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/cad_workbench_shell.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/open_document_dialog.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/part_document_tree_controller.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/part_sketch_interaction_controller.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/part_viewport_controller.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/project_creation_dialog.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/project_hub_window.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/project_workspace_shell.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/workspace_location_dialog.cpp
)
target_include_directories(wb01a_workbench_document_dialog_integration_test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/application
        ${PROJECT_SOURCE_DIR}/src/ui
)
target_link_libraries(wb01a_workbench_document_dialog_integration_test
    PRIVATE
        simplesolid2_application
        simplesolid2_viewer
        Qt6::Widgets
        Qt6::Test
)
target_compile_features(wb01a_workbench_document_dialog_integration_test PRIVATE cxx_std_20)
add_test(
    NAME wb01a.workbench_document_dialog_integration
    COMMAND wb01a_workbench_document_dialog_integration_test
)
set_tests_properties(wb01a.workbench_document_dialog_integration PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 30
)


add_executable(sk01_workbench_sketch_host_test
    sk01_workbench_sketch_host_test.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/cad_workbench.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/cad_workbench_shell.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/open_document_dialog.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/part_document_tree_controller.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/part_sketch_interaction_controller.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/part_viewport_controller.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/workspace_location_dialog.cpp
)

target_include_directories(sk01_workbench_sketch_host_test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/application
        ${PROJECT_SOURCE_DIR}/src/ui
)

target_link_libraries(sk01_workbench_sketch_host_test
    PRIVATE
        simplesolid2_application
        simplesolid2_viewer
        Qt6::Widgets
        Qt6::Test
)

target_compile_features(sk01_workbench_sketch_host_test PRIVATE cxx_std_20)

add_test(
    NAME sk01.workbench_sketch_host
    COMMAND sk01_workbench_sketch_host_test
)

set_tests_properties(sk01.workbench_sketch_host PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 30
)


add_executable(sk02a_shared_2d_core_test
    sk02a_shared_2d_core_test.cpp
)

target_link_libraries(sk02a_shared_2d_core_test
    PRIVATE
        simplesolid2_sketch
)

target_compile_features(sk02a_shared_2d_core_test PRIVATE cxx_std_20)

add_test(
    NAME sk02a.shared_2d_core
    COMMAND sk02a_shared_2d_core_test
)

add_test(
    NAME sk02a.shared_2d_boundaries
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_ROOT=${PROJECT_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_sketch_boundaries.cmake
)


add_executable(sk02b_part_sketch_model_test
    sk02b_part_sketch_model_test.cpp
)

target_link_libraries(sk02b_part_sketch_model_test
    PRIVATE
        simplesolid2_part
)

target_compile_features(sk02b_part_sketch_model_test PRIVATE cxx_std_20)

add_test(
    NAME sk02b.part_sketch_model
    COMMAND sk02b_part_sketch_model_test
)


add_executable(sk02b_sketch_entity_lifecycle_test
    sk02b_sketch_entity_lifecycle_test.cpp
)

target_link_libraries(sk02b_sketch_entity_lifecycle_test
    PRIVATE
        simplesolid2_application
)

target_compile_features(sk02b_sketch_entity_lifecycle_test PRIVATE cxx_std_20)

add_test(
    NAME sk02b.sketch_entity_lifecycle
    COMMAND sk02b_sketch_entity_lifecycle_test
)


add_executable(sk02b_part_sketch_persistence_test
    sk02b_part_sketch_persistence_test.cpp
)

target_link_libraries(sk02b_part_sketch_persistence_test
    PRIVATE
        simplesolid2_application
)

target_compile_features(sk02b_part_sketch_persistence_test PRIVATE cxx_std_20)

add_test(
    NAME sk02b.part_sketch_persistence
    COMMAND sk02b_part_sketch_persistence_test
)


add_executable(sk03a_viewer_sketch_contracts_test
    sk03a_viewer_sketch_contracts_test.cpp
)

target_link_libraries(sk03a_viewer_sketch_contracts_test
    PRIVATE
        simplesolid2_viewer
)

target_compile_features(sk03a_viewer_sketch_contracts_test PRIVATE cxx_std_20)

add_test(
    NAME sk03a.viewer_sketch_contracts
    COMMAND sk03a_viewer_sketch_contracts_test
)


add_executable(sk03a_sketch_viewport_mapping_test
    sk03a_sketch_viewport_mapping_test.cpp
)

target_include_directories(sk03a_sketch_viewport_mapping_test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/ui
)

target_link_libraries(sk03a_sketch_viewport_mapping_test
    PRIVATE
        simplesolid2_part
        simplesolid2_viewer
)

target_compile_features(sk03a_sketch_viewport_mapping_test PRIVATE cxx_std_20)

add_test(
    NAME sk03a.sketch_viewport_mapping
    COMMAND sk03a_sketch_viewport_mapping_test
)


add_executable(sk03a_part_viewport_controller_test
    sk03a_part_viewport_controller_test.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/part_document_tree_controller.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/part_sketch_interaction_controller.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/part_viewport_controller.cpp
)

target_include_directories(sk03a_part_viewport_controller_test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/ui
)

target_link_libraries(sk03a_part_viewport_controller_test
    PRIVATE
        simplesolid2_application
        simplesolid2_viewer
        Qt6::Widgets
)

target_compile_features(sk03a_part_viewport_controller_test PRIVATE cxx_std_20)

add_test(
    NAME sk03a.part_viewport_controller
    COMMAND sk03a_part_viewport_controller_test
)

set_tests_properties(sk03a.part_viewport_controller PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)


add_executable(sk03a_viewer_native_input_test
    sk03a_viewer_native_input_test.cpp
)

target_include_directories(sk03a_viewer_native_input_test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/ui
)

target_link_libraries(sk03a_viewer_native_input_test
    PRIVATE
        simplesolid2_part
        simplesolid2_viewer
        simplesolid2_viewer_qt_occt
        Qt6::Widgets
        Qt6::Test
)

target_compile_features(sk03a_viewer_native_input_test PRIVATE cxx_std_20)

add_test(
    NAME sk03a.viewer_native_input
    COMMAND sk03a_viewer_native_input_test
)

set_tests_properties(sk03a.viewer_native_input PROPERTIES
    TIMEOUT 30
)


add_executable(sk04a_sketch_interaction_state_test
    sk04a_sketch_interaction_state_test.cpp
)

target_link_libraries(sk04a_sketch_interaction_state_test
    PRIVATE
        simplesolid2_sketch
)

target_compile_features(sk04a_sketch_interaction_state_test PRIVATE cxx_std_20)

add_test(
    NAME sk04a.sketch_interaction_state
    COMMAND sk04a_sketch_interaction_state_test
)


add_executable(sk04a_batch_delete_test
    sk04a_batch_delete_test.cpp
)

target_link_libraries(sk04a_batch_delete_test
    PRIVATE
        simplesolid2_application
)

target_compile_features(sk04a_batch_delete_test PRIVATE cxx_std_20)

add_test(
    NAME sk04a.batch_delete
    COMMAND sk04a_batch_delete_test
)


add_executable(sk04a_line_commit_protocol_test
    sk04a_line_commit_protocol_test.cpp
)

target_link_libraries(sk04a_line_commit_protocol_test
    PRIVATE
        simplesolid2_application
        simplesolid2_sketch
)

target_compile_features(sk04a_line_commit_protocol_test PRIVATE cxx_std_20)

add_test(
    NAME sk04a.line_commit_protocol
    COMMAND sk04a_line_commit_protocol_test
)


add_executable(sk04b_viewer_selection_query_contracts_test
    sk04b_viewer_selection_query_contracts_test.cpp
)

target_link_libraries(sk04b_viewer_selection_query_contracts_test
    PRIVATE
        simplesolid2_viewer
)

target_compile_features(sk04b_viewer_selection_query_contracts_test PRIVATE cxx_std_20)

add_test(
    NAME sk04b.viewer_selection_query_contracts
    COMMAND sk04b_viewer_selection_query_contracts_test
)


add_executable(sk04b_part_viewport_selection_bridge_test
    sk04b_part_viewport_selection_bridge_test.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/part_document_tree_controller.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/part_sketch_interaction_controller.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/part_viewport_controller.cpp
)

target_include_directories(sk04b_part_viewport_selection_bridge_test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/ui
)

target_link_libraries(sk04b_part_viewport_selection_bridge_test
    PRIVATE
        simplesolid2_application
        simplesolid2_viewer
        Qt6::Widgets
)

target_compile_features(sk04b_part_viewport_selection_bridge_test PRIVATE cxx_std_20)

add_test(
    NAME sk04b.part_viewport_selection_bridge
    COMMAND sk04b_part_viewport_selection_bridge_test
)

set_tests_properties(sk04b.part_viewport_selection_bridge PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)


add_executable(sk04b_viewer_native_selection_query_test
    sk04b_viewer_native_selection_query_test.cpp
)

target_link_libraries(sk04b_viewer_native_selection_query_test
    PRIVATE
        simplesolid2_viewer
        simplesolid2_viewer_qt_occt
        Qt6::Widgets
        Qt6::Test
)

target_compile_features(sk04b_viewer_native_selection_query_test PRIVATE cxx_std_20)

add_test(
    NAME sk04b.viewer_native_selection_query
    COMMAND sk04b_viewer_native_selection_query_test
)

set_tests_properties(sk04b.viewer_native_selection_query PROPERTIES
    TIMEOUT 30
)


add_executable(sk04c_part_sketch_interaction_controller_test
    sk04c_part_sketch_interaction_controller_test.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/part_document_tree_controller.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/part_sketch_interaction_controller.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/part_viewport_controller.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/part_sketch_interaction_controller.cpp
)

target_include_directories(sk04c_part_sketch_interaction_controller_test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/ui
)

target_link_libraries(sk04c_part_sketch_interaction_controller_test
    PRIVATE
        simplesolid2_application
        simplesolid2_viewer
        Qt6::Widgets
)

target_compile_features(sk04c_part_sketch_interaction_controller_test PRIVATE cxx_std_20)

add_test(
    NAME sk04c.part_sketch_interaction_controller
    COMMAND sk04c_part_sketch_interaction_controller_test
)

set_tests_properties(sk04c.part_sketch_interaction_controller PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)


add_executable(wb01b_navigation_cube_semantics_test
    wb01b_navigation_cube_semantics_test.cpp
)

target_link_libraries(wb01b_navigation_cube_semantics_test
    PRIVATE
        simplesolid2_viewer
)

target_compile_features(wb01b_navigation_cube_semantics_test PRIVATE cxx_std_20)

add_test(
    NAME wb01b.navigation_cube_semantics
    COMMAND wb01b_navigation_cube_semantics_test
)


add_executable(wb01b_navigation_cube_native_test
    wb01b_navigation_cube_native_test.cpp
)

target_link_libraries(wb01b_navigation_cube_native_test
    PRIVATE
        simplesolid2_viewer
        simplesolid2_viewer_qt_occt
        Qt6::Widgets
        Qt6::Test
)

target_compile_features(wb01b_navigation_cube_native_test PRIVATE cxx_std_20)

add_test(
    NAME wb01b.navigation_cube_native
    COMMAND wb01b_navigation_cube_native_test
)

set_tests_properties(wb01b.navigation_cube_native PROPERTIES
    TIMEOUT 30
)
