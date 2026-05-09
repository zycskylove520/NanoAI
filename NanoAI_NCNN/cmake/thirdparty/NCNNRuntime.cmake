include_guard(GLOBAL)

set(NANOAI_NCNN_NCNN_CONFIG_DIR "" CACHE PATH "ncnnConfig.cmake directory (optional, same as ncnn_DIR)")

if(NANOAI_NCNN_WITH_NCNN)
    set(_NANOAI_NCNN_NCNN_FOUND FALSE)

    # Keep compatibility with historical variable while using standard ncnn_DIR flow.
    if(NOT NANOAI_NCNN_NCNN_CONFIG_DIR STREQUAL "")
        set(ncnn_DIR "${NANOAI_NCNN_NCNN_CONFIG_DIR}" CACHE PATH "ncnn package config directory" FORCE)
    endif()

    if(NANOAI_NCNN_NCNN_CONFIG_DIR STREQUAL "")
        find_package(ncnn CONFIG QUIET)
    else()
        find_package(ncnn CONFIG QUIET)
    endif()

    if(TARGET ncnn)
        list(APPEND NANOAI_NCNN_LINK_LIBS ncnn)

        # Some ncnn package variants export include path as <prefix>/include/ncnn,
        # while this project includes headers as <ncnn/net.h>. Auto append
        # parent include directory when this pattern is detected.
        get_target_property(_ncnn_inc_dirs ncnn INTERFACE_INCLUDE_DIRECTORIES)
        if(_ncnn_inc_dirs)
            foreach(_ncnn_inc_dir IN LISTS _ncnn_inc_dirs)
                if(_ncnn_inc_dir MATCHES "/ncnn$")
                    get_filename_component(_ncnn_inc_parent "${_ncnn_inc_dir}" DIRECTORY)
                    list(APPEND NANOAI_NCNN_INCLUDE_DIRS ${_ncnn_inc_parent})
                endif()
            endforeach()
        endif()

        set(_NANOAI_NCNN_NCNN_FOUND TRUE)
    endif()

    if(NOT _NANOAI_NCNN_NCNN_FOUND)
        message(FATAL_ERROR
            "NCNN dependency is enabled but no ncnn package target was found. "
            "Please provide -Dncnn_DIR (or -DNANOAI_NCNN_NCNN_CONFIG_DIR for compatibility)."
        )
    endif()
endif()
