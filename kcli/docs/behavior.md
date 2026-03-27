# Parsing Behavior

This page collects the parsing rules that matter in practice.

## Parse Lifecycle

`kcli` processes the command line in three stages:

1. Read the caller's `argv` into an internal token list.
2. Validate and schedule handler invocations in a single pass.
3. Execute scheduled handlers only after the full command line validates.

This means:

- handlers do not run on partially-valid command lines
- unknown options fail the parse before any handler side effects occur
- the caller's `argv` is never rewritten or compacted

## Option Naming Rules

Top-level handlers:

- accepted forms: `"name"` or `"--name"`
- effective option token at runtime: `--name`

Inline roots:

- accepted forms: `"build"` or `"--build"`
- effective bare root token at runtime: `--build`

Inline handlers:

- accepted forms: `"-flag"` or `"--build-flag"`
- effective option token at runtime: `--build-flag`

Aliases:

- alias form must be single-dash, such as `-v`
- target form must be double-dash, such as `--verbose`

## Inline Root Behavior

Bare inline roots behave specially.

`--build`

- prints a help listing for the `--build-*` handlers

`--build release`

- invokes the root value handler if one is registered
- fails if no root value handler is registered

If a root value handler is registered with a placeholder and description, the
bare-root help view includes a row such as:

```text
--build <selector>  Select build targets.
```

## Value Consumption Rules

`kcli` supports three public registration styles:

- flag handlers consume no trailing value tokens
- required-value handlers consume at least one value token
- optional-value handlers consume values only when the next token looks like a value

Additional details:

- once value collection starts, `kcli` keeps consuming subsequent non-option-like
  tokens for that handler
- explicit empty tokens are preserved
- joined handler values are produced by joining `value_tokens` with spaces

Examples:

```text
--name "Joe"            -> value_tokens = ["Joe"]
--name "Joe" "Smith"    -> value_tokens = ["Joe", "Smith"]
--name ""               -> value_tokens = [""]
--profile -debug        -> value_tokens = ["-debug"]
```

## Alias Behavior

Aliases are only expanded when a token is parsed as an option.

Examples:

```cpp
parser.addAlias("-v", "--verbose");
parser.addAlias("-c", "--config-load", {"user-file"});
```

Rules:

- consumed value tokens are not alias-expanded
- preset tokens are prepended to effective `value_tokens`
- preset tokens can satisfy required-value handlers
- aliases with preset tokens cannot target flag handlers

Example:

```text
-c settings.json
```

can behave like:

```text
--config-load user-file settings.json
```

## Positionals

The positional handler receives all remaining non-option tokens in a single
invocation.

Important details:

- explicit empty positional tokens are preserved
- positionals are dispatched only after option parsing succeeds

## Failure Behavior

Unknown option-like tokens fail the parse.

Notable cases:

- unknown top-level option: `--bogus`
- unknown inline option: `--build-unknown`
- literal `--`

`kcli` does not treat `--` as an option terminator. It is reported as an
unknown option.

## Error Surface

`parseOrExit()`

- prints `[error] [cli] ...` to `stderr`
- colors `error` red and `cli` blue on terminals
- exits with code `2`

`parseOrThrow()`

- throws `kcli::CliError`
- preserves the human-facing error message
- surfaces handler exceptions as `CliError`

## Behavior Coverage

The executable behavior is covered by:

- [`cmake/tests/kcli_api_cases.cpp`](../cmake/tests/kcli_api_cases.cpp)

The demo programs under [`demo/`](../demo/) show complete, compiling usage.
