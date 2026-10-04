add_executable(pm00a_kernel_native_smoke_test
    pm00a_kernel_native_smoke_test.cpp
)

target_include_directories(pm00a_kernel_native_smoke_test
    PRIVATE
        ${OpenCASCADE_INCLUDE_DIR}
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

add_executable(pm00a_profile_cold_rebuild_test
    pm00a_profile_cold_rebuild_test.cpp
)

target_link_libraries(pm00a_profile_cold_rebuild_test
    PRIVATE
        simplesolid2_application
        simplesolid2_kernel
        simplesolid2_kernel_occt
        simplesolid2_part
)

target_compile_features(pm00a_profile_cold_rebuild_test PRIVATE cxx_std_20)

add_test(
    NAME pm00a.profile_cold_rebuild
    COMMAND pm00a_profile_cold_rebuild_test
)

set_tests_properties(pm00a.profile_cold_rebuild PROPERTIES
    LABELS "tier-full-only;subsystem-kernel;subsystem-part;subsystem-persistence;native"
)

add_executable(pm00a_e01_extrude_evidence_test
    pm00a_e01_extrude_evidence_test.cpp
)

target_link_libraries(pm00a_e01_extrude_evidence_test
    PRIVATE
        simplesolid2_application
        simplesolid2_kernel
        simplesolid2_kernel_occt
        simplesolid2_part
)

target_compile_features(pm00a_e01_extrude_evidence_test PRIVATE cxx_std_20)

add_test(
    NAME pm00a.e01_extrude_evidence
    COMMAND pm00a_e01_extrude_evidence_test
)

set_tests_properties(pm00a.e01_extrude_evidence PROPERTIES
    LABELS "tier-full-only;subsystem-kernel;subsystem-part;subsystem-persistence;native"
)

add_executable(pm00a_e05_similarity_evidence_test
    pm00a_e05_similarity_evidence_test.cpp
)

target_link_libraries(pm00a_e05_similarity_evidence_test
    PRIVATE
        simplesolid2_application
        simplesolid2_kernel
        simplesolid2_kernel_occt
        simplesolid2_part
)

target_compile_features(pm00a_e05_similarity_evidence_test PRIVATE cxx_std_20)

add_test(
    NAME pm00a.e05_similarity_guardrails
    COMMAND pm00a_e05_similarity_evidence_test
)

set_tests_properties(pm00a.e05_similarity_guardrails PROPERTIES
    LABELS "tier-full-only;subsystem-kernel;subsystem-part;subsystem-persistence;native"
)

add_executable(pm00a_e03_e04_cardinality_evidence_test
    pm00a_e03_e04_cardinality_evidence_test.cpp
)

target_link_libraries(pm00a_e03_e04_cardinality_evidence_test
    PRIVATE
        simplesolid2_kernel
        simplesolid2_kernel_occt
)

target_compile_features(pm00a_e03_e04_cardinality_evidence_test PRIVATE cxx_std_20)

add_test(
    NAME pm00a.e03_e04_cardinality
    COMMAND pm00a_e03_e04_cardinality_evidence_test
)

set_tests_properties(pm00a.e03_e04_cardinality PROPERTIES
    LABELS "tier-full-only;subsystem-kernel;native"
)

add_executable(pm00a_e02_multistage_lineage_test
    pm00a_e02_multistage_lineage_test.cpp
)

target_link_libraries(pm00a_e02_multistage_lineage_test
    PRIVATE
        simplesolid2_kernel
        simplesolid2_kernel_occt
)

target_compile_features(pm00a_e02_multistage_lineage_test PRIVATE cxx_std_20)

add_test(
    NAME pm00a.e02_multistage_lineage
    COMMAND pm00a_e02_multistage_lineage_test
)

set_tests_properties(pm00a.e02_multistage_lineage PROPERTIES
    LABELS "tier-full-only;subsystem-kernel;native"
)

add_executable(pm00a_e06_revolve_evidence_test
    pm00a_e06_revolve_evidence_test.cpp
)

target_link_libraries(pm00a_e06_revolve_evidence_test
    PRIVATE
        simplesolid2_kernel
        simplesolid2_kernel_occt
)

target_compile_features(pm00a_e06_revolve_evidence_test PRIVATE cxx_std_20)

add_test(
    NAME pm00a.e06_revolve_periodic
    COMMAND pm00a_e06_revolve_evidence_test
)

set_tests_properties(pm00a.e06_revolve_periodic PROPERTIES
    LABELS "tier-full-only;subsystem-kernel;native"
)

add_executable(pm00a_e10_numerical_policy_evidence_test
    pm00a_e10_numerical_policy_evidence_test.cpp
)

target_include_directories(pm00a_e10_numerical_policy_evidence_test
    PRIVATE
        ${OpenCASCADE_INCLUDE_DIR}
)

target_link_libraries(pm00a_e10_numerical_policy_evidence_test
    PRIVATE
        simplesolid2_kernel
        simplesolid2_kernel_occt
        TKernel
        TKMath
        TKG2d
        TKG3d
        TKGeomBase
        TKBRep
        TKTopAlgo
        TKPrim
        TKBO
)

target_compile_features(pm00a_e10_numerical_policy_evidence_test PRIVATE cxx_std_20)

add_test(
    NAME pm00a.e10_numerical_policy
    COMMAND pm00a_e10_numerical_policy_evidence_test
)

set_tests_properties(pm00a.e10_numerical_policy PROPERTIES
    LABELS "tier-full-only;subsystem-kernel;native"
)

add_executable(pm01b_production_kernel_test
    pm01b_production_kernel_test.cpp
)

target_link_libraries(pm01b_production_kernel_test
    PRIVATE
        simplesolid2_kernel
        simplesolid2_kernel_occt
)

target_compile_features(pm01b_production_kernel_test PRIVATE cxx_std_20)

add_test(
    NAME pm01b.production_kernel
    COMMAND pm01b_production_kernel_test
)

set_tests_properties(pm01b.production_kernel PROPERTIES
    LABELS "tier-full-only;subsystem-kernel;subsystem-part;native"
)

add_executable(pm02p_a_topology_inventory_test
    pm02p_a_topology_inventory_test.cpp
)

target_link_libraries(pm02p_a_topology_inventory_test
    PRIVATE
        simplesolid2_kernel
        simplesolid2_kernel_occt
)

target_compile_features(pm02p_a_topology_inventory_test PRIVATE cxx_std_20)

add_test(
    NAME pm02p.a_topology_inventory
    COMMAND pm02p_a_topology_inventory_test
)

set_tests_properties(pm02p.a_topology_inventory PROPERTIES
    LABELS "tier-full-only;subsystem-kernel;native"
)

add_executable(pm02p_b1_surface_carriers_test
    pm02p_b1_surface_carriers_test.cpp
)

target_link_libraries(pm02p_b1_surface_carriers_test
    PRIVATE
        simplesolid2_kernel
        simplesolid2_kernel_occt
)

target_compile_features(pm02p_b1_surface_carriers_test PRIVATE cxx_std_20)

add_test(
    NAME pm02p.b1_surface_carriers
    COMMAND pm02p_b1_surface_carriers_test
)

set_tests_properties(pm02p.b1_surface_carriers PROPERTIES
    LABELS "tier-full-only;subsystem-kernel;native"
)

add_executable(pm02p_b2_boolean_surface_lineage_test
    pm02p_b2_boolean_surface_lineage_test.cpp
)

target_link_libraries(pm02p_b2_boolean_surface_lineage_test
    PRIVATE
        simplesolid2_kernel
        simplesolid2_kernel_occt
)

target_compile_features(pm02p_b2_boolean_surface_lineage_test PRIVATE cxx_std_20)

add_test(
    NAME pm02p.b2_boolean_surface_lineage
    COMMAND pm02p_b2_boolean_surface_lineage_test
)

set_tests_properties(pm02p.b2_boolean_surface_lineage PROPERTIES
    LABELS "tier-full-only;subsystem-kernel;native"
)

add_executable(pm02p_b3_extent_cap_frames_test
    pm02p_b3_extent_cap_frames_test.cpp
)

target_link_libraries(pm02p_b3_extent_cap_frames_test
    PRIVATE
        simplesolid2_kernel
        simplesolid2_kernel_occt
)

target_compile_features(pm02p_b3_extent_cap_frames_test PRIVATE cxx_std_20)

add_test(
    NAME pm02p.b3_extent_cap_frames
    COMMAND pm02p_b3_extent_cap_frames_test
)

set_tests_properties(pm02p.b3_extent_cap_frames PROPERTIES
    LABELS "tier-full-only;subsystem-kernel;native"
)

add_executable(pm02p_c_edge_curve_lineage_test
    pm02p_c_edge_curve_lineage_test.cpp
)

target_link_libraries(pm02p_c_edge_curve_lineage_test
    PRIVATE
        simplesolid2_kernel
        simplesolid2_kernel_occt
)

target_compile_features(pm02p_c_edge_curve_lineage_test PRIVATE cxx_std_20)

add_test(
    NAME pm02p.c_edge_curve_lineage
    COMMAND pm02p_c_edge_curve_lineage_test
)

set_tests_properties(pm02p.c_edge_curve_lineage PROPERTIES
    LABELS "tier-full-only;subsystem-kernel;native"
)

add_executable(pm02p_d_vertex_point_lineage_test
    pm02p_d_vertex_point_lineage_test.cpp
)

target_link_libraries(pm02p_d_vertex_point_lineage_test
    PRIVATE
        simplesolid2_kernel
        simplesolid2_kernel_occt
)

target_compile_features(pm02p_d_vertex_point_lineage_test PRIVATE cxx_std_20)

add_test(
    NAME pm02p.d_vertex_point_lineage
    COMMAND pm02p_d_vertex_point_lineage_test
)

set_tests_properties(pm02p.d_vertex_point_lineage PROPERTIES
    LABELS "tier-full-only;subsystem-kernel;native"
)

add_executable(pm02p_e_kernel_lifecycle_test
    pm02p_e_kernel_lifecycle_test.cpp
)

target_link_libraries(pm02p_e_kernel_lifecycle_test
    PRIVATE
        simplesolid2_kernel
        simplesolid2_kernel_occt
)

target_compile_features(pm02p_e_kernel_lifecycle_test PRIVATE cxx_std_20)

add_test(
    NAME pm02p.e_kernel_lifecycle
    COMMAND pm02p_e_kernel_lifecycle_test
)

set_tests_properties(pm02p.e_kernel_lifecycle PROPERTIES
    LABELS "tier-full-only;subsystem-kernel;native"
)

add_custom_target(ss2_tests_subsystem_kernel
    DEPENDS
        pm00a_kernel_native_smoke_test
        pm00a_profile_cold_rebuild_test
        pm00a_e01_extrude_evidence_test
        pm00a_e05_similarity_evidence_test
        pm00a_e03_e04_cardinality_evidence_test
        pm00a_e02_multistage_lineage_test
        pm00a_e06_revolve_evidence_test
        pm00a_e10_numerical_policy_evidence_test
        pm01b_production_kernel_test
        pm02p_a_topology_inventory_test
        pm02p_b1_surface_carriers_test
        pm02p_b2_boolean_surface_lineage_test
        pm02p_b3_extent_cap_frames_test
        pm02p_c_edge_curve_lineage_test
        pm02p_d_vertex_point_lineage_test
        pm02p_e_kernel_lifecycle_test
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
