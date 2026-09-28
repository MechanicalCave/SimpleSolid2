if(NOT DEFINED SOURCE_ROOT)
    message(FATAL_ERROR "SOURCE_ROOT is required")
endif()

set(_semantic_files
    "${SOURCE_ROOT}/src/application/include/simplesolid2/application/cad_input_semantics.hpp"
    "${SOURCE_ROOT}/src/application/cad_input_semantics.cpp"
)
foreach(_file IN LISTS _semantic_files)
    file(READ "${_file}" _content)
    foreach(_forbidden "#include <Q" "Qt::" "OpenCASCADE" "TopoDS" "AIS_" "V3d_" "viewer_qt_occt" "src/ui/")
        string(FIND "${_content}" "${_forbidden}" _pos)
        if(NOT _pos EQUAL -1)
            message(FATAL_ERROR "D semantic CAD input boundary leaked '${_forbidden}' in ${_file}")
        endif()
    endforeach()
endforeach()

file(READ "${SOURCE_ROOT}/src/ui/cad_workbench.cpp" _workbench)
foreach(_forbidden "parseBareSketchDistance(" "submitted.toUpper()" "command == QStringLiteral(\"LINE\")" "command == QStringLiteral(\"MOVE\")" "command == QStringLiteral(\"COPY\")")
    string(FIND "${_workbench}" "${_forbidden}" _pos)
    if(NOT _pos EQUAL -1)
        message(FATAL_ERROR "D raw CAD token semantics remain in cad_workbench.cpp: ${_forbidden}")
    endif()
endforeach()
message(STATUS "D CAD input semantic boundary verification PASSED")
