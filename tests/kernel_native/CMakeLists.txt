add_executable(pm00a_kernel_native_smoke_test
    pm00a_kernel_native_smoke_test.cpp
)

target_link_libraries(pm00a_kernel_native_smoke_test
    PRIVATE
        TKernel
        TKMath
        TKG2d
        TKG3d
        TKGeomBase
        TKBRep
        TKTopAlgo
        TKPrim
)

target_compile_features(pm00a_kernel_native_smoke_test PRIVATE cxx_std_20)

add_test(
    NAME pm00a.kernel_native_smoke
    COMMAND pm00a_kernel_native_smoke_test
)

set_tests_properties(pm00a.kernel_native_smoke PROPERTIES
    LABELS "tier-fast;subsystem-kernel;native"
)

add_custom_target(ss2_tests_subsystem_kernel
    DEPENDS pm00a_kernel_native_smoke_test
)

foreach(_forbidden_target IN ITEMS
        simplesolid2_ui
        simplesolid2_viewer_qt_occt
        simplesolid2)
    if(TARGET ${_forbidden_target})
        message(FATAL_ERROR
            "Kernel-native build unexpectedly contains desktop target ${_forbidden_target}")
    endif()
endforeach()

if(TARGET Qt6::Widgets OR TARGET Qt6::Test)
    message(FATAL_ERROR "Kernel-native build unexpectedly discovered Qt")
endif()

get_property(_ss2_kernel_tests DIRECTORY PROPERTY TESTS)
foreach(_test IN LISTS _ss2_kernel_tests)
    get_property(_labels TEST "${_test}" PROPERTY LABELS)
    list(FIND _labels "subsystem-kernel" _kernel_label)
    if(_kernel_label EQUAL -1)
        message(FATAL_ERROR
            "PM-00A kernel-native regression '${_test}' is missing subsystem-kernel")
    endif()

    list(FIND _labels "tier-fast" _fast_label)
    list(FIND _labels "tier-full-only" _full_only_label)
    if((_fast_label EQUAL -1 AND _full_only_label EQUAL -1) OR
       (NOT _fast_label EQUAL -1 AND NOT _full_only_label EQUAL -1))
        message(FATAL_ERROR
            "PM-00A kernel-native regression '${_test}' must have exactly one tier classification")
    endif()
endforeach()
