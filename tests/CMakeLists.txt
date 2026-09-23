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

add_executable(ph02a_project_hub_selection_ux_test
    project_hub_selection_ux_test.cpp
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
