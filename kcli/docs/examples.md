# Examples

This page shows a few common `kcli` patterns. For complete compiling examples,
also see:

- [`demo/sdk/alpha/src/cli.cpp`](../demo/sdk/alpha/src/cli.cpp)
- [`demo/sdk/beta/src/cli.cpp`](../demo/sdk/beta/src/cli.cpp)
- [`demo/sdk/gamma/src/cli.cpp`](../demo/sdk/gamma/src/cli.cpp)
- [`demo/exe/core/src/main.cpp`](../demo/exe/core/src/main.cpp)
- [`demo/exe/omega/src/main.cpp`](../demo/exe/omega/src/main.cpp)

## Minimal Executable

```cpp
#include <kcli.hpp>

void handleVerbose(const kcli::HandlerContext&) {
}

int main(int argc, char** argv) {
    kcli::Parser parser;

    parser.addAlias("-v", "--verbose");
    parser.setHandler("--verbose", handleVerbose, "Enable verbose logging.");

    parser.parseOrExit(argc, argv);
    return 0;
}
```

## Inline Root With Subcommands-Like Options

```cpp
kcli::Parser parser;
kcli::InlineParser build("--build");

build.setHandler("-profile",
                 handleProfile,
                 "Set build profile.");
build.setHandler("-clean",
                 handleClean,
                 "Enable clean build.");

parser.addInlineParser(build);
parser.parseOrExit(argc, argv);
```

This enables:

```text
--build
--build-profile release
--build-clean
```

## Bare Root Value Handler

```cpp
kcli::InlineParser config("--config");

config.setRootValueHandler(handleConfigValue,
                           "<assignment>",
                           "Store a config assignment.");
```

This enables:

```text
--config
--config user=alice
```

Behavior:

- `--config` prints inline help
- `--config user=alice` invokes `handleConfigValue`

## Alias Preset Tokens

```cpp
kcli::Parser parser;

parser.addAlias("-c", "--config-load", {"user-file"});
parser.setHandler("--config-load", handleConfigLoad, "Load config.");
```

This makes:

```text
-c settings.json
```

behave like:

```text
--config-load user-file settings.json
```

Inside the handler:

- `context.option` is `--config-load`
- `context.value_tokens` is `["user-file", "settings.json"]`

## Optional Values

```cpp
parser.setOptionalValueHandler("--color",
                               handleColor,
                               "Set or auto-detect color output.");
```

This enables both:

```text
--color
--color always
```

## Positionals

```cpp
parser.setPositionalHandler(
    [&](const kcli::HandlerContext& context) {
        for (std::string_view token : context.value_tokens) {
            usePositional(token);
        }
    });
```

The positional handler receives all remaining non-option tokens after option
parsing succeeds.

## Custom Error Handling

If you want your own formatting or exit policy, use `parseOrThrow()`:

```cpp
try {
    parser.parseOrThrow(argc, argv);
} catch (const kcli::CliError& ex) {
    std::cerr << "custom cli error: " << ex.what() << "\n";
    return 2;
}
```

Use this when:

- you want custom error text
- you want custom logging
- you want a different exit code policy
