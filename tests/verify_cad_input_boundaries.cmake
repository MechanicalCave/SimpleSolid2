if(NOT DEFINED SOURCE_ROOT)
    message(FATAL_ERROR "SOURCE_ROOT is required")
endif()

set(_files
    "${SOURCE_ROOT}/src/application/include/simplesolid2/application/cad_input.hpp"
    "${SOURCE_ROOT}/src/application/cad_input.cpp"
)

foreach(_file IN LISTS _files)
    file(READ "${_file}" _content)
    foreach(_forbidden
            "QWidget"
            "QLineEdit"
            "Qt::"
            "OpenCASCADE"
            "TopoDS"
            "AIS_"
            "V3d_"
            "PartDocument"
            "SketchTool"
            "PointRequest"
            "AssemblyDocument"
            "DrawingDocument")
        string(FIND "${_content}" "${_forbidden}" _pos)
        if(NOT _pos EQUAL -1)
            message(FATAL_ERROR
                "WB-02 CAD input boundary leaked '${_forbidden}' in ${_file}")
        endif()
    endforeach()
endforeach()

message(STATUS "WB-02 CAD input boundary verification PASSED")
