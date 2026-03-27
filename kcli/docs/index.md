# Kcli Documentation

`kcli` is a compact C++20 SDK for executable startup and command-line parsing.
It is intentionally opinionated about normal CLI behavior:

- parse first
- fail early on invalid input
- do not run handlers until the full command line validates
- preserve the caller's `argv`
- support grouped inline roots such as `--trace-*` and `--config-*`

## Start Here

- [API guide](api.md)
- [Parsing behavior](behavior.md)
- [Examples](examples.md)

## Typical Flow

```cpp
kcli::Parser parser;
kcli::InlineParser build("--build");

build.setHandler("-profile", handleProfile, "Set build profile.");

parser.addInlineParser(build);
parser.addAlias("-v", "--verbose");
parser.setHandler("--verbose", handleVerbose, "Enable verbose logging.");

parser.parseOrExit(argc, argv);
```

## Core Concepts

`Parser`

- Owns top-level handlers, aliases, inline parser registrations, and the single
  parse pass.

`InlineParser`

- Defines one inline root namespace such as `--alpha`, `--trace`, or `--build`.

`HandlerContext`

- Exposes the effective option, command, root, and value tokens seen by the
  handler after alias expansion.

`CliError`

- Used by `parseOrThrow()` to surface invalid CLI input and handler failures.

## Which Entry Point Should I Use?

Use `parseOrExit()` when:

- you are in a normal executable `main()`
- invalid CLI input should print a standardized error and exit with code `2`
- you do not need custom formatting or recovery

Use `parseOrThrow()` when:

- you want to customize error formatting
- you want custom exit codes
- you want to intercept and test parse failures directly

## Build And Explore

```bash
kbuild --help
kbuild --build-latest
./demo/exe/core/build/latest/test --alpha-message "hello"
./demo/exe/omega/build/latest/test --build
```

## Working References

If you want to see complete, compiling examples, start with:

- [`demo/sdk/alpha/src/cli.cpp`](../demo/sdk/alpha/src/cli.cpp)
- [`demo/exe/core/src/main.cpp`](../demo/exe/core/src/main.cpp)
- [`demo/exe/omega/src/main.cpp`](../demo/exe/omega/src/main.cpp)
- [`cmake/tests/kcli_api_cases.cpp`](../cmake/tests/kcli_api_cases.cpp)

The public API contract lives in [`include/kcli.hpp`](../include/kcli.hpp).
