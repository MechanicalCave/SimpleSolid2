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

foreach(_forbidden_target IN ITEMS
        simplesolid2_ui
        simplesolid2_viewer_qt_occt
        simplesolid2)
    if(TARGET ${_forbidden_target})
        message(FATAL_ERROR
            "Core-only build unexpectedly contains desktop target ${_forbidden_target}")
    endif()
endforeach()
