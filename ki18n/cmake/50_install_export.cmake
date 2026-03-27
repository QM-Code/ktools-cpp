include(CMakePackageConfigHelpers)

set(KTOOLS_INSTALL_CMAKEDIR "lib/cmake/Ki18nSDK")

set(_ki18n_install_targets)
if(TARGET ki18n_sdk_static)
    list(APPEND _ki18n_install_targets ki18n_sdk_static)
endif()
if(TARGET ki18n_sdk_shared)
    list(APPEND _ki18n_install_targets ki18n_sdk_shared)
endif()

install(TARGETS ${_ki18n_install_targets}
    EXPORT Ki18nSDKTargets
    ARCHIVE DESTINATION lib COMPONENT Ki18nSDK
    LIBRARY DESTINATION lib COMPONENT Ki18nSDK
    RUNTIME DESTINATION bin COMPONENT Ki18nSDK
    INCLUDES DESTINATION include
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
    DESTINATION include
    COMPONENT Ki18nSDK
    FILES_MATCHING PATTERN "*.hpp"
)

install(EXPORT Ki18nSDKTargets
    FILE Ki18nSDKTargets.cmake
    NAMESPACE ki18n::
    DESTINATION ${KTOOLS_INSTALL_CMAKEDIR}
    COMPONENT Ki18nSDK
)

configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/cmake/Ki18nSDKConfig.cmake.in
    ${PROJECT_BINARY_DIR}/Ki18nSDKConfig.cmake
    INSTALL_DESTINATION ${KTOOLS_INSTALL_CMAKEDIR}
)

write_basic_package_version_file(
    ${PROJECT_BINARY_DIR}/Ki18nSDKConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${PROJECT_BINARY_DIR}/Ki18nSDKConfig.cmake
    ${PROJECT_BINARY_DIR}/Ki18nSDKConfigVersion.cmake
    DESTINATION ${KTOOLS_INSTALL_CMAKEDIR}
    COMPONENT Ki18nSDK
)
