# Beta Demo SDK

Exists for CI and as a minimal reference for integrating an SDK add-on with KconfigSDK.

This target defines its own `KTRACE_NAMESPACE`, includes `ktrace.hpp`, and
verifies that a non-executable SDK can still materialize `kconfig::GetTraceLogger()`
when it opts into `ktrace`.
