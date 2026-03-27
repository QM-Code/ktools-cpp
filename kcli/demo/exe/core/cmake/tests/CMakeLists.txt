set(KCLI_DEMO_CLI_CASES_SCRIPT
    ${CMAKE_CURRENT_LIST_DIR}/demo_cli_cases.sh
)

add_test(
    NAME demo_cli_unknown_alpha_option_errors
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_core>
            unknown_alpha_option
)

add_test(
    NAME demo_cli_known_alpha_option_ok
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_core>
            known_alpha_option
)

add_test(
    NAME demo_cli_alpha_multi_value_ok
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_core>
            alpha_multi_value
)

add_test(
    NAME demo_cli_alpha_required_whitespace_value_ok
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_core>
            alpha_required_whitespace_value
)

add_test(
    NAME demo_cli_alpha_optional_explicit_empty_value_ok
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_core>
            alpha_optional_explicit_empty_value
)

add_test(
    NAME demo_cli_alpha_required_alias_like_value_ok
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_core>
            alpha_required_alias_like_value
)

add_test(
    NAME demo_cli_alpha_optional_no_value_ok
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_core>
            alpha_optional_no_value
)

add_test(
    NAME demo_cli_alpha_alias_option_ok
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_core>
            alpha_alias_option
)

add_test(
    NAME demo_cli_alpha_help_root_ok
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_core>
            alpha_help_root
)

add_test(
    NAME demo_cli_unknown_app_option_errors
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_core>
            unknown_app_option
)

add_test(
    NAME demo_cli_output_option_ok
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_core>
            output_option
)

add_test(
    NAME demo_cli_output_alias_option_ok
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_core>
            output_alias_option
)

add_test(
    NAME demo_cli_double_dash_not_separator_errors
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_core>
            double_dash_not_separator
)
