#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $0 <demo-binary> <case>"
    echo "Cases:"
    echo "  unknown_alpha_option"
    echo "  known_alpha_option"
    echo "  alpha_multi_value"
    echo "  alpha_required_whitespace_value"
    echo "  alpha_optional_explicit_empty_value"
    echo "  alpha_required_alias_like_value"
    echo "  alpha_optional_no_value"
    echo "  alpha_alias_option"
    echo "  alpha_help_root"
    echo "  unknown_app_option"
    echo "  output_option"
    echo "  output_alias_option"
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
        require_not_contains "$output" "KCLI demo core compile/link/integration check passed"
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
    alpha_required_whitespace_value)
        run_and_split --alpha-message " Joe "
        if [[ "$status" -ne 0 ]]; then
            echo "Expected zero exit status for whitespace-preserved alpha value" >&2
            echo "--- output begin ---" >&2
            echo "$output" >&2
            echo "--- output end ---" >&2
            exit 1
        fi
        require_contains "$output" "Processing --alpha-message with value \" Joe \""
        require_not_contains "$output" "CLI error:"
        ;;
    alpha_optional_explicit_empty_value)
        run_and_split --alpha-enable ""
        if [[ "$status" -ne 0 ]]; then
            echo "Expected zero exit status for explicit empty optional alpha value" >&2
            echo "--- output begin ---" >&2
            echo "$output" >&2
            echo "--- output end ---" >&2
            exit 1
        fi
        require_contains "$output" "Processing --alpha-enable with value \"\""
        require_not_contains "$output" "CLI error:"
        ;;
    alpha_required_alias_like_value)
        run_and_split --alpha-message -a
        if [[ "$status" -ne 0 ]]; then
            echo "Expected zero exit status for alias-like alpha payload value" >&2
            echo "--- output begin ---" >&2
            echo "$output" >&2
            echo "--- output end ---" >&2
            exit 1
        fi
        require_contains "$output" "Processing --alpha-message with value \"-a\""
        require_not_contains "$output" "Processing --alpha-enable"
        require_not_contains "$output" "CLI error:"
        ;;
    alpha_optional_no_value)
        run_and_split --alpha-enable
        if [[ "$status" -ne 0 ]]; then
            echo "Expected zero exit status for optional alpha value" >&2
            echo "--- output begin ---" >&2
            echo "$output" >&2
            echo "--- output end ---" >&2
            exit 1
        fi
        require_contains "$output" "Processing --alpha-enable"
        require_not_contains "$output" "CLI error:"
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
    unknown_app_option)
        run_and_split --bogus
        if [[ "$status" -eq 0 ]]; then
            echo "Expected non-zero exit status for unknown app option" >&2
            exit 1
        fi
        require_contains "$output" "[error] [cli] unknown option --bogus"
        require_not_contains "$output" "KCLI demo core compile/link/integration check passed"
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
        require_contains "$output" "KCLI demo core compile/link/integration check passed"
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
        require_contains "$output" "KCLI demo core compile/link/integration check passed"
        require_not_contains "$output" "CLI error:"
        ;;
    double_dash_not_separator)
        run_and_split -- --alpha-message hello
        if [[ "$status" -eq 0 ]]; then
            echo "Expected non-zero exit status when passing '--'" >&2
            exit 1
        fi
        require_contains "$output" "[error] [cli] unknown option --"
        require_not_contains "$output" "KCLI demo core compile/link/integration check passed"
        ;;
    *)
        echo "Error: unknown case '$test_case'" >&2
        usage
        exit 2
        ;;
esac

echo "PASS: $test_case"
