if(SS2_ENABLE_C2_HISTORY_BENCHMARK)
    add_executable(c2_history_benchmark
        c2_history_benchmark.cpp
    )

    target_link_libraries(c2_history_benchmark
        PRIVATE
            simplesolid2_application
    )

    target_compile_features(c2_history_benchmark PRIVATE cxx_std_20)

    if(WIN32)
        target_link_libraries(c2_history_benchmark PRIVATE Psapi)
    endif()
endif()

if(SS2_ENABLE_E2_PRESENTATION_BENCHMARK)
    if(NOT SS2_BUILD_DESKTOP)
        message(FATAL_ERROR
            "E2 presentation benchmark requires SS2_BUILD_DESKTOP=ON")
    endif()

    find_package(Qt6 6 REQUIRED COMPONENTS Widgets)

    add_executable(e2_presentation_benchmark
        e2_presentation_benchmark.cpp
    )

    target_link_libraries(e2_presentation_benchmark
        PRIVATE
            simplesolid2_ui
            simplesolid2_viewer_qt_occt
            Qt6::Widgets
    )

    target_compile_features(e2_presentation_benchmark PRIVATE cxx_std_20)
endif()
