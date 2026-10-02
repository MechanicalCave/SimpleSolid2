set(_ss2_core_only_test_root "${CMAKE_CURRENT_LIST_DIR}/..")

function(ss2_core_only_test target test_name source)
    add_executable(${target}
        "${_ss2_core_only_test_root}/${source}"
    )
    target_compile_features(${target} PRIVATE cxx_std_20)
    add_test(NAME ${test_name} COMMAND ${target})
endfunction()

ss2_core_only_test(d_cad_input_semantics_test d.cad_input_semantics d_cad_input_semantics_test.cpp)
target_link_libraries(d_cad_input_semantics_test PRIVATE simplesolid2_application)

ss2_core_only_test(r10_quantity_input_test r10.quantity_input r10_quantity_input_test.cpp)
target_link_libraries(r10_quantity_input_test PRIVATE simplesolid2_application)

ss2_core_only_test(wb02_cad_input_session_test wb02.cad_input_session wb02_cad_input_session_test.cpp)
target_link_libraries(wb02_cad_input_session_test PRIVATE simplesolid2_cad_input)

ss2_core_only_test(part01_document_primitives_test part01.document_primitives document_primitives_test.cpp)
target_link_libraries(part01_document_primitives_test PRIVATE simplesolid2_core)

ss2_core_only_test(part01_part_document_transaction_test part01.part_document_transaction part_document_transaction_test.cpp)
target_link_libraries(part01_part_document_transaction_test PRIVATE simplesolid2_part)

ss2_core_only_test(part01_document_session_test part01.document_session document_session_test.cpp)
target_link_libraries(part01_document_session_test PRIVATE simplesolid2_application)

ss2_core_only_test(sk04a_line_commit_protocol_test sk04a.line_commit_protocol sk04a_line_commit_protocol_test.cpp)
target_link_libraries(sk04a_line_commit_protocol_test PRIVATE simplesolid2_application simplesolid2_sketch)

ss2_core_only_test(sk05a_line_geometry_command_test sk05a.line_geometry_command sk05a_line_geometry_command_test.cpp)
target_link_libraries(sk05a_line_geometry_command_test PRIVATE simplesolid2_application)

ss2_core_only_test(e1_arc_numerical_stability_test e1.arc_numerical_stability e1_arc_numerical_stability_test.cpp)
target_link_libraries(e1_arc_numerical_stability_test PRIVATE simplesolid2_sketch)

ss2_core_only_test(sk07a_transform_core_test sk07a.transform_core sk07a_transform_core_test.cpp)
target_link_libraries(sk07a_transform_core_test PRIVATE simplesolid2_sketch)

ss2_core_only_test(sk07c_copy_interaction_state_test sk07c.copy_interaction_state sk07c_copy_interaction_state_test.cpp)
target_link_libraries(sk07c_copy_interaction_state_test PRIVATE simplesolid2_sketch)

ss2_core_only_test(sk07c_copy_command_test sk07c.copy_command sk07c_copy_command_test.cpp)
target_link_libraries(sk07c_copy_command_test PRIVATE simplesolid2_application)

ss2_core_only_test(sk07f_precision_input_state_test sk07f.precision_input_state sk07f_precision_input_state_test.cpp)
target_link_libraries(sk07f_precision_input_state_test PRIVATE simplesolid2_sketch)

ss2_core_only_test(r11_snap_core_test r11.snap_core r11_snap_core_test.cpp)
target_link_libraries(r11_snap_core_test PRIVATE simplesolid2_sketch)

add_test(
    NAME wb02.cad_input_boundaries
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_ROOT=${PROJECT_SOURCE_DIR}
        -P ${_ss2_core_only_test_root}/verify_cad_input_boundaries.cmake
)

add_test(
    NAME d.cad_input_semantic_boundaries
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_ROOT=${PROJECT_SOURCE_DIR}
        -P ${_ss2_core_only_test_root}/verify_cad_input_semantic_boundaries.cmake
)

add_test(
    NAME pm00a.kernel_boundaries
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_ROOT=${PROJECT_SOURCE_DIR}
        -P ${_ss2_core_only_test_root}/verify_kernel_boundaries.cmake
)

set_tests_properties(d.cad_input_semantics PROPERTIES LABELS "tier-fast;subsystem-application")
set_tests_properties(r10.quantity_input PROPERTIES LABELS "tier-fast;subsystem-application;subsystem-core")
set_tests_properties(wb02.cad_input_session PROPERTIES LABELS "tier-fast;subsystem-application")
set_tests_properties(part01.document_primitives PROPERTIES LABELS "tier-fast;subsystem-core")
set_tests_properties(part01.part_document_transaction PROPERTIES LABELS "tier-fast;subsystem-part")
set_tests_properties(part01.document_session PROPERTIES LABELS "tier-fast;subsystem-application;subsystem-part")
set_tests_properties(sk04a.line_commit_protocol PROPERTIES LABELS "tier-fast;subsystem-application;subsystem-part;subsystem-sketch")
set_tests_properties(sk05a.line_geometry_command PROPERTIES LABELS "tier-fast;subsystem-application;subsystem-persistence;subsystem-part;subsystem-sketch")
set_tests_properties(e1.arc_numerical_stability PROPERTIES LABELS "tier-fast;subsystem-sketch")
set_tests_properties(sk07a.transform_core PROPERTIES LABELS "tier-fast;subsystem-sketch")
set_tests_properties(sk07c.copy_interaction_state PROPERTIES LABELS "tier-fast;subsystem-sketch")
set_tests_properties(sk07c.copy_command PROPERTIES LABELS "tier-fast;subsystem-application;subsystem-persistence;subsystem-part;subsystem-sketch")
set_tests_properties(sk07f.precision_input_state PROPERTIES LABELS "tier-fast;subsystem-sketch")
set_tests_properties(r11.snap_core PROPERTIES LABELS "tier-fast;subsystem-core;subsystem-sketch")
set_tests_properties(wb02.cad_input_boundaries PROPERTIES LABELS "tier-fast;subsystem-core")
set_tests_properties(d.cad_input_semantic_boundaries PROPERTIES LABELS "tier-fast;subsystem-core")
set_tests_properties(pm00a.kernel_boundaries PROPERTIES LABELS "tier-fast;subsystem-core;subsystem-kernel;subsystem-part")

get_property(_ss2_core_only_registered_tests DIRECTORY PROPERTY TESTS)
foreach(_test IN LISTS _ss2_core_only_registered_tests)
    get_property(_labels TEST "${_test}" PROPERTY LABELS)

    list(FIND _labels "tier-fast" _fast_label)
    list(FIND _labels "tier-full-only" _full_only_label)
    if((_fast_label EQUAL -1 AND _full_only_label EQUAL -1) OR
       (NOT _fast_label EQUAL -1 AND NOT _full_only_label EQUAL -1))
        message(FATAL_ERROR
            "Core-only regression '${_test}' must have exactly one tier classification")
    endif()

    set(_has_subsystem FALSE)
    foreach(_label IN LISTS _labels)
        if(_label MATCHES "^subsystem-[a-z0-9_-]+$")
            set(_has_subsystem TRUE)
        endif()
    endforeach()
    if(NOT _has_subsystem)
        message(FATAL_ERROR
            "Core-only regression '${_test}' is missing subsystem metadata")
    endif()
endforeach()

foreach(_forbidden_target IN ITEMS
        simplesolid2_ui
        simplesolid2_viewer_qt_occt
        simplesolid2)
    if(TARGET ${_forbidden_target})
        message(FATAL_ERROR
            "Core-only build unexpectedly contains desktop target ${_forbidden_target}")
    endif()
endforeach()
