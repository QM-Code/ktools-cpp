#pragma once

#include <ki18n/i18n.hpp>

namespace ktrace {

class TraceLogger;

} // namespace ktrace

namespace ki18n {

ktrace::TraceLogger GetTraceLogger();

} // namespace ki18n
