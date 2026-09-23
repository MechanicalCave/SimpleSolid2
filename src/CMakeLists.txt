add_library(simplesolid2_application STATIC
    application/recent_project_store.cpp
    application/project_session.cpp
    application/project_workspace_metadata.cpp
)

target_include_directories(simplesolid2_application
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/application/include
)

target_compile_features(simplesolid2_application PUBLIC cxx_std_20)
