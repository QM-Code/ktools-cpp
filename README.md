# ktools-cpp

`ktools-cpp/` is the C++ workspace for the broader **ktools** ecosystem. It
collects the C++ implementations of the shared libraries in one repo while
preserving clear component boundaries inside the tree.

The main workspace components are:

- `kcli/`
- `ktrace/`
- `kconfig/`
- `ki18n/`

If you are building, integrating, or consuming the C++ SDKs, start here. If you are modifying library internals or repo structure, see [AGENTS.md](AGENTS.md).

## Getting Started

Clone the workspace normally:

```bash
git clone git@github.com:QM-Code/ktools-cpp.git
```

The shared build tool is `kbuild`, expected on `PATH`.

Useful first checks:

```bash
kbuild --help
kbuild --batch --build-latest
```

`kbuild --batch` runs the command across the configured workspace components in
dependency order.

## Workspace Build Model

Use the workspace root when you want to build or clean the whole C++ stack:

```bash
kbuild --batch --build-latest
kbuild --batch --clean-latest
kbuild --batch --clean-all
```

Use an individual component directory when you only need one SDK or tool:

```bash
cd kcli
kbuild --build-latest
```

General output layout:

- core build tree: `build/<slot>/`
- core SDK install prefix: `build/<slot>/sdk/`
- demo build tree: `demo/<demo>/build/<slot>/`
- demo SDK install prefix: `demo/<demo>/build/<slot>/sdk/` when applicable

## Workspace Map

- `kcli/`
  CLI parsing SDK with top-level option parsing, inline `--<root>` parsing, aliases, and demo SDK/executable examples.
- `ktrace/`
  Trace/logging SDK layered on top of `kcli`.
- `kconfig/`
  JSON configuration/storage SDK layered on top of `kcli` and `ktrace`.
- `ki18n/`
  Internationalization/localization SDK layered on top of the stack above it.

## Common Commands

From a repo root:

```bash
kbuild --build-latest
kbuild --build <slot>
kbuild --build-demos [demo ...]
kbuild --clean-latest
kbuild --clean <slot>
```

If a repo uses `vcpkg`, first-time setup is usually:

```bash
kbuild --vcpkg-install
```

Running `kbuild` with no arguments from a valid repo root prints usage.

## Dependency Shape

The C++ components are intended to layer on each other:

1. `kcli`
2. `ktrace`
3. `kconfig`
4. `ki18n`

That dependency order matters both for SDK consumption and for demo builds.
Later components may depend on SDK artifacts produced by earlier ones.

## Where To Go Next

For implementation and API details, use the component docs directly:

- [kcli/README.md](kcli/README.md)
- [ktrace/README.md](ktrace/README.md)
- [kconfig/README.md](kconfig/README.md)
- [ki18n/README.md](ki18n/README.md)
- [../kbuild/README.md](../kbuild/README.md)
