if(NOT DEFINED SOURCE_ROOT)
    message(FATAL_ERROR "SOURCE_ROOT is required")
endif()

file(GLOB_RECURSE SKETCH_CORE_FILES
    "${SOURCE_ROOT}/src/sketch/*.cpp"
    "${SOURCE_ROOT}/src/sketch/include/*.hpp")

if(NOT SKETCH_CORE_FILES)
    message(FATAL_ERROR "No Shared 2D / Sketch Core files found")
endif()

set(FORBIDDEN_SKETCH_TOKENS
    "simplesolid2/part/"
    "simplesolid2/application/"
    "simplesolid2/persistence/"
    "simplesolid2/viewer/"
    "viewer_qt_occt"
    "TopoDS_"
    "AIS_"
    "V3d_"
    "Graphic3d_"
    "#include <Q"
    "#include <Qt")

foreach(FILE_PATH IN LISTS SKETCH_CORE_FILES)
    file(READ "${FILE_PATH}" CONTENT)
    foreach(TOKEN IN LISTS FORBIDDEN_SKETCH_TOKENS)
        string(FIND "${CONTENT}" "${TOKEN}" POSITION)
        if(NOT POSITION EQUAL -1)
            message(FATAL_ERROR
                "Shared 2D / Sketch Core leaks forbidden dependency token '${TOKEN}': ${FILE_PATH}")
        endif()
    endforeach()
endforeach()
