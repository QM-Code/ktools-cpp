# Bootstrap Demo

Exists for CI and as the smallest compile/link usage reference for KconfigSDK.

This demo also serves as the minimal `ktrace` integration check:

- target defines `KTRACE_NAMESPACE`
- translation unit includes `ktrace.hpp`
- code can materialize `kconfig::GetTraceLogger()`
