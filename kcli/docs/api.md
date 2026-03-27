# API Guide

This page summarizes the public types in
[`include/kcli.hpp`](../include/kcli.hpp).

## Core Types

| Type | Purpose |
| --- | --- |
| `kcli::Parser` | Owns aliases, top-level handlers, positional handling, and inline parser registration. |
| `kcli::InlineParser` | Defines one inline root namespace such as `--build` plus its `--build-*` handlers. |
| `kcli::HandlerContext` | Metadata delivered to flag, value, and positional handlers. |
| `kcli::CliError` | Exception used by `parseOrThrow()` for invalid CLI input and handler failures. |

## HandlerContext

`HandlerContext` is passed to every handler.

| Field | Meaning |
| --- | --- |
| `root` | Inline root name without leading dashes, such as `build`. Empty for top-level handlers and positional dispatch. |
| `option` | Effective option token after alias expansion, such as `--verbose` or `--build-profile`. Empty for positional dispatch. |
| `command` | Normalized command name without leading dashes. Empty for positional dispatch and inline root value handlers. |
| `value_tokens` | Effective value tokens after alias expansion. Tokens from the shell are preserved verbatim; alias preset tokens are prepended. |

## CliError

`parseOrThrow()` throws `CliError` when:

- the command line is invalid
- a registered option handler throws
- the positional handler throws

`CliError::option()` returns the option token associated with the failure when
one exists. For positional-handler failures and parser-global errors, it may be
empty.

## InlineParser

### Construction

```cpp
kcli::InlineParser parser("--build");
```

The root may be provided as either:

- `"build"`
- `"--build"`

### Root Value Handler

```cpp
parser.setRootValueHandler(handler);
parser.setRootValueHandler(handler, "<selector>", "Select build targets.");
```

The root value handler processes the bare root form, for example:

- `--build release`
- `--config user.json`

If the bare root is used without a value, `kcli` prints inline help for that
root instead.

### Inline Handlers

```cpp
parser.setHandler("-flag", flagHandler, "Enable build flag.");
parser.setHandler("-profile", valueHandler, "Set build profile.");
parser.setOptionalValueHandler("-enable", optionalHandler, "Enable build mode.");
```

Inline handler options may be written in either form:

- short inline form: `-profile`
- fully-qualified form: `--build-profile`

## Parser

### Top-Level Handlers

```cpp
parser.setHandler("--verbose", handleVerbose, "Enable verbose logging.");
parser.setHandler("--output", handleOutput, "Set output target.");
parser.setOptionalValueHandler("--color", handleColor, "Set or auto-detect color output.");
```

Top-level handler options may be written as either:

- `"verbose"`
- `"--verbose"`

### Aliases

```cpp
parser.addAlias("-v", "--verbose");
parser.addAlias("-c", "--config-load", {"user-file"});
```

Rules:

- aliases use single-dash form such as `-v`
- alias targets use double-dash form such as `--verbose`
- preset tokens are prepended to the handler's effective `value_tokens`

### Positional Handler

```cpp
parser.setPositionalHandler(handlePositionals);
```

The positional handler receives remaining non-option tokens in
`HandlerContext::value_tokens`.

### Inline Parser Registration

```cpp
parser.addInlineParser(buildParser);
```

Duplicate inline roots are rejected.

### Parse Entry Points

```cpp
parser.parseOrExit(argc, argv);
parser.parseOrThrow(argc, argv);
```

`parseOrExit()`

- preserves the caller's `argv`
- reports invalid CLI input to `stderr` as `[error] [cli] ...`
- exits with code `2`

`parseOrThrow()`

- preserves the caller's `argv`
- throws `kcli::CliError`
- does not run handlers until the full command line validates

## Value Handler Registration

Use the registration form that matches the CLI contract you want:

- `setHandler(option, FlagHandler, description)` for flag-style options
- `setHandler(option, ValueHandler, description)` for required values
- `setOptionalValueHandler(option, ValueHandler, description)` for optional values
- `setRootValueHandler(...)` for bare inline roots such as `--build release`

## API Notes

- `Parser` is moveable and not copyable.
- `InlineParser` is copyable and moveable.
- The public header is intended to be the source-of-truth contract for library
  consumers.
