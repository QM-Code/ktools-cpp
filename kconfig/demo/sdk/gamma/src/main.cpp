#include <gamma/sdk.hpp>

#include <kconfig.hpp>
#include <ktrace.hpp>

#include <iostream>

namespace kconfig::demo::gamma {

void EmitDemoOutput() {
    const ktrace::TraceLogger logger = kconfig::GetTraceLogger();
    (void)logger;
    std::cout << "[gamma] kconfig demo sdk initialized\n";
}

} // namespace kconfig::demo::gamma
