add_library(simplesolid2_core STATIC
    core/document.cpp
)

target_include_directories(simplesolid2_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/core/include
)

target_compile_features(simplesolid2_core PUBLIC cxx_std_20)

add_library(simplesolid2_sketch STATIC
    sketch/entity_id.cpp
    sketch/interaction_state.cpp
    sketch/local_solver.cpp
    sketch/region_analysis.cpp
    sketch/snap.cpp
    sketch/sketch_id.cpp
    sketch/sketch_model.cpp
    sketch/structural_edit.cpp
    sketch/transform.cpp
)

target_include_directories(simplesolid2_sketch
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/sketch/include
)

target_compile_features(simplesolid2_sketch PUBLIC cxx_std_20)

# Optional original-source-backed PlaneGCS 0.8.0 DEV native test.
# Never fetch, vendor, install or enable for normal application builds.
option(SS2_R13_PLANEGCS_DEV "Opt in to real original PlaneGCS 0.8.0 native DEV CTest" OFF)
if(SS2_R13_PLANEGCS_DEV)
    set(SS2_PLANEGCS_080_SOURCE_DIR "" CACHE PATH "Verified existing PlaneGCS 0.8.0 source root")
    set(SS2_PLANEGCS_EIGEN_INCLUDE_DIR "" CACHE PATH "Existing Eigen3 include directory")
    set(SS2_PLANEGCS_BOOST_INCLUDE_DIR "" CACHE PATH "Existing Boost include directory")
    set(_pg_src "${SS2_PLANEGCS_080_SOURCE_DIR}")
    set(_pg_native "${_pg_src}/src/planegcs")
    # Original planegcs-0.8.0.tar.gz SHA-256:
    # 09e77e6e3a27fa77d3b5a69065ea4c82ae6ed35753246a2e518b1a6005fea162
    # Checking the exact consumed source bytes removes any requirement to
    # keep a tarball next to an already extracted original source tree.
    # No network, vendoring or dependency installation. OFF builds do not
    # execute this verification.
    set(_ss2_pg080_manifest
        "pyproject.toml|356a598b82b6d881966a204ef9037c6894844917fff3e7dbc35657242d32188d"
        "src/wrapper.h|42326c01ad1e29c132f2cdb0a5a975a8e147f6d6e2d165ee535309ad158e6e4a"
        "src/planegcs/Constraints.cpp|aa60fa14490f988d8ff93f94ecf96b0668e70bb3d990ff9a19f3981d02a57a2d"
        "src/planegcs/Constraints.h|20241b5e69293796d7f884b84b72e796e924b9ac6936577f412f97d64db7bcfe"
        "src/planegcs/GCS.cpp|d0babb0ed1442595d294e5f6da29abb7131258e7d56df1cd00378dce6c08da26"
        "src/planegcs/GCS.h|360d669db5f2699b60799e754d87423cdc8753afd45d35a5345080d9074269e9"
        "src/planegcs/Geo.cpp|169aad09829b81288d3a81d81cd00fdb5ce9723a4ee34450fb0ac8667b8e9e2b"
        "src/planegcs/Geo.h|8e83416553ddc4e39899958a66f2570879f995b6af518e759d08b3506af13859"
        "src/planegcs/SketcherGlobal.h|821a9ae4f14ce1dab5f3e7410feaf5414ccf4deeadd4b47c2561e089b762c6f1"
        "src/planegcs/SubSystem.cpp|28c94e84ff50187ec9b6060ac72e05fb6671a51fae2be40f479d2b4dd151e393"
        "src/planegcs/SubSystem.h|1327cc2dd648bfb8650442a66a182b71cd0a7e1fd53a956f0426a98a543d4696"
        "src/planegcs/Util.h|dc442c064c232283a9f54285ba709dd6e1aafa5d114f94a5de37ffc180329340"
        "src/planegcs/qp_eq.cpp|dc71350766c1a4edf7024d593cb5d55b0736a4f43d319d25a9c961e8342ca11c"
        "src/planegcs/qp_eq.h|34e373d9b3b0f5938c849ea7489a8405617692a5062198e8ae0f0c6ac5e4d468"
        "src/planegcs/shims/Base/Console.h|d92878730ed22d5d9dd6e76796f33daf6f20133e0ba46520aa5249f321fdfa34"
        "src/planegcs/shims/FCConfig.h|3531d7765950257f109ecbcff71a4ceab9a4ee4c1884f877c732b7365fcb8a93"
        "src/planegcs/shims/FCGlobal.h|ccb438d8401659cb1190934e70d7bbe47b392d93ce5567b3325d18bfb38e6e5a"
        "src/planegcs/shims/boost_graph_adjacency_list.hpp|be9abbbbddb5ca783d07d95dbee732b4254d5b407b91ca88e0c7f1377d1b704d"
    )
    foreach(_ss2_pg080_entry IN LISTS _ss2_pg080_manifest)
        string(REPLACE "|" ";" _ss2_pg080_parts "${_ss2_pg080_entry}")
        list(GET _ss2_pg080_parts 0 _ss2_pg080_relative)
        list(GET _ss2_pg080_parts 1 _ss2_pg080_expected)
        set(_ss2_pg080_file "${_pg_src}/${_ss2_pg080_relative}")
        if(NOT EXISTS "${_ss2_pg080_file}")
            message(FATAL_ERROR
                "Original PlaneGCS 0.8.0 DEV source missing: ${_ss2_pg080_relative}")
        endif()
        file(SHA256 "${_ss2_pg080_file}" _ss2_pg080_actual)
        if(NOT _ss2_pg080_actual STREQUAL _ss2_pg080_expected)
            message(FATAL_ERROR
                "Original PlaneGCS 0.8.0 DEV source hash mismatch: ${_ss2_pg080_relative}")
        endif()
    endforeach()

    if(NOT EXISTS "${_pg_src}/src/wrapper.h" OR
       NOT EXISTS "${_pg_src}/pyproject.toml")
        message(FATAL_ERROR "SS2_R13_PLANEGCS_DEV requires external original PlaneGCS 0.8.0 source")
    endif()
    file(READ "${_pg_src}/pyproject.toml" _pg_version)
    if(NOT _pg_version MATCHES "version = \"0[.]8[.]0\"")
        message(FATAL_ERROR "Expected exactly original PlaneGCS version 0.8.0")
    endif()
    if(NOT EXISTS "${SS2_PLANEGCS_EIGEN_INCLUDE_DIR}/Eigen/Core")
        message(FATAL_ERROR "SS2_PLANEGCS_EIGEN_INCLUDE_DIR must contain Eigen/Core")
    endif()
    if(NOT EXISTS "${SS2_PLANEGCS_BOOST_INCLUDE_DIR}/boost/graph/adjacency_list.hpp")
        message(FATAL_ERROR "SS2_PLANEGCS_BOOST_INCLUDE_DIR must contain Boost Graph")
    endif()
    foreach(_pg_tu Constraints.cpp GCS.cpp Geo.cpp SubSystem.cpp qp_eq.cpp)
        if(NOT EXISTS "${_pg_native}/${_pg_tu}")
            message(FATAL_ERROR "Original 0.8.0 PlaneGCS source missing: ${_pg_tu}")
        endif()
    endforeach()
    add_library(ss2_r13_planegcs_dev STATIC
        sketch/planegcs_dev_solver.cpp
        "${_pg_native}/Constraints.cpp"
        "${_pg_native}/GCS.cpp"
        "${_pg_native}/Geo.cpp"
        "${_pg_native}/SubSystem.cpp"
        "${_pg_native}/qp_eq.cpp")
    target_compile_features(ss2_r13_planegcs_dev PUBLIC cxx_std_20)
    target_link_libraries(ss2_r13_planegcs_dev PUBLIC simplesolid2_sketch)
    target_include_directories(ss2_r13_planegcs_dev
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/sketch/include"
        PRIVATE "${_pg_src}" "${_pg_src}/src"
                "${_pg_native}" "${_pg_native}/shims")
    target_include_directories(ss2_r13_planegcs_dev SYSTEM PRIVATE
        "${SS2_PLANEGCS_EIGEN_INCLUDE_DIR}"
        "${SS2_PLANEGCS_BOOST_INCLUDE_DIR}")
    if(MSVC)
        target_compile_options(ss2_r13_planegcs_dev PRIVATE /W0)
    endif()
