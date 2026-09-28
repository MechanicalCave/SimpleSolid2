cmake_minimum_required(VERSION 3.24)

project(SimpleSolid2
    VERSION 0.1.0
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

option(
    SS2_ENABLE_C2_HISTORY_BENCHMARK
    "Build the C2 semantic history benchmark evidence target"
    OFF
)

add_subdirectory(src)
if(SS2_ENABLE_C2_HISTORY_BENCHMARK)
    add_subdirectory(benchmarks)
endif()
add_subdirectory(tests)
