#include <kconfig.hpp>
#include <ktrace.hpp>

#include <iostream>

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    const ktrace::TraceLogger logger = kconfig::GetTraceLogger();
    (void)logger;
    std::cout << "Bootstrap succeeded.\n";
    return 0;
}
