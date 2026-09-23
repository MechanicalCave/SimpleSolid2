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
