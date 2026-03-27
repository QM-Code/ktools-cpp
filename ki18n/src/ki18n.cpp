#include <ki18n.hpp>

#include <ktrace.hpp>

namespace ki18n {

ktrace::TraceLogger GetTraceLogger() {
    static ktrace::TraceLogger logger("ki18n");
    return logger;
}

} // namespace ki18n
