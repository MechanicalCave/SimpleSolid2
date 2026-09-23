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
