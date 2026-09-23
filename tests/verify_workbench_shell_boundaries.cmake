if(NOT DEFINED SOURCE_ROOT)
    message(FATAL_ERROR "SOURCE_ROOT is required")
endif()

set(_workbench_shell_files
    "${SOURCE_ROOT}/src/ui/cad_workbench_shell.hpp"
    "${SOURCE_ROOT}/src/ui/cad_workbench_shell.cpp"
)

foreach(_file IN LISTS _workbench_shell_files)
    file(READ "${_file}" _content)

    foreach(_forbidden IN ITEMS
        "simplesolid2/application"
        "simplesolid2/part"
        "ProjectSession"
        "DocumentSession"
        "PartDocument"
        "AssemblyDocument"
        "DrawingDocument"
    )
        string(FIND "${_content}" "${_forbidden}" _found)
        if(NOT _found EQUAL -1)
            message(FATAL_ERROR
                "CAD Workbench shell boundary violation in ${_file}: "
                "forbidden dependency/token '${_forbidden}'")
        endif()
    endforeach()
endforeach()
