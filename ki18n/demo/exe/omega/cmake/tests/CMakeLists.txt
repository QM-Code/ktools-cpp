get_filename_component(KI18N_DEMO_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.." ABSOLUTE)

set(KI18N_DEMO_CASES_SCRIPT
    ${CMAKE_CURRENT_LIST_DIR}/demo_i18n_cases.sh
)

add_test(
    NAME demo_i18n_baseline_success
    COMMAND ${KI18N_DEMO_CASES_SCRIPT}
            $<TARGET_FILE:ki18n_demo_omega>
            ${KI18N_DEMO_REPO_ROOT}
            baseline_success
)

add_test(
    NAME demo_i18n_help
    COMMAND ${KI18N_DEMO_CASES_SCRIPT}
            $<TARGET_FILE:ki18n_demo_omega>
            ${KI18N_DEMO_REPO_ROOT}
            config_help
)

add_test(
    NAME demo_i18n_user_override_success
    COMMAND ${KI18N_DEMO_CASES_SCRIPT}
            $<TARGET_FILE:ki18n_demo_omega>
            ${KI18N_DEMO_REPO_ROOT}
            config_user_override_success
)

add_test(
    NAME demo_i18n_cli_override_success
    COMMAND ${KI18N_DEMO_CASES_SCRIPT}
            $<TARGET_FILE:ki18n_demo_omega>
            ${KI18N_DEMO_REPO_ROOT}
            cli_config_override_success
)
