#include <beta/sdk.hpp>

#include <kconfig.hpp>
#include <ktrace.hpp>

#include <iostream>

namespace kconfig::demo::beta {

void EmitDemoOutput() {
    const ktrace::TraceLogger logger = kconfig::GetTraceLogger();
    (void)logger;
    std::cout << "[beta] kconfig demo sdk initialized\n";
}

} // namespace kconfig::demo::beta
