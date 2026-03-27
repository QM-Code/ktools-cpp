# Karma CLI Parsing SDK

`kcli` is a small C++20 SDK for building structured command-line interfaces.
It is used by `ktrace` and `kconfig`, and is designed around two common CLI
shapes:

- Top-level options such as `--verbose` and `--output`.
- Inline roots such as `--trace-*`, `--config-*`, and `--build-*`.

The library gives you two explicit entrypoints:

- `parseOrExit(argc, argv)` for normal executable startup.
- `parseOrThrow(argc, argv)` when the caller wants to intercept `kcli::CliError`.

## Documentation

- [Overview and quick start](docs/index.md)
- [API guide](docs/api.md)
- [Parsing behavior](docs/behavior.md)
- [Examples](docs/examples.md)

## Quick Start

```cpp
#include <kcli.hpp>

void handleVerbose(const kcli::HandlerContext&) {
}

void handleProfile(const kcli::HandlerContext&, std::string_view) {
}

int main(int argc, char** argv) {
    kcli::Parser parser;
    kcli::InlineParser build("--build");

    build.setHandler("-profile", handleProfile, "Set build profile.");

    parser.addInlineParser(build);
    parser.addAlias("-v", "--verbose");
    parser.setHandler("--verbose", handleVerbose, "Enable verbose logging.");

    parser.parseOrExit(argc, argv);
    return 0;
}
```

## Behavior Highlights

- The full command line is validated before any registered handler runs.
- `parseOrExit()` preserves the caller's `argv`, reports invalid CLI input to
  `stderr`, and exits with code `2`.
- `parseOrThrow()` preserves the caller's `argv` and throws `kcli::CliError`.
- Bare inline roots such as `--build` print inline help unless a root value is
  provided.
- `setHandler(..., ValueHandler, ...)` registers a required-value option.
- `setOptionalValueHandler(...)` registers an optional-value option.
- Required values may consume a first token that begins with `-`.
- Literal `--` is rejected as an unknown option; it is not treated as an option
  terminator.

For the full parsing rules, see [docs/behavior.md](docs/behavior.md).

## Build SDK

```bash
kbuild --build-latest
```

SDK output:

- `build/latest/sdk/include`
- `build/latest/sdk/lib`
- `build/latest/sdk/lib/cmake/KcliSDK`

## Build And Run Demos

```bash
# Builds the SDK plus demos listed in kbuild.json build.defaults.demos.
kbuild --build-latest

# Explicit demo-only run (uses kbuild.json build.demos when no args are passed).
kbuild --build-demos
```

Demo directories:

- Bootstrap compile/link check: `demo/bootstrap/`
- SDK demos: `demo/sdk/{alpha,beta,gamma}`
- Executable demos: `demo/exe/{core,omega}`

Useful demo commands:

```bash
./demo/exe/core/build/latest/test
./demo/exe/core/build/latest/test --alpha
./demo/exe/core/build/latest/test --alpha-message "hello"
./demo/exe/core/build/latest/test --output stdout
./demo/exe/omega/build/latest/test --beta-workers 8
./demo/exe/omega/build/latest/test --newgamma-tag "prod"
./demo/exe/omega/build/latest/test --build
```

## Install

Consumer CMake:

```cmake
find_package(KcliSDK CONFIG REQUIRED)
target_link_libraries(main PRIVATE kcli::sdk)
```

## Repository Layout

- Public API: `include/kcli.hpp`
- Library implementation: `src/`
- API behavior coverage: `cmake/tests/kcli_api_cases.cpp`
- Integration demos: `demo/`

## Coding Agents

If you are using a coding agent, paste the following prompt:

```bash
Follow the instructions in agent/BOOTSTRAP.md
```
