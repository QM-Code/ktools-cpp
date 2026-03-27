#pragma once

#include <kconfig.hpp>

#include <ktrace.hpp>

namespace kconfig::detail {

inline const ktrace::TraceLogger& GetLibraryTraceLogger() {
    static const ktrace::TraceLogger logger = kconfig::GetTraceLogger();
    return logger;
}

} // namespace kconfig::detail

#ifndef KTRACE
#define KTRACE(channel, ...) ::kconfig::detail::GetLibraryTraceLogger().trace(channel, __VA_ARGS__)
#endif
