include(CMakePackageConfigHelpers)

set(KTOOLS_INSTALL_CMAKEDIR "lib/cmake/KcliSDK")

set(_kcli_install_targets)
if(TARGET kcli_sdk_static)
    list(APPEND _kcli_install_targets kcli_sdk_static)
endif()
if(TARGET kcli_sdk_shared)
    list(APPEND _kcli_install_targets kcli_sdk_shared)
endif()

install(TARGETS ${_kcli_install_targets}
    EXPORT KcliSDKTargets
    ARCHIVE DESTINATION lib COMPONENT KcliSDK
    LIBRARY DESTINATION lib COMPONENT KcliSDK
    RUNTIME DESTINATION bin COMPONENT KcliSDK
    INCLUDES DESTINATION include
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
    DESTINATION include
    COMPONENT KcliSDK
    FILES_MATCHING PATTERN "*.hpp"
)

install(EXPORT KcliSDKTargets
    FILE KcliSDKTargets.cmake
    NAMESPACE kcli::
    DESTINATION ${KTOOLS_INSTALL_CMAKEDIR}
    COMPONENT KcliSDK
)

configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/cmake/KcliSDKConfig.cmake.in
    ${PROJECT_BINARY_DIR}/KcliSDKConfig.cmake
    INSTALL_DESTINATION ${KTOOLS_INSTALL_CMAKEDIR}
)

write_basic_package_version_file(
    ${PROJECT_BINARY_DIR}/KcliSDKConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${PROJECT_BINARY_DIR}/KcliSDKConfig.cmake
    ${PROJECT_BINARY_DIR}/KcliSDKConfigVersion.cmake
    DESTINATION ${KTOOLS_INSTALL_CMAKEDIR}
    COMPONENT KcliSDK
)
