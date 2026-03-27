include(CMakePackageConfigHelpers)

set(KTOOLS_INSTALL_CMAKEDIR "lib/cmake/KconfigSDK")

set(_kconfig_install_targets)
if(TARGET kconfig_sdk_static)
    list(APPEND _kconfig_install_targets kconfig_sdk_static)
endif()
if(TARGET kconfig_sdk_shared)
    list(APPEND _kconfig_install_targets kconfig_sdk_shared)
endif()

install(TARGETS ${_kconfig_install_targets}
    EXPORT KconfigSDKTargets
    ARCHIVE DESTINATION lib COMPONENT KconfigSDK
    LIBRARY DESTINATION lib COMPONENT KconfigSDK
    RUNTIME DESTINATION bin COMPONENT KconfigSDK
    INCLUDES DESTINATION include
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
    DESTINATION include
    COMPONENT KconfigSDK
    FILES_MATCHING PATTERN "*.hpp"
)

install(EXPORT KconfigSDKTargets
    FILE KconfigSDKTargets.cmake
    NAMESPACE kconfig::
    DESTINATION ${KTOOLS_INSTALL_CMAKEDIR}
    COMPONENT KconfigSDK
)

configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/cmake/KconfigSDKConfig.cmake.in
    ${PROJECT_BINARY_DIR}/KconfigSDKConfig.cmake
    INSTALL_DESTINATION ${KTOOLS_INSTALL_CMAKEDIR}
)

write_basic_package_version_file(
    ${PROJECT_BINARY_DIR}/KconfigSDKConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${PROJECT_BINARY_DIR}/KconfigSDKConfig.cmake
    ${PROJECT_BINARY_DIR}/KconfigSDKConfigVersion.cmake
    DESTINATION ${KTOOLS_INSTALL_CMAKEDIR}
    COMPONENT KconfigSDK
)
