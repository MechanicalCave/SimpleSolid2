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
