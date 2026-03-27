add_executable(kconfig_json_api_test
    kconfig_json_api_test.cpp
)

target_link_libraries(kconfig_json_api_test PRIVATE
    kconfig::sdk
)

add_test(
    NAME kconfig_json_api
    COMMAND kconfig_json_api_test
)
set_tests_properties(kconfig_json_api PROPERTIES TIMEOUT 30)
