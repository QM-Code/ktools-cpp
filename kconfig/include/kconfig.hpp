#pragma once

#include <kconfig/asset.hpp>
#include <kconfig/cli.hpp>
#include <kconfig/json.hpp>
#include <kconfig/store.hpp>

namespace ktrace {

class TraceLogger;

} // namespace ktrace

namespace kconfig {

ktrace::TraceLogger GetTraceLogger();

} // namespace kconfig
