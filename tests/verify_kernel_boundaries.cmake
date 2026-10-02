cmake_minimum_required(VERSION 3.24)

if(NOT DEFINED SOURCE_ROOT)
    message(FATAL_ERROR "SOURCE_ROOT is required")
endif()

function(ss2_assert_no_pattern path pattern description)
    file(READ "${path}" content)
    if(content MATCHES "${pattern}")
        file(RELATIVE_PATH rel "${SOURCE_ROOT}" "${path}")
        message(FATAL_ERROR
            "PM-00A Kernel boundary violation in ${rel}: ${description}")
    endif()
endfunction()

file(GLOB_RECURSE neutral_kernel_files
    "${SOURCE_ROOT}/src/kernel/*.hpp"
    "${SOURCE_ROOT}/src/kernel/*.cpp"
)

foreach(path IN LISTS neutral_kernel_files)
    ss2_assert_no_pattern(
        "${path}"
        "simplesolid2/(part|sketch|application|viewer|ui)"
        "neutral Kernel depends on an owning/product subsystem")
    ss2_assert_no_pattern(
        "${path}"
        "(OpenCASCADE|TopoDS_|BRep[A-Z]|AIS_|V3d_|Qt6|#include[ \t]*<Q[A-Z])"
        "neutral Kernel contains provider/UI types")
endforeach()

file(GLOB_RECURSE part_files
    "${SOURCE_ROOT}/src/part/*.hpp"
    "${SOURCE_ROOT}/src/part/*.cpp"
)

foreach(path IN LISTS part_files)
    ss2_assert_no_pattern(
        "${path}"
        "simplesolid2/kernel_occt"
        "Part depends on OCCT provider surface")
    ss2_assert_no_pattern(
        "${path}"
        "(TopoDS_|AIS_|V3d_)"
        "Part contains provider topology/UI identity")
endforeach()

set(provider_header
    "${SOURCE_ROOT}/src/kernel_occt/include/simplesolid2/kernel_occt/profile_face_evidence.hpp")
ss2_assert_no_pattern(
    "${provider_header}"
    "(OpenCASCADE|TopoDS_|BRep[A-Z]|AIS_|V3d_|Qt6|#include[ \t]*<Q[A-Z])"
    "public OCCT evidence header leaks provider handles")

message(STATUS "PM-00A Kernel boundaries verified")
