set(KCLI_SOURCES
    ${PROJECT_SOURCE_DIR}/src/kcli.cpp
    ${PROJECT_SOURCE_DIR}/src/kcli/impl.cpp
    ${PROJECT_SOURCE_DIR}/src/kcli/process.cpp
    ${PROJECT_SOURCE_DIR}/src/kcli/util.cpp
)

if(NOT KCLI_BUILD_STATIC AND NOT KCLI_BUILD_SHARED)
    message(FATAL_ERROR "kcli requires at least one of KCLI_BUILD_STATIC or KCLI_BUILD_SHARED to be ON.")
endif()

function(ktools_apply_runtime_rpath target_name)
    if(NOT TARGET "${target_name}")
        return()
    endif()
    if(NOT DEFINED KTOOLS_RUNTIME_RPATH_DIRS OR KTOOLS_RUNTIME_RPATH_DIRS STREQUAL "")
        return()
    endif()
    set_target_properties("${target_name}" PROPERTIES
        BUILD_RPATH "${KTOOLS_RUNTIME_RPATH_DIRS}"
    )
endfunction()

if(KCLI_BUILD_STATIC)
    add_library(kcli_sdk_static STATIC ${KCLI_SOURCES})
    add_library(kcli::sdk_static ALIAS kcli_sdk_static)

    target_include_directories(kcli_sdk_static
        PUBLIC
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )

    # Add link dependencies here when needed.
    # target_link_libraries(kcli_sdk_static PUBLIC some::dependency)

    set_target_properties(kcli_sdk_static PROPERTIES
        OUTPUT_NAME kcli
        EXPORT_NAME sdk_static
    )
endif()

if(KCLI_BUILD_SHARED)
    add_library(kcli_sdk_shared SHARED ${KCLI_SOURCES})
    add_library(kcli::sdk_shared ALIAS kcli_sdk_shared)

    target_include_directories(kcli_sdk_shared
        PUBLIC
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )

    # Add link dependencies here when needed.
    # target_link_libraries(kcli_sdk_shared PUBLIC some::dependency)

    set_target_properties(kcli_sdk_shared PROPERTIES
        OUTPUT_NAME kcli
        EXPORT_NAME sdk_shared
    )
    ktools_apply_runtime_rpath(kcli_sdk_shared)
endif()

if(TARGET kcli_sdk_shared)
    add_library(kcli::sdk ALIAS kcli_sdk_shared)
elseif(TARGET kcli_sdk_static)
    add_library(kcli::sdk ALIAS kcli_sdk_static)
endif()
