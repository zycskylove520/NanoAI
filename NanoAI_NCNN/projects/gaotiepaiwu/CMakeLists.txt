cmake_minimum_required(VERSION 3.16)

# This subproject must be built from NanoAI_NCNN root.
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR
        "projects/gaotiepaiwu does not support standalone configure. "
        "Configure from NanoAI_NCNN root with -DNANOAI_NCNN_BUILD_MODE=PROJECTS and select this project."
    )
endif()

set(NANOAI_NCNN_TARGET NanoAI_NCNN)

if(TARGET NanoAI_NCNN::NanoAI_NCNN)
    set(NANOAI_NCNN_TARGET NanoAI_NCNN::NanoAI_NCNN)
elseif(TARGET NanoAI_NCNN)
    set(NANOAI_NCNN_TARGET NanoAI_NCNN)
else()
    message(FATAL_ERROR
        "NanoAI_NCNN target not found. This subproject must be built from NanoAI_NCNN root."
    )
endif()

if(ANDROID)
    set(NANOAI_NCNN_GAOTIEPAIWU_TARGET gaotiepaiwu_project)

    add_library(${NANOAI_NCNN_GAOTIEPAIWU_TARGET} SHARED
        ${CMAKE_CURRENT_LIST_DIR}/src/paiwu_detector_android.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/paiwu_jni.cpp
    )

    target_include_directories(${NANOAI_NCNN_GAOTIEPAIWU_TARGET}
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/src
    )

    target_link_libraries(${NANOAI_NCNN_GAOTIEPAIWU_TARGET}
        PRIVATE
        ${NANOAI_NCNN_TARGET}
        log
        android
        jnigraphics
    )

    target_compile_features(${NANOAI_NCNN_GAOTIEPAIWU_TARGET}
        PRIVATE
        cxx_std_20
    )

    install(TARGETS ${NANOAI_NCNN_GAOTIEPAIWU_TARGET}
        LIBRARY DESTINATION lib
    )

    install(FILES ${CMAKE_CURRENT_LIST_DIR}/src/paiwu_jni.h
        DESTINATION include
    )
else()
    set(NANOAI_NCNN_GAOTIEPAIWU_TARGET NanoAI_ncnn_gaotiepaiwu_project)

    add_executable(${NANOAI_NCNN_GAOTIEPAIWU_TARGET}
        ${CMAKE_CURRENT_LIST_DIR}/src/gaotiepaiwu_demo_main.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/paiwu_detector.cpp
    )

    target_include_directories(${NANOAI_NCNN_GAOTIEPAIWU_TARGET}
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/src
    )

    target_link_libraries(${NANOAI_NCNN_GAOTIEPAIWU_TARGET}
        PRIVATE
        ${NANOAI_NCNN_TARGET}
    )

    target_compile_features(${NANOAI_NCNN_GAOTIEPAIWU_TARGET}
        PRIVATE
        cxx_std_20
    )

    install(TARGETS ${NANOAI_NCNN_GAOTIEPAIWU_TARGET}
        RUNTIME DESTINATION .
    )
endif()

if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/model)
    install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/model/
        DESTINATION model
    )
endif()
