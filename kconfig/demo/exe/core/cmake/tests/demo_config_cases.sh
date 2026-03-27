#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $0 <demo-binary> <repo-root> <case>"
    echo "Cases:"
    echo "  baseline_success"
    echo "  config_help"
    echo "  cli_config_override_success"
    echo "  trace_store_requests"
}

if [[ $# -ne 3 ]]; then
    usage
    exit 2
fi

binary="$1"
repo_root="$2"
test_case="$3"

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

run_case() {
    local -a args=("$@")
    local output
    set +e
    output="$(cd "$repo_root" && "$binary" "${args[@]}" 2>&1)"
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
    baseline_success)
        run_and_split
        if [[ "$status" -ne 0 ]]; then
            echo "Expected zero exit status for baseline run" >&2
            echo "--- output begin ---" >&2
            echo "$output" >&2
            echo "--- output end ---" >&2
            exit 1
        fi
        require_contains "$output" "port=7777"
        require_contains "$output" "enabled=true"
        require_contains "$output" "port=7777"
        require_contains "$output" "name=runtime-session"
        require_contains "$output" "[alpha] kconfig demo sdk initialized"
        require_contains "$output" "KConfig demo core compile/link/integration check passed"
        ;;
    config_help)
        run_and_split --config
        if [[ "$status" -ne 0 ]]; then
            echo "Expected zero exit status for bare config root" >&2
            echo "--- output begin ---" >&2
            echo "$output" >&2
            echo "--- output end ---" >&2
            exit 1
        fi
        require_contains "$output" "Available --config-* options:"
        require_contains "$output" "--config <assignment>"
        require_contains "$output" "KConfig demo core compile/link/integration check passed"
        ;;
    cli_config_override_success)
        run_and_split \
            --config "\"network.port\"=5002" \
            --config "\"text.name\"=\"cli-core\""
        if [[ "$status" -ne 0 ]]; then
            echo "Expected zero exit status for cli-config override run" >&2
            echo "--- output begin ---" >&2
            echo "$output" >&2
            echo "--- output end ---" >&2
            exit 1
        fi
        require_contains "$output" "port=5002"
        require_contains "$output" "name=cli-core"
        require_contains "$output" "KConfig demo core compile/link/integration check passed"
        ;;
    trace_store_requests)
        run_and_split --trace ".store.requests"
        if [[ "$status" -ne 0 ]]; then
            echo "Expected zero exit status for trace store requests run" >&2
            echo "--- output begin ---" >&2
            echo "$output" >&2
            echo "--- output end ---" >&2
            exit 1
        fi
        require_contains "$output" "core demo preparing config load defaults="
        require_contains "$output" "KConfig demo core compile/link/integration check passed"
        ;;
    *)
        echo "Error: unknown case '$test_case'" >&2
        usage
        exit 2
        ;;
esac

echo "PASS: $test_case"