endif()

add_library(simplesolid2_miniz STATIC
    ${PROJECT_SOURCE_DIR}/third_party/miniz-3.1.2/miniz.c
    ${PROJECT_SOURCE_DIR}/third_party/miniz-3.1.2/miniz_zip.c
    ${PROJECT_SOURCE_DIR}/third_party/miniz-3.1.2/miniz_tinfl.c
    ${PROJECT_SOURCE_DIR}/third_party/miniz-3.1.2/miniz_tdef.c
)

target_include_directories(simplesolid2_miniz
    PRIVATE
        ${PROJECT_SOURCE_DIR}/third_party/miniz-3.1.2
)

target_compile_definitions(simplesolid2_miniz
    PUBLIC
        MINIZ_NO_TIME
        MINIZ_NO_ZLIB_COMPATIBLE_NAMES
)

set_target_properties(simplesolid2_miniz PROPERTIES
    C_STANDARD 90
    C_STANDARD_REQUIRED ON
)

add_library(simplesolid2_persistence STATIC
    persistence/atomic_file.cpp
    persistence/file_guard.cpp
    persistence/file_snapshot.cpp
    persistence/native_document_container.cpp
)

target_include_directories(simplesolid2_persistence
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/persistence/include
    PRIVATE
        ${PROJECT_SOURCE_DIR}/third_party/miniz-3.1.2
        ${PROJECT_SOURCE_DIR}/third_party/nlohmann-json-3.12.0/include
)

