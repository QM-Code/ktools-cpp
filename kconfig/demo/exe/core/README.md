# Core Demo

Basic load, merge, read, and integration showcase for KconfigSDK and the alpha demo SDK.

Trace integration in this demo matches the rewritten `ktrace` API:

- local demo channels are declared with a `ktrace::TraceLogger`
- `kconfig::GetTraceLogger()` is added to the executable-owned `ktrace::Logger`
- trace CLI is bound through `logger.makeInlineParser(tracer)`
- config CLI uses `kconfig::cli::GetInlineParser()`
