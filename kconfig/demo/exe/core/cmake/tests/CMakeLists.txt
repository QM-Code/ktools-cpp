get_filename_component(KCONFIG_DEMO_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.." ABSOLUTE)

set(KCONFIG_DEMO_CASES_SCRIPT
    ${CMAKE_CURRENT_LIST_DIR}/demo_config_cases.sh
)

add_test(
    NAME demo_config_baseline_success
    COMMAND ${KCONFIG_DEMO_CASES_SCRIPT}
            $<TARGET_FILE:kconfig_demo_core>
            ${KCONFIG_DEMO_REPO_ROOT}
            baseline_success
)

add_test(
    NAME demo_config_help
    COMMAND ${KCONFIG_DEMO_CASES_SCRIPT}
            $<TARGET_FILE:kconfig_demo_core>
            ${KCONFIG_DEMO_REPO_ROOT}
            config_help
)

add_test(
    NAME demo_config_cli_override_success
    COMMAND ${KCONFIG_DEMO_CASES_SCRIPT}
            $<TARGET_FILE:kconfig_demo_core>
            ${KCONFIG_DEMO_REPO_ROOT}
            cli_config_override_success
)

add_test(
    NAME demo_config_trace_store_requests
    COMMAND ${KCONFIG_DEMO_CASES_SCRIPT}
            $<TARGET_FILE:kconfig_demo_core>
            ${KCONFIG_DEMO_REPO_ROOT}
            trace_store_requests
)
