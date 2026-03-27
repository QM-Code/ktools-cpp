set(KCONFIG_SOURCES
    ${PROJECT_SOURCE_DIR}/src/kconfig.cpp
    ${PROJECT_SOURCE_DIR}/src/kconfig/cli.cpp
    ${PROJECT_SOURCE_DIR}/src/kconfig/asset.cpp
    ${PROJECT_SOURCE_DIR}/src/kconfig/io.cpp
    ${PROJECT_SOURCE_DIR}/src/kconfig/json.cpp
    ${PROJECT_SOURCE_DIR}/src/kconfig/store.cpp
    ${PROJECT_SOURCE_DIR}/src/kconfig/store/fs.cpp
    ${PROJECT_SOURCE_DIR}/src/kconfig/store/user.cpp
    ${PROJECT_SOURCE_DIR}/src/kconfig/store/state.cpp
    ${PROJECT_SOURCE_DIR}/src/kconfig/store/layers.cpp
    ${PROJECT_SOURCE_DIR}/src/kconfig/store/access.cpp
    ${PROJECT_SOURCE_DIR}/src/kconfig/store/bindings.cpp
    ${PROJECT_SOURCE_DIR}/src/kconfig/store/read.cpp
    ${PROJECT_SOURCE_DIR}/src/kconfig/store/json_path.cpp
    ${PROJECT_SOURCE_DIR}/src/kconfig/store/persistence.cpp
)

if(NOT KCONFIG_BUILD_STATIC AND NOT KCONFIG_BUILD_SHARED)
    message(FATAL_ERROR "kconfig requires at least one of KCONFIG_BUILD_STATIC or KCONFIG_BUILD_SHARED to be ON.")
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

set(_kconfig_kcli_static_dep kcli::sdk_static)
if(NOT TARGET kcli::sdk_static)
    set(_kconfig_kcli_static_dep kcli::sdk)
endif()

set(_kconfig_ktrace_static_dep ktrace::sdk_static)
if(NOT TARGET ktrace::sdk_static)
    set(_kconfig_ktrace_static_dep ktrace::sdk)
endif()

set(_kconfig_kcli_shared_dep kcli::sdk_shared)
if(NOT TARGET kcli::sdk_shared)
    set(_kconfig_kcli_shared_dep kcli::sdk)
endif()

set(_kconfig_ktrace_shared_dep ktrace::sdk_shared)
if(NOT TARGET ktrace::sdk_shared)
    set(_kconfig_ktrace_shared_dep ktrace::sdk)
endif()

if(KCONFIG_BUILD_STATIC)
    add_library(kconfig_sdk_static STATIC ${KCONFIG_SOURCES})
    add_library(kconfig::sdk_static ALIAS kconfig_sdk_static)

    target_include_directories(kconfig_sdk_static
        PUBLIC
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            ${PROJECT_SOURCE_DIR}/src
    )

    target_link_libraries(kconfig_sdk_static
        PUBLIC
            ${_kconfig_kcli_static_dep}
            ${_kconfig_ktrace_static_dep}
    )

    target_compile_definitions(kconfig_sdk_static
        PRIVATE
            KTRACE_NAMESPACE="kconfig"
    )

    set_target_properties(kconfig_sdk_static PROPERTIES
        OUTPUT_NAME kconfig
        EXPORT_NAME sdk_static
    )
endif()

if(KCONFIG_BUILD_SHARED)
    add_library(kconfig_sdk_shared SHARED ${KCONFIG_SOURCES})
    add_library(kconfig::sdk_shared ALIAS kconfig_sdk_shared)

    target_include_directories(kconfig_sdk_shared
        PUBLIC
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            ${PROJECT_SOURCE_DIR}/src
    )

    target_link_libraries(kconfig_sdk_shared
        PUBLIC
            ${_kconfig_kcli_shared_dep}
            ${_kconfig_ktrace_shared_dep}
    )

    target_compile_definitions(kconfig_sdk_shared
        PRIVATE
            KTRACE_NAMESPACE="kconfig"
    )

    set_target_properties(kconfig_sdk_shared PROPERTIES
        OUTPUT_NAME kconfig
        EXPORT_NAME sdk_shared
    )
    ktools_apply_runtime_rpath(kconfig_sdk_shared)
endif()

if(TARGET kconfig_sdk_shared)
    add_library(kconfig::sdk ALIAS kconfig_sdk_shared)
elseif(TARGET kconfig_sdk_static)
    add_library(kconfig::sdk ALIAS kconfig_sdk_static)
endif()
