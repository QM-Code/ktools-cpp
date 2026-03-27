# kcli-cpp

Assume these have already been read:

- `../../ktools/AGENTS.md`
- `../AGENTS.md`

`ktools-cpp/kcli/` is the C++ implementation of `kcli`.

## What This Repo Owns

This repo owns the C++ API and implementation details for `kcli`, including:

- public headers
- parser and inline-parser behavior
- CMake packaging for the C++ SDK
- C++ demos and tests

Cross-language conceptual behavior belongs to the `ktools/` overview docs. C++
workspace concerns belong to `ktools-cpp/`.

## Local Bootstrap

When familiarizing yourself with this repo, read:

- [README.md](README.md)
- `CMakeLists.txt`
- `include/*`
- `src/*`

If ongoing projects exist under `agent/projects/*.md`, review them and present them to the operator after bootstrap.

## Build And Test Expectations

- Use `kbuild` from the repo root for normal builds.
- Prefer end-to-end checks using the demo binaries under `demo/*/build/<slot>/`.
- Keep unit-style tests focused and explicit.

Useful commands:

```bash
kbuild --help
kbuild --build-latest
kbuild --build-demos
kbuild --clean-latest
```

## Guidance For Agents

1. Preserve the cross-language `kcli` behavior model unless the operator explicitly wants a C++-specific deviation.
2. Keep public API changes deliberate; downstream repos in the C++ stack depend on this SDK.
3. Prefer reading the demos when validating intended behavior; they are part of the contract, not just examples.
4. Surface issues or recommendations when you find them.
