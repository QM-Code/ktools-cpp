set(KI18N_SOURCES
    ${PROJECT_SOURCE_DIR}/src/ki18n.cpp
    ${PROJECT_SOURCE_DIR}/src/ki18n/i18n.cpp
)

if(NOT KI18N_BUILD_STATIC AND NOT KI18N_BUILD_SHARED)
    message(FATAL_ERROR "ki18n requires at least one of KI18N_BUILD_STATIC or KI18N_BUILD_SHARED to be ON.")
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

set(_ki18n_kconfig_static_dep kconfig::sdk_static)
if(NOT TARGET kconfig::sdk_static)
    set(_ki18n_kconfig_static_dep kconfig::sdk)
endif()

set(_ki18n_kcli_static_dep kcli::sdk_static)
if(NOT TARGET kcli::sdk_static)
    set(_ki18n_kcli_static_dep kcli::sdk)
endif()

set(_ki18n_ktrace_static_dep ktrace::sdk_static)
if(NOT TARGET ktrace::sdk_static)
    set(_ki18n_ktrace_static_dep ktrace::sdk)
endif()

set(_ki18n_kconfig_shared_dep kconfig::sdk_shared)
if(NOT TARGET kconfig::sdk_shared)
    set(_ki18n_kconfig_shared_dep kconfig::sdk)
endif()

set(_ki18n_kcli_shared_dep kcli::sdk_shared)
if(NOT TARGET kcli::sdk_shared)
    set(_ki18n_kcli_shared_dep kcli::sdk)
endif()

set(_ki18n_ktrace_shared_dep ktrace::sdk_shared)
if(NOT TARGET ktrace::sdk_shared)
    set(_ki18n_ktrace_shared_dep ktrace::sdk)
endif()

if(KI18N_BUILD_STATIC)
    add_library(ki18n_sdk_static STATIC ${KI18N_SOURCES})
    add_library(ki18n::sdk_static ALIAS ki18n_sdk_static)

    target_include_directories(ki18n_sdk_static
        PUBLIC
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )

    target_link_libraries(ki18n_sdk_static
        PUBLIC
            ${_ki18n_kconfig_static_dep}
            ${_ki18n_kcli_static_dep}
            ${_ki18n_ktrace_static_dep}
    )

    set_target_properties(ki18n_sdk_static PROPERTIES
        OUTPUT_NAME ki18n
        EXPORT_NAME sdk_static
    )
endif()

if(KI18N_BUILD_SHARED)
    add_library(ki18n_sdk_shared SHARED ${KI18N_SOURCES})
    add_library(ki18n::sdk_shared ALIAS ki18n_sdk_shared)

    target_include_directories(ki18n_sdk_shared
        PUBLIC
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )

    target_link_libraries(ki18n_sdk_shared
        PUBLIC
            ${_ki18n_kconfig_shared_dep}
            ${_ki18n_kcli_shared_dep}
            ${_ki18n_ktrace_shared_dep}
    )

    set_target_properties(ki18n_sdk_shared PROPERTIES
        OUTPUT_NAME ki18n
        EXPORT_NAME sdk_shared
    )
    ktools_apply_runtime_rpath(ki18n_sdk_shared)
endif()

if(TARGET ki18n_sdk_shared)
    add_library(ki18n::sdk ALIAS ki18n_sdk_shared)
elseif(TARGET ki18n_sdk_static)
    add_library(ki18n::sdk ALIAS ki18n_sdk_static)
endif()
