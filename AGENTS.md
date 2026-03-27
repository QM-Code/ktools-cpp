# ktools-cpp

Assume `../ktools/AGENTS.md` has already been read.

`ktools-cpp/` is the C++ workspace for the ktools ecosystem. It groups the C++ repos that are normally built and tested together:

- `kcli/`
- `ktrace/`
- `kconfig/`
- `ki18n/`

The shared `kbuild` tool lives outside this workspace in `../kbuild/` and is
expected on `PATH`.

## What This Level Owns

This workspace owns C++-specific concerns such as:

- C++ build and packaging flow
- repo-to-repo SDK dependencies inside the C++ stack
- batch orchestration across the C++ repos
- C++ implementation conventions and constraints

This workspace does not own the cross-language conceptual definition of the tools. That belongs at the overview/spec level.

## Guidance For Agents

When working in `ktools-cpp/`:

1. First determine whether the task belongs at the workspace root or in a child repo.
2. Prefer making changes in the narrowest repo that actually owns the behavior.
3. Use the root workspace only for C++-stack-wide concerns such as orchestration, dependency ordering, or root documentation.
4. Read the relevant child repo `AGENTS.md` and `README.md` files before changing code in that repo.

## Build Model

The shared build tool is `kbuild`, expected on `PATH`.

Typical workspace commands:

```bash
kbuild --batch --build-latest
kbuild --batch --clean-latest
kbuild --batch --git-sync "Sync child repos"
```

Typical child-repo commands:

```bash
cd kcli
kbuild --build-latest
```

Use `kbuild` from `PATH` as the entrypoint.
