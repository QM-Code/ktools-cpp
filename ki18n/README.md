# Karma JSON Internationalization SDK

JSON internationalization SDK extracted from `kconfig/`.

`ki18n` currently loads translation assets through `KconfigSDK`, and it keeps
direct `KcliSDK` and `KtraceSDK` imports in place for the planned inline CLI and
trace integration work.

## Build SDK

```bash
kbuild --build-latest
```

SDK output:
- `build/latest/sdk/include`
- `build/latest/sdk/lib`
- `build/latest/sdk/lib/cmake/Ki18nSDK`

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

The core demo exercises the basic config-backed language load path. The omega
demo keeps the fuller config, asset, i18n, and backing-file flow that used to
live in `kconfig/`.

## Trace Integration

`ki18n` now exposes `ki18n::GetTraceLogger()`. Operational warnings/errors from
language asset loading flow through that logger, so executables that want to see
them should attach it to their local `ktrace::Logger`.

## Coding Agents

If you are using a coding agent, paste the following prompt:

```bash
Follow the instructions in agent/BOOTSTRAP.md
```
