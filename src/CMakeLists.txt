add_library(simplesolid2_application STATIC
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

target_compile_features(simplesolid2_application PUBLIC cxx_std_20)

find_package(Qt6 6 REQUIRED COMPONENTS Widgets)

add_executable(simplesolid2
    app/main.cpp
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
