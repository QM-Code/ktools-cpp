# C++ kcli Project

## Mission

`ktools-cpp/kcli/` is the reference implementation. This project is not a parity
catch-up effort. The goal is to keep the C++ repo authoritative, explicit, and
easy for the other language agents to follow.

## Required Reading

- `../ktools/AGENTS.md`
- `AGENTS.md`
- `kcli/AGENTS.md`
- `kcli/README.md`
- `kcli/docs/api.md`
- `kcli/docs/behavior.md`
- `kcli/cmake/tests/kcli_api_cases.cpp`

## Current Position

- The repo already has the best overall structure in the stack.
- The public API surface is clear.
- The demo layout is the canonical shape that other languages should mirror.
- The main remaining job is to reduce ambiguity for downstream ports.

## Work Items

1. Audit the reference contract.
- Make sure every intended `kcli` behavior is captured in docs or tests.
- Treat missing docs and missing tests as reference defects.
- Prefer strengthening the contract instead of changing behavior.

2. Tighten structure only where it improves reference quality.
- Review whether `src/kcli/process.cpp` should be split further for readability.
- Review whether helper types in `src/kcli/backend.hpp` and `src/kcli/util.*`
  are placed where a porter would expect them.
- Do not refactor for style alone. Only refactor if it improves clarity for the
  other implementations.

3. Make the demos more obviously contractual.
- Confirm that `demo/bootstrap`, `demo/sdk/{alpha,beta,gamma}`, and
  `demo/exe/{core,omega}` express the intended layering clearly.
- Expand demo README files if they assume too much local knowledge.
- Keep the demo naming and scenario set stable unless there is a strong reason
  to change them across the stack.

4. Publish a clearer parity checklist for other agents.
- Add or improve documentation that states what every implementation must match:
  option normalization, alias semantics, inline-root semantics, help behavior,
  validation-before-handlers, error formatting, and exit/throw entrypoints.
- If the existing docs already cover this, tighten wording instead of creating
  duplicate documents.

5. Protect the reference with tests.
- Add focused tests when a behavior is currently documented but not asserted.
- Add focused docs when a test currently encodes behavior without explanation.

## Constraints

- Do not redesign the public API casually.
- Assume downstream ports depend on current semantics.
- Preserve the current demo topology unless a cross-language improvement is
  clearly worth the migration cost.

## Validation

- `cd kcli && kbuild --build-latest`
- `cd kcli && kbuild --build-demos`
- Run the demo binaries described in `kcli/README.md`
- Confirm docs, demos, and tests agree on behavior

## Done When

- The repo remains the clearest and most complete implementation.
- Another language agent can use the C++ repo as a reference without having to
  infer unstated behavior.
- Any added refactors improve clarity without destabilizing the public contract.
