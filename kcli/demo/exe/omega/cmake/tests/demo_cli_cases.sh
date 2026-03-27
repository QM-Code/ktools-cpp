#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $0 <demo-binary> <case>"
    echo "Cases:"
    echo "  unknown_alpha_option"
    echo "  unknown_beta_option"
    echo "  unknown_newgamma_option"
    echo "  known_alpha_option"
    echo "  alpha_multi_value"
    echo "  beta_workers_option"
    echo "  beta_workers_invalid_option"
    echo "  newgamma_tag_option"
    echo "  alpha_help_root"
    echo "  newgamma_help_root"
    echo "  unknown_app_option"
    echo "  known_and_unknown_option"
    echo "  alpha_alias_option"
    echo "  output_option"
    echo "  output_alias_option"
    echo "  build_profile_option"
    echo "  build_alias_option"
    echo "  positional_args"
    echo "  double_dash_not_separator"
}

if [[ $# -ne 2 ]]; then
    usage
    exit 2
fi

binary="$1"
test_case="$2"

if [[ ! -x "$binary" ]]; then
    echo "Error: demo binary is not executable: $binary" >&2
    exit 2
fi

require_contains() {
    local output="$1"
    local needle="$2"
    if ! grep -Fq -- "$needle" <<<"$output"; then
        echo "Expected output to contain: $needle" >&2
        echo "--- output begin ---" >&2
        echo "$output" >&2
        echo "--- output end ---" >&2
        exit 1
    fi
}

require_not_contains() {
    local output="$1"
    local needle="$2"
    if grep -Fq -- "$needle" <<<"$output"; then
        echo "Expected output to not contain: $needle" >&2
        echo "--- output begin ---" >&2
        echo "$output" >&2
        echo "--- output end ---" >&2
        exit 1
    fi
}

run_case() {
    local -a args=("$@")
    local output
    set +e
    output="$("$binary" "${args[@]}" 2>&1)"
    local status=$?
    set -e
    echo "$status"$'\n'"$output"
}

run_and_split() {
    local payload
    payload="$(run_case "$@")"
    status="$(head -n1 <<<"$payload")"
    output="$(tail -n +2 <<<"$payload")"
}

status=0
output=""

case "$test_case" in
    unknown_alpha_option)
        run_and_split --alpha-d
        if [[ "$status" -eq 0 ]]; then
            echo "Expected non-zero exit status for unknown alpha option" >&2
            exit 1
        fi
        require_contains "$output" "[error] [cli] unknown option --alpha-d"
        ;;
    unknown_beta_option)
        run_and_split --beta-z
        if [[ "$status" -eq 0 ]]; then
            echo "Expected non-zero exit status for unknown beta option" >&2
            exit 1
        fi
        require_contains "$output" "[error] [cli] unknown option --beta-z"
        ;;
    unknown_newgamma_option)
        run_and_split --newgamma-wut
        if [[ "$status" -eq 0 ]]; then
            echo "Expected non-zero exit status for unknown newgamma option" >&2
            exit 1
        fi
        require_contains "$output" "[error] [cli] unknown option --newgamma-wut"
        ;;
    known_alpha_option)
        run_and_split --alpha-message hello
        if [[ "$status" -ne 0 ]]; then
            echo "Expected zero exit status for known alpha option" >&2
            echo "--- output begin ---" >&2
            echo "$output" >&2
            echo "--- output end ---" >&2
            exit 1
        fi
        require_contains "$output" "Processing --alpha-message with value \"hello\""
        require_not_contains "$output" "CLI error:"
        ;;
    alpha_multi_value)
        run_and_split --alpha-message hello world
        if [[ "$status" -ne 0 ]]; then
            echo "Expected zero exit status for multi-value alpha option" >&2
            echo "--- output begin ---" >&2
            echo "$output" >&2
            echo "--- output end ---" >&2
            exit 1
        fi
        require_contains "$output" "Processing --alpha-message with values [\"hello\",\"world\"]"
        require_not_contains "$output" "CLI error:"
        ;;
    beta_workers_option)
        run_and_split --beta-workers 8
        if [[ "$status" -ne 0 ]]; then
            echo "Expected zero exit status for beta workers option" >&2
            echo "--- output begin ---" >&2
            echo "$output" >&2
            echo "--- output end ---" >&2
            exit 1
        fi
        require_contains "$output" "Processing --beta-workers with value \"8\""
        require_not_contains "$output" "CLI error:"
        ;;
    beta_workers_invalid_option)
        run_and_split --beta-workers abc
        if [[ "$status" -eq 0 ]]; then
            echo "Expected non-zero exit status for invalid beta workers option" >&2
            exit 1
        fi
        require_contains "$output" "[error] [cli] option '--beta-workers': expected an integer"
        require_not_contains "$output" "Processing --beta-workers"
        ;;
    newgamma_tag_option)
        run_and_split --newgamma-tag prod
        if [[ "$status" -ne 0 ]]; then
            echo "Expected zero exit status for newgamma tag option" >&2
            echo "--- output begin ---" >&2
            echo "$output" >&2
            echo "--- output end ---" >&2
            exit 1
        fi
        require_contains "$output" "Processing --newgamma-tag with value \"prod\""
        require_not_contains "$output" "CLI error:"
        ;;
    alpha_help_root)
        run_and_split --alpha
        if [[ "$status" -ne 0 ]]; then
            echo "Expected zero exit status for alpha help root" >&2
            echo "--- output begin ---" >&2
            echo "$output" >&2
            echo "--- output end ---" >&2
            exit 1
        fi
        require_contains "$output" "Available --alpha-* options:"
        require_contains "$output" "--alpha-enable [value]"
        require_not_contains "$output" "CLI error:"
        ;;
    newgamma_help_root)
        run_and_split --newgamma
        if [[ "$status" -ne 0 ]]; then
            echo "Expected zero exit status for newgamma help root" >&2
            echo "--- output begin ---" >&2
            echo "$output" >&2
            echo "--- output end ---" >&2
            exit 1
        fi
        require_contains "$output" "Available --newgamma-* options:"
        require_contains "$output" "--newgamma-tag <value>"
        require_not_contains "$output" "CLI error:"
        ;;
    unknown_app_option)
        run_and_split --bogus
        if [[ "$status" -eq 0 ]]; then
            echo "Expected non-zero exit status for unknown app option" >&2
            exit 1
        fi
        require_contains "$output" "[error] [cli] unknown option --bogus"
        ;;
    known_and_unknown_option)
        run_and_split --alpha-message hello --bogus
        if [[ "$status" -eq 0 ]]; then
            echo "Expected non-zero exit status when mixing known and unknown options" >&2
            exit 1
        fi
        require_contains "$output" "[error] [cli] unknown option --bogus"
        require_not_contains "$output" "Processing --alpha-message with value \"hello\""
        ;;
    alpha_alias_option)
        run_and_split -a
        if [[ "$status" -ne 0 ]]; then
            echo "Expected zero exit status for alpha alias option" >&2
            echo "--- output begin ---" >&2
            echo "$output" >&2
            echo "--- output end ---" >&2
            exit 1
        fi
        require_contains "$output" "Processing --alpha-enable"
        require_not_contains "$output" "CLI error:"
        ;;
    output_option)
        run_and_split --output stdout
        if [[ "$status" -ne 0 ]]; then
            echo "Expected zero exit status for output option" >&2
            echo "--- output begin ---" >&2
            echo "$output" >&2
            echo "--- output end ---" >&2
            exit 1
        fi
        require_contains "$output" "Enabled --<root> prefixes:"
        require_not_contains "$output" "CLI error:"
        ;;
    output_alias_option)
        run_and_split -out stdout
        if [[ "$status" -ne 0 ]]; then
            echo "Expected zero exit status for output alias option" >&2
            echo "--- output begin ---" >&2
            echo "$output" >&2
            echo "--- output end ---" >&2
            exit 1
        fi
        require_contains "$output" "Enabled --<root> prefixes:"
        require_not_contains "$output" "CLI error:"
        ;;
    build_profile_option)
        run_and_split --build-profile debug
        if [[ "$status" -ne 0 ]]; then
            echo "Expected zero exit status for build profile option" >&2
            echo "--- output begin ---" >&2
            echo "$output" >&2
            echo "--- output end ---" >&2
            exit 1
        fi
        require_contains "$output" "Enabled --<root> prefixes:"
        require_not_contains "$output" "CLI error:"
        ;;
    build_alias_option)
        run_and_split -b debug
        if [[ "$status" -ne 0 ]]; then
            echo "Expected zero exit status for build alias option" >&2
            echo "--- output begin ---" >&2
            echo "$output" >&2
            echo "--- output end ---" >&2
            exit 1
        fi
        require_contains "$output" "Enabled --<root> prefixes:"
        require_not_contains "$output" "CLI error:"
        ;;
    positional_args)
        run_and_split input-a input-b
        if [[ "$status" -ne 0 ]]; then
            echo "Expected zero exit status for positional arguments" >&2
            echo "--- output begin ---" >&2
            echo "$output" >&2
            echo "--- output end ---" >&2
            exit 1
        fi
        require_not_contains "$output" "CLI error:"
        ;;
    double_dash_not_separator)
        run_and_split -- --alpha-message hello
        if [[ "$status" -eq 0 ]]; then
            echo "Expected non-zero exit status when passing '--'" >&2
            exit 1
        fi
        require_contains "$output" "[error] [cli] unknown option --"
        require_not_contains "$output" "Processing --alpha-message with value \"hello\""
        ;;
    *)
        echo "Error: unknown case '$test_case'" >&2
        usage
        exit 2
        ;;
esac

echo "PASS: $test_case"