target_link_libraries(simplesolid2_persistence
    PRIVATE
        simplesolid2_miniz
)

target_compile_features(simplesolid2_persistence PUBLIC cxx_std_20)

add_library(simplesolid2_kernel STATIC
    kernel/profile_input.cpp
    kernel/solid_modeling.cpp
)

target_include_directories(simplesolid2_kernel
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/kernel/include
)

target_compile_features(simplesolid2_kernel PUBLIC cxx_std_20)

add_library(simplesolid2_part STATIC
    part/axis.cpp
    part/axis_evaluation.cpp
    part/axis_id.cpp
    part/body_id.cpp
    part/datum.cpp
    part/datum_evaluation.cpp
    part/datum_id.cpp
    part/feature.cpp
    part/feature_evaluation.cpp
    part/feature_id.cpp
    part/part_document.cpp
    part/part_document_store.cpp
    part/part_sketch.cpp
    part/profile.cpp
    part/profile_kernel_input.cpp
    part/profile_id.cpp
)

target_include_directories(simplesolid2_part
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/part/include
)

target_link_libraries(simplesolid2_part
    PUBLIC
        simplesolid2_core
        simplesolid2_kernel
        simplesolid2_persistence
        simplesolid2_sketch
)

target_include_directories(simplesolid2_part
    PRIVATE
        ${PROJECT_SOURCE_DIR}/third_party/nlohmann-json-3.12.0/include
)

target_compile_features(simplesolid2_part PUBLIC cxx_std_20)

add_library(simplesolid2_cad_input STATIC
    application/cad_input.cpp
)

target_include_directories(simplesolid2_cad_input
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/application/include
)

target_compile_features(simplesolid2_cad_input PUBLIC cxx_std_20)

add_library(simplesolid2_application STATIC
    application/axis_draft.cpp
    application/datum_plane_draft.cpp
    application/edge_feature_draft.cpp
    application/extrude_draft.cpp
    application/revolve_draft.cpp
    application/precision_input.cpp
    application/cad_input_semantics.cpp
    application/document_session.cpp
    application/document_workspace.cpp
    application/project_hub_controller.cpp
    application/recent_project_store.cpp
    application/project_session.cpp
    application/project_workspace_metadata.cpp
    application/workspace_directory.cpp
)

target_include_directories(simplesolid2_application
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/application/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/application
)

target_link_libraries(simplesolid2_application
    PUBLIC
        simplesolid2_core
        simplesolid2_part
        simplesolid2_cad_input
)

target_compile_features(simplesolid2_application PUBLIC cxx_std_20)

