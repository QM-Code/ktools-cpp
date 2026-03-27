#include <alpha/sdk.hpp>

#include <kconfig.hpp>
#include <ktrace.hpp>

#include <iostream>

namespace kconfig::demo::alpha {

void EmitDemoOutput() {
    const ktrace::TraceLogger logger = kconfig::GetTraceLogger();
    (void)logger;
    std::cout << "[alpha] kconfig demo sdk initialized\n";
}

} // namespace kconfig::demo::alpha
