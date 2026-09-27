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

add_test(
    NAME c2.history_benchmark_smoke
    COMMAND c2_history_benchmark
        --entities 100
        --depth 5
        --scenario all
        --samples 2
        --warmup 1
)

add_test(
    NAME c2.history_benchmark_self_test
    COMMAND c2_history_benchmark --self-test
)

add_test(
    NAME c2.history_benchmark_invalid_args
    COMMAND c2_history_benchmark
        --entities 0
        --depth 5
        --scenario add
        --samples 1
        --warmup 0
)

set_tests_properties(
    c2.history_benchmark_invalid_args
    PROPERTIES
        WILL_FAIL TRUE
)

set(_ss2_c2_benchmark_tests
    c2.history_benchmark_smoke
    c2.history_benchmark_self_test
    c2.history_benchmark_invalid_args
)

set_property(
    TEST ${_ss2_c2_benchmark_tests}
    APPEND PROPERTY LABELS
        "tier-fast;subsystem-application;subsystem-part;subsystem-sketch"
)
