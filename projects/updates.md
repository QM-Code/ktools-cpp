# C++ Updates

## Mission

Keep `ktools-cpp/` authoritative as the reference workspace for both `kcli`
and `ktrace`, and close the missing CMake residual-checker gap in the shared
`kbuild` repo.

Other language ports have moved forward. Re-read the current C++ docs, tests,
and demos with fresh eyes and tighten anything that is no longer explicit
enough for a downstream porter.

## Required Reading

- `../ktools/AGENTS.md`
- `AGENTS.md`
- `README.md`
- `kcli/AGENTS.md`
- `kcli/README.md`
- `kcli/docs/api.md`
- `kcli/docs/behavior.md`
- `kcli/cmake/tests/kcli_api_cases.cpp`
- `ktrace/AGENTS.md`
- `ktrace/README.md`
- `ktrace/include/ktrace.hpp`
- `ktrace/src/ktrace/cli.cpp`
- `ktrace/cmake/tests/ktrace_channel_semantics_test.cpp`
- `ktrace/cmake/tests/ktrace_format_api_test.cpp`
- `ktrace/cmake/tests/ktrace_log_api_test.cpp`
- `../kbuild/AGENTS.md`
- `../kbuild/README.md`
- `../kbuild/docs/commands.md`
- `../kbuild/libs/kbuild/residual_ops.py`
- `../kbuild/libs/kbuild/backend_ops.py`
- `../kbuild/libs/kbuild/cmake_backend.py`

## kcli Focus

- Audit `kcli` docs, focused tests, and demos together. Treat any
  documented-but-untested or tested-but-undocumented behavior as a reference
  defect.
- Tighten the explicit contract for option normalization, aliases, bare inline
  roots, root value handlers, optional and required value consumption,
  validation-before-handlers, and error behavior.
- Keep `demo/bootstrap`, `demo/sdk/{alpha,beta,gamma}`, and
  `demo/exe/{core,omega}` clearly instructional for downstream ports.

## ktrace Focus

- Audit `ktrace` docs, focused tests, and demos together. Treat the current C++
  repo as the place where selector behavior, logging rules, CLI integration,
  formatting, and `traceChanged(...)` become explicit.
- Tighten the contract for selector syntax, unmatched-selector warnings,
  logger and trace-source attachment rules, output options, local-selector
  resolution, and logger-bound inline parser behavior.
- Keep the demo topology contractual and do not introduce shared demo layers.

## kbuild Residual Focus

- Add a real residual checker for CMake-backed repos in the sibling
  `../kbuild/` repo.
- Wire that checker through the existing residual-hygiene flow so
  `kbuild --build-*` and `kbuild --git-sync` reject obvious CMake build
  artifacts outside `build/` trees.
- Add focused tests for the CMake checker and document the behavior alongside
  the existing residual guidance.
- After adding the checker, confirm that `ktools-cpp/kcli/`, `ktrace/`,
  `kconfig/`, and `ki18n/` do not leave residuals outside the intended build
  directories.

## Constraints

- Do not redesign the public C++ APIs casually.
- Prefer sharpening docs and focused tests over broad internal churn.
- Keep demos readable as separate entities that happen to work together.
- Keep the CMake residual checker strict enough to catch real source-tree
  build leakage without flagging normal handwritten project files.

## Validation

- `cd ktools-cpp/kcli && kbuild --build-latest`
- `cd ktools-cpp/kcli && kbuild --build-demos`
- `cd ktools-cpp/ktrace && kbuild --build-latest`
- `cd ktools-cpp/ktrace && kbuild --build-demos`
- Run the demo binaries described in `ktools-cpp/kcli/README.md`
- Run the demo binaries described in `ktools-cpp/ktrace/README.md`
- `cd ../kbuild && python3 -m unittest tests.test_cmake_residuals`
- Use the live residual checker against the real C++ component roots before
  finishing

## Done When

- The C++ workspace is still the clearest statement of the `kcli` and
  `ktrace` contracts.
- Another language agent can study the C++ docs, tests, and demos without
  guessing at behavior.
- `kbuild` has a working CMake residual checker with tests and docs.
- The C++ repos stay clean under that checker.