if(SS2_BUILD_DESKTOP OR SS2_BUILD_KERNEL_NATIVE)
foreach(_target IN ITEMS
        TKernel
        TKMath
        TKG2d
        TKG3d
        TKGeomBase
        TKBRep
        TKMesh
        TKTopAlgo
        TKPrim
        TKBO
        TKFillet)
    if(NOT TARGET ${_target})
        message(FATAL_ERROR "OpenCASCADE package is missing required kernel-native target ${_target}")
    endif()
endforeach()

add_library(simplesolid2_kernel_occt STATIC
    kernel_occt/profile_face_evidence.cpp
    kernel_occt/edge_feature_evidence.cpp
    kernel_occt/solid_modeling_kernel.cpp
)

target_include_directories(simplesolid2_kernel_occt
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/kernel_occt/include
    PRIVATE
        ${OpenCASCADE_INCLUDE_DIR}
)

target_link_libraries(simplesolid2_kernel_occt
    PUBLIC
        simplesolid2_kernel
    PRIVATE
        TKernel
        TKMath
        TKG2d
        TKG3d
        TKGeomBase
        TKBRep
        TKMesh
        TKTopAlgo
        TKPrim
        TKBO
        TKFillet
)

target_compile_features(simplesolid2_kernel_occt PUBLIC cxx_std_20)
endif()

if(SS2_BUILD_DESKTOP)
find_package(Qt6 6 REQUIRED COMPONENTS Widgets)

foreach(_target IN ITEMS TKernel TKMath TKG3d TKGeomBase TKService TKV3d TKOpenGl)
    if(NOT TARGET ${_target})
        message(FATAL_ERROR "OpenCASCADE package is missing required Viewer target ${_target}")
    endif()
endforeach()

add_library(simplesolid2_viewer STATIC
    viewer/camera_state.cpp
    viewer/navigation.cpp
    viewer/navigation_cube.cpp
)

target_include_directories(simplesolid2_viewer
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/viewer/include
)

target_compile_features(simplesolid2_viewer PUBLIC cxx_std_20)

if(NOT WIN32)
    message(FATAL_ERROR "WB-01 native Qt/OCCT Viewer provider is currently supported on Windows")
endif()

add_library(simplesolid2_viewer_qt_occt STATIC
    viewer_qt_occt/navigation_mapping.cpp
    viewer_qt_occt/qt_occt_viewer_widget.cpp
)

target_include_directories(simplesolid2_viewer_qt_occt
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/viewer_qt_occt/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/viewer_qt_occt
)

target_link_libraries(simplesolid2_viewer_qt_occt
    PUBLIC
        simplesolid2_viewer
        Qt6::Widgets
    PRIVATE
        TKernel
        TKMath
        TKG3d
        TKGeomBase
        TKService
        TKV3d
        TKOpenGl
)

target_compile_features(simplesolid2_viewer_qt_occt PUBLIC cxx_std_20)

add_library(simplesolid2_ui STATIC
    ui/cad_workbench.cpp
    ui/cad_workbench_shell.cpp
    ui/open_document_dialog.cpp
    ui/part_document_tree_controller.cpp
    ui/part_sketch_interaction_controller.cpp
    ui/part_viewport_controller.cpp
    ui/project_creation_dialog.cpp
    ui/project_hub_window.cpp
    ui/project_workspace_shell.cpp
    ui/workspace_location_dialog.cpp
)

target_include_directories(simplesolid2_ui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/ui
        ${CMAKE_CURRENT_SOURCE_DIR}/application
)

target_link_libraries(simplesolid2_ui
    PUBLIC
        simplesolid2_application
        simplesolid2_cad_input
        simplesolid2_viewer
        Qt6::Widgets
)

target_compile_features(simplesolid2_ui PUBLIC cxx_std_20)

add_executable(simplesolid2
    app/main.cpp
)

target_include_directories(simplesolid2
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/application
)

target_link_libraries(simplesolid2
    PRIVATE
        simplesolid2_ui
        simplesolid2_viewer_qt_occt
        simplesolid2_kernel_occt
        Qt6::Widgets
)

target_compile_features(simplesolid2 PRIVATE cxx_std_20)

set_target_properties(simplesolid2 PROPERTIES
    OUTPUT_NAME "SimpleSolid2"
)
endif()
