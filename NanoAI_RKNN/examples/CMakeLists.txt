# Aggregated examples entry.

file(GLOB _NANOAI_RKNN_EXAMPLE_CMAKES RELATIVE ${CMAKE_CURRENT_LIST_DIR} "*/CMakeLists.txt")
set(_NANOAI_RKNN_AVAILABLE_EXAMPLES)
foreach(_example_cmake IN LISTS _NANOAI_RKNN_EXAMPLE_CMAKES)
    get_filename_component(_example_name "${_example_cmake}" DIRECTORY)
    list(APPEND _NANOAI_RKNN_AVAILABLE_EXAMPLES ${_example_name})
endforeach()

set(_NANOAI_RKNN_SELECTED_EXAMPLES)
if(NANOAI_RKNN_EXAMPLES STREQUAL "ALL")
    set(_NANOAI_RKNN_SELECTED_EXAMPLES ${_NANOAI_RKNN_AVAILABLE_EXAMPLES})
else()
    if(NANOAI_RKNN_EXAMPLES STREQUAL "NONE")
        message(FATAL_ERROR
            "NANOAI_RKNN_EXAMPLES='NONE' is not allowed. "
            "Use ALL or a semicolon-separated list of example names."
        )
    endif()

    set(_NANOAI_RKNN_SELECTED_EXAMPLES ${NANOAI_RKNN_EXAMPLES})

    foreach(_selected_example IN LISTS _NANOAI_RKNN_SELECTED_EXAMPLES)
        list(FIND _NANOAI_RKNN_AVAILABLE_EXAMPLES ${_selected_example} _selected_index)
        if(_selected_index LESS 0)
            message(FATAL_ERROR
                "Unknown example in NANOAI_RKNN_EXAMPLES: '${_selected_example}'. "
                "Available examples: ${_NANOAI_RKNN_AVAILABLE_EXAMPLES}"
            )
        endif()
    endforeach()
endif()

foreach(_example_name IN LISTS _NANOAI_RKNN_AVAILABLE_EXAMPLES)
    string(TOUPPER ${_example_name} _example_name_upper)
    set(_example_option_name NANOAI_ENABLE_EXAMPLE_${_example_name_upper})
    option(${_example_option_name} "Build ${_example_name} example" OFF)

    if(NANOAI_RKNN_EXAMPLES STREQUAL "ALL")
        set(${_example_option_name} ON CACHE BOOL "Build ${_example_name} example" FORCE)
    else()
        list(FIND _NANOAI_RKNN_SELECTED_EXAMPLES ${_example_name} _selected_index)
        if(_selected_index GREATER -1)
            set(${_example_option_name} ON CACHE BOOL "Build ${_example_name} example" FORCE)
        else()
            set(${_example_option_name} OFF CACHE BOOL "Build ${_example_name} example" FORCE)
        endif()
    endif()

    if(${_example_option_name})
        add_subdirectory(${_example_name})
    endif()
endforeach()
