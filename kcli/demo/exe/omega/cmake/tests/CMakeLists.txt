set(KCLI_DEMO_CLI_CASES_SCRIPT
    ${CMAKE_CURRENT_LIST_DIR}/demo_cli_cases.sh
)

add_test(
    NAME demo_cli_unknown_alpha_option_errors
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_omega>
            unknown_alpha_option
)

add_test(
    NAME demo_cli_unknown_beta_option_errors
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_omega>
            unknown_beta_option
)

add_test(
    NAME demo_cli_unknown_newgamma_option_errors
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_omega>
            unknown_newgamma_option
)

add_test(
    NAME demo_cli_known_alpha_option_ok
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_omega>
            known_alpha_option
)

add_test(
    NAME demo_cli_alpha_multi_value_ok
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_omega>
            alpha_multi_value
)

add_test(
    NAME demo_cli_beta_workers_option_ok
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_omega>
            beta_workers_option
)

add_test(
    NAME demo_cli_beta_workers_invalid_option_errors
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_omega>
            beta_workers_invalid_option
)

add_test(
    NAME demo_cli_newgamma_tag_option_ok
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_omega>
            newgamma_tag_option
)

add_test(
    NAME demo_cli_alpha_help_root_ok
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_omega>
            alpha_help_root
)

add_test(
    NAME demo_cli_newgamma_help_root_ok
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_omega>
            newgamma_help_root
)

add_test(
    NAME demo_cli_unknown_app_option_errors
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_omega>
            unknown_app_option
)

add_test(
    NAME demo_cli_known_and_unknown_option_errors
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_omega>
            known_and_unknown_option
)

add_test(
    NAME demo_cli_alpha_alias_option_ok
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_omega>
            alpha_alias_option
)

add_test(
    NAME demo_cli_output_option_ok
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_omega>
            output_option
)

add_test(
    NAME demo_cli_output_alias_option_ok
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_omega>
            output_alias_option
)

add_test(
    NAME demo_cli_build_profile_option_ok
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_omega>
            build_profile_option
)

add_test(
    NAME demo_cli_build_alias_option_ok
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_omega>
            build_alias_option
)

add_test(
    NAME demo_cli_positional_args_ok
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_omega>
            positional_args
)

add_test(
    NAME demo_cli_double_dash_not_separator_errors
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_omega>
            double_dash_not_separator
)
