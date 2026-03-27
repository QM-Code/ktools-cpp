# Deferred Issues - March 10, 2026

Captured during bootstrap review on March 10, 2026. These are intentionally deferred for later follow-up.

## 1. Store root type invariant can be broken

- `store::Set(name, ".", value)` can replace a namespace root with any JSON type in [src/kconfig/store/access.cpp#L106](/home/karmak/dev/ktools/kconfig/src/kconfig/store/access.cpp#L106).
- That conflicts with the object-root expectation enforced by [src/kconfig/store/layers.cpp#L114](/home/karmak/dev/ktools/kconfig/src/kconfig/store/layers.cpp#L114) and [src/kconfig/store/layers.cpp#L397](/home/karmak/dev/ktools/kconfig/src/kconfig/store/layers.cpp#L397).
- Risk: a config can be saved in memory and persisted, then fail normal reload/load flows because the root is no longer an object.
- Follow-up: decide whether all namespaces must remain object-rooted, then enforce that consistently in mutation and persistence paths.

## 2. `AddMutable` does not mark existing backed configs dirty

- Replacing a mutable namespace through [src/kconfig/store/layers.cpp#L106](/home/karmak/dev/ktools/kconfig/src/kconfig/store/layers.cpp#L106) sets `pendingSave = false` even if the namespace already has a backing file.
- The merge path in [src/kconfig/store/layers.cpp#L270](/home/karmak/dev/ktools/kconfig/src/kconfig/store/layers.cpp#L270) does mark backed mutable configs dirty, so behavior is inconsistent.
- Risk: `AddMutable` can leave the backing file stale until some later mutation triggers a save.
- Follow-up: align `AddMutable` dirty-state handling with merge/update behavior and add regression coverage.

## 3. Persistence silently rounds floats to 2 decimals

- [src/kconfig/store/persistence.cpp#L59](/home/karmak/dev/ktools/kconfig/src/kconfig/store/persistence.cpp#L59) rounds every floating-point value before writing JSON.
- Risk: config writes lose precision without the caller opting into that behavior or the README documenting it.
- Follow-up: confirm whether this is intended policy. If yes, document it clearly. If not, remove or gate it behind an explicit option.
