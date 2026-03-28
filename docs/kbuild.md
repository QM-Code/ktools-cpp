# `kbuild` In `ktools-cpp`

The C++ workspace uses the shared `kbuild` tool from the top-level `kbuild/`
repo rather than keeping a checked-in C++-local fork in this workspace.

## Current Usage

- run `kbuild` from `PATH`
- use the workspace root for batch builds across the C++ components
- use component directories when building or validating one library at a time

## Current Status

`kbuild` is fully part of the C++ development flow even though its source lives
outside `ktools-cpp/`.
