#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $0 <demo-binary> <repo-root> <case>"
    echo "Cases:"
    echo "  baseline_success"
    echo "  config_help"
    echo "  triple_merge_success"
    echo "  backing_write_success"
    echo "  config_user_override_success"
    echo "  cli_config_override_success"
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

run_case_with_env() {
    local env_name="$1"
    local env_value="$2"
    shift 2
    local -a args=("$@")
    local output
    set +e
    output="$(cd "$repo_root" && env "$env_name=$env_value" "$binary" "${args[@]}" 2>&1)"
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

run_and_split_with_env() {
    local env_name="$1"
    local env_value="$2"
    shift 2
    local payload
    payload="$(run_case_with_env "$env_name" "$env_value" "$@")"
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
        require_contains "$output" "retries=11"
        require_contains "$output" "language=fr"
        require_contains "$output" "name=runtime-session"
        require_contains "$output" "banner=runtime-banner"
        require_contains "$output" "preview=KI18N pret"
        require_contains "$output" "weights=[9, 8, 7, 6]"
        require_contains "$output" "KI18N demo omega compile/link/integration check passed"
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
        require_contains "$output" "KI18N demo omega compile/link/integration check passed"
        ;;
    triple_merge_success)
        run_and_split
        if [[ "$status" -ne 0 ]]; then
            echo "Expected zero exit status for triple-merge run" >&2
            echo "--- output begin ---" >&2
            echo "$output" >&2
            echo "--- output end ---" >&2
            exit 1
        fi
        require_contains "$output" "port=7777"
        require_contains "$output" "retries=11"
        require_contains "$output" "language=fr"
        require_contains "$output" "name=runtime-session"
        require_contains "$output" "banner=runtime-banner"
        require_contains "$output" "preview=KI18N pret"
        require_contains "$output" "weights=[9, 8, 7, 6]"
        require_contains "$output" "KI18N demo omega compile/link/integration check passed"
        ;;
    backing_write_success)
        run_and_split_with_env "KI18N_DEMO_TEST_BACKING" "1"
        if [[ "$status" -ne 0 ]]; then
            echo "Expected zero exit status for backing-write run" >&2
            echo "--- output begin ---" >&2
            echo "$output" >&2
            echo "--- output end ---" >&2
            exit 1
        fi
        require_contains "$output" "Backing file roundtrip check passed"
        require_contains "$output" "KI18N demo omega compile/link/integration check passed"
        ;;
    config_user_override_success)
        tmp_dir="$(mktemp -d)"
        override_path="${tmp_dir}/override-config.json"
        xdg_config_home="${tmp_dir}/xdg-config"
        cat >"$override_path" <<'JSON'
{
  "feature": {
    "enabled": false
  },
  "numbers": {
    "u32": 42
  },
  "limits": {
    "timeout": 4.5
  },
  "positive": {
    "float": 2.5
  }
}
JSON
        mkdir -p "$xdg_config_home"
        set +e
        output="$(cd "$repo_root" && env XDG_CONFIG_HOME="$xdg_config_home" KI18N_DEMO_USER_CONFIG_DIRNAME="demo-app" "$binary" --config-user "$override_path" 2>&1)"
        status=$?
        set -e
        rm -rf "$tmp_dir"
        if [[ "$status" -ne 0 ]]; then
            echo "Expected zero exit status for config-user override run" >&2
            echo "--- output begin ---" >&2
            echo "$output" >&2
            echo "--- output end ---" >&2
            exit 1
        fi
        require_contains "$output" "enabled=false"
        require_contains "$output" "maxSessions=42"
        require_contains "$output" "timeout=4.5"
        require_contains "$output" "scale=2.5"
        require_contains "$output" "banner=runtime-banner"
        require_contains "$output" "preview=KI18N pret"
        require_contains "$output" "KI18N demo omega compile/link/integration check passed"
        ;;
    cli_config_override_success)
        run_and_split \
            --config "\"network.port\"=5001" \
            --config "\"network.port\"=5002" \
            --config "\"numbers.u32\"=99" \
            --config "\"limits.timeout\"=7.75" \
            --config "\"text.name\"=\"cli-override\""
        if [[ "$status" -ne 0 ]]; then
            echo "Expected zero exit status for cli-config override run" >&2
            echo "--- output begin ---" >&2
            echo "$output" >&2
            echo "--- output end ---" >&2
            exit 1
        fi
        require_contains "$output" "port=5002"
        require_contains "$output" "maxSessions=99"
        require_contains "$output" "timeout=7.75"
        require_contains "$output" "name=cli-override"
        require_contains "$output" "banner=runtime-banner"
        require_contains "$output" "preview=KI18N pret"
        require_contains "$output" "KI18N demo omega compile/link/integration check passed"
        ;;
    *)
        echo "Error: unknown case '$test_case'" >&2
        usage
        exit 2
        ;;
esac

echo "PASS: $test_case"
