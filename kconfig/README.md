# Karma JSON Config Storage SDK

JSON configuration/data/serialization SDK.

## Build SDK

```bash
kbuild --build-latest
```

SDK output:
- `build/latest/sdk/include`
- `build/latest/sdk/lib`
- `build/latest/sdk/lib/cmake/KconfigSDK`

## Build and Test Demos

```bash
# Builds SDK plus kbuild.json "build.defaults.demos".
kbuild --build-latest

# Explicit demo-only run (uses build.demos when no args are provided).
kbuild --build-demos

./demo/exe/core/build/latest/test
```

Demos:
- Bootstrap compile/link check: `demo/bootstrap/`
- SDKs: `demo/sdk/{alpha,beta,gamma}`
- Executables: `demo/exe/{core,omega}`

Demo builds are orchestrated by the root `kbuild` command.

The core demo validates the common load/merge/read path. The omega demo exercises the fuller config, asset, user-store, and backing-file flows.

## Trace Integration

`kconfig` now follows the rewritten `ktrace` model:

- the library exposes `kconfig::GetTraceLogger()`
- executables own a `ktrace::Logger`
- library warnings/errors now emit through that trace logger
- trace CLI is logger-bound via `logger.makeInlineParser(...)`
- config CLI remains separate via `kconfig::cli::GetInlineParser()`

Executable integration looks like:

```cpp
ktrace::Logger logger;
ktrace::TraceLogger app_trace("my_app");
app_trace.addChannel("app");

logger.addTraceLogger(app_trace);
logger.addTraceLogger(kconfig::GetTraceLogger());

kcli::Parser parser;
parser.addInlineParser(logger.makeInlineParser(app_trace));
parser.addInlineParser(kconfig::cli::GetInlineParser());
```

`kconfig.hpp` forward-declares `ktrace::TraceLogger` so non-tracing consumers do not
inherit the `KTRACE_NAMESPACE` requirement just by including KConfig headers.
Any translation unit that materializes a `ktrace::TraceLogger` from
`kconfig::GetTraceLogger()` should include `ktrace.hpp`.
Operational warnings that previously depended on `spdlog` now flow through this
logger attachment as well.

## Coding Agents

If you are using a coding agent, paste the following prompt:

```bash
Follow the instructions in agent/BOOTSTRAP.md
```
