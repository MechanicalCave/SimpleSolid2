if(NOT DEFINED SOURCE_ROOT)
    message(FATAL_ERROR "SOURCE_ROOT is required")
endif()

set(_part_workbench_files
    "${SOURCE_ROOT}/src/ui/cad_workbench.hpp"
    "${SOURCE_ROOT}/src/ui/cad_workbench.cpp"
)

foreach(_file IN LISTS _part_workbench_files)
    file(READ "${_file}" _content)

    foreach(_forbidden IN ITEMS
        "ProjectSession"
        "QTabBar"
        "OpenDocumentDialog"
        "WorkspaceLocationDialog"
        "DocumentWorkspaceIndex"
    )
        string(FIND "${_content}" "${_forbidden}" _found)
        if(NOT _found EQUAL -1)
            message(FATAL_ERROR
                "WS-01 Part Workbench boundary violation in ${_file}: "
                "forbidden Project-navigation token '${_forbidden}'")
        endif()
    endforeach()
endforeach()

set(_project_shell_files
    "${SOURCE_ROOT}/src/ui/project_workspace_shell.hpp"
    "${SOURCE_ROOT}/src/ui/project_workspace_shell.cpp"
)

foreach(_file IN LISTS _project_shell_files)
    file(READ "${_file}" _content)

    foreach(_forbidden IN ITEMS
        "DocumentSession"
        "PartDocument"
        "AssemblyDocument"
        "DrawingDocument"
        "PartDocumentStore"
    )
        string(FIND "${_content}" "${_forbidden}" _found)
        if(NOT _found EQUAL -1)
            message(FATAL_ERROR
                "WS-01 Project Workspace Shell boundary violation in ${_file}: "
                "forbidden CAD-domain token '${_forbidden}'")
        endif()
    endforeach()
endforeach()
